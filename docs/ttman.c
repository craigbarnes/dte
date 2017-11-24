// ttman - text to man converter
// Copyright 2017 Craig Barnes
// Copyright 2006-2010 Timo Hirvonen
// Licensed under the GPLv2

#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include "../src/macros.h"

typedef enum {
    TOK_TEXT, // Max. one line without \n
    TOK_NL, // \n
    TOK_ITALIC, // `text`
    TOK_BOLD, // *text*
    TOK_INDENT, // \t
    // Keywords:
    TOK_H1, // @h1
    TOK_H2, // @h2
    TOK_LI, // @li
    TOK_BR, // @br
    TOK_PRE, // @pre
    TOK_ENDPRE, // @endpre (must be after TOK_PRE)
    TOK_RAW, // @raw
    TOK_ENDRAW, // @endraw (must be after TOK_RAW)
    TOK_TITLE, // @title DTE 1 "July 2017"
} TokenType;

typedef struct Token Token;

struct Token {
    Token *next;
    Token *prev;
    TokenType type;
    size_t line;
    const char *text; // Not NUL-terminated
    size_t len; // Length of text
};

static const char *program;
static size_t cur_line = 1;

static Token head = {
    .next = &head,
    .prev = &head,
    .type = TOK_TEXT,
    .line = 0,
    .text = NULL,
    .len = 0
};

#define CONST_STR(str) {str, sizeof(str) - 1}

static const struct {
    const char *const str;
    size_t len;
} token_names[] = {
    CONST_STR("text"),
    CONST_STR("nl"),
    CONST_STR("italic"),
    CONST_STR("bold"),
    CONST_STR("indent"),
    // Keywords
    CONST_STR("h1"),
    CONST_STR("h2"),
    CONST_STR("li"),
    CONST_STR("br"),
    CONST_STR("pre"),
    CONST_STR("endpre"),
    CONST_STR("raw"),
    CONST_STR("endraw"),
    CONST_STR("title")
};

#define BUG() die("BUG in %s\n", __func__)

static NORETURN FORMAT(1) void die(const char *format, ...)
{
    va_list ap;
    fprintf(stderr, "%s: ", program);
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);
    exit(1);
}

static NORETURN FORMAT(2) void syntax(size_t line, const char *format, ...)
{
    va_list ap;
    fprintf(stderr, "line %zu: error: ", line);
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);
    exit(1);
}

static inline const char *keyword_name(TokenType type)
{
    if (type < TOK_H1 || type > TOK_TITLE) {
        die("BUG: no keyword name for type %u\n", type);
    }
    return token_names[type].str;
}

static void oom(size_t size)
{
    die("OOM when allocating %zu bytes\n", size);
}

static void *xmalloc(size_t size)
{
    void *ret = malloc(size);
    if (!ret) {
        oom(size);
    }
    return ret;
}

static char *memdup(const char *const str, size_t len)
{
    char *s = xmalloc(len + 1);
    memcpy(s, str, len);
    s[len] = '\0';
    return s;
}

static Token *new_token(TokenType type)
{
    Token *tok = xmalloc(sizeof(Token));
    tok->prev = NULL;
    tok->next = NULL;
    tok->type = type;
    tok->line = cur_line;
    return tok;
}

static void free_token(Token *tok)
{
    Token *prev = tok->prev;
    Token *next = tok->next;

    if (tok == &head) {
        BUG();
    }

    prev->next = next;
    next->prev = prev;
    free(tok);
}

static void emit_token(Token *tok)
{
    tok->prev = head.prev;
    tok->next = &head;
    head.prev->next = tok;
    head.prev = tok;
}

static void emit(TokenType type)
{
    Token *tok = new_token(type);
    tok->len = 0;
    tok->text = NULL;
    emit_token(tok);
}

static size_t emit_keyword(const char *const buf, size_t size)
{
    size_t len;
    for (len = 0; len < size; len++) {
        if (!isalnum((unsigned char)buf[len])) {
            break;
        }
    }

    if (!len) {
        syntax(cur_line, "keyword expected\n");
    }

    for (size_t i = TOK_H1; i < ARRAY_COUNT(token_names); i++) {
        if (len != token_names[i].len) {
            continue;
        }
        if (!strncmp(buf, token_names[i].str, len)) {
            emit(i);
            return len;
        }
    }
    syntax(cur_line, "invalid keyword '@%s'\n", memdup(buf, len));
}

static size_t emit_text(const char *const buf, size_t size)
{
    size_t i;
    for (i = 0; i < size; i++) {
        const char c = buf[i];
        if (
            c == '@' || c == '`' || c == '*' ||
            c == '\n' || c == '\\' || c == '\t'
        ) {
            break;
        }
    }
    Token *tok = new_token(TOK_TEXT);
    tok->text = buf;
    tok->len = i;
    emit_token(tok);
    return i;
}

static void tokenize(const char *buf, size_t size)
{
    size_t pos = 0;

    while (pos < size) {
        const char ch = buf[pos++];
        switch (ch) {
        case '@':
            pos += emit_keyword(buf + pos, size - pos);
            break;
        case '`':
            emit(TOK_ITALIC);
            break;
        case '*':
            emit(TOK_BOLD);
            break;
        case '\n':
            emit(TOK_NL);
            cur_line++;
            break;
        case '\t':
            emit(TOK_INDENT);
            break;
        case '\\': {
            Token *tok = new_token(TOK_TEXT);
            tok->text = buf + pos;
            tok->len = 1;
            if (pos == size) {
                tok->text--;
            } else {
                pos++;
            }

            if (tok->text[0] == '\\') {
                tok->text = "\\\\";
                tok->len = 2;
            }

            emit_token(tok);
            break;
            }
        default:
            pos--;
            pos += emit_text(buf + pos, size - pos);
            break;
        }
    }
}

static bool is_empty_line(const Token *tok)
{
    while (tok != &head) {
        switch (tok->type) {
        case TOK_TEXT:
            for (size_t i = 0; i < tok->len; i++) {
                if (tok->text[i] != ' ') {
                    return false;
                }
            }
            break;
        case TOK_INDENT:
            break;
        case TOK_NL:
            return true;
        default:
            return false;
        }
        tok = tok->next;
    }
    return true;
}

static Token *remove_line(Token *tok)
{
    while (tok != &head) {
        Token *next = tok->next;
        TokenType type = tok->type;

        free_token(tok);
        tok = next;
        if (type == TOK_NL) {
            break;
        }
    }
    return tok;
}

static Token *skip_after(Token *tok, TokenType type)
{
    const Token *const save = tok;

    while (tok != &head) {
        if (tok->type == type) {
            tok = tok->next;
            if (tok->type != TOK_NL) {
                syntax (
                    tok->line,
                    "newline expected after @%s\n",
                    keyword_name(type)
                );
            }
            return tok->next;
        }
        if (tok->type >= TOK_H1) {
            syntax (
                tok->line,
                "keywords not allowed betweed @%s and @%s\n",
                keyword_name(type - 1),
                keyword_name(type)
            );
        }
        tok = tok->next;
    }
    syntax(save->prev->line, "missing @%s\n", keyword_name(type));
}

static Token *get_next_line(Token *tok)
{
    while (tok != &head) {
        const TokenType type = tok->type;
        tok = tok->next;
        if (type == TOK_NL) {
            break;
        }
    }
    return tok;
}

static Token *get_indent(Token *tok, int *ip)
{
    int i = 0;
    while (tok != &head && tok->type == TOK_INDENT) {
        tok = tok->next;
        i++;
    }
    *ip = i;
    return tok;
}

// Line must be non-empty
static Token *check_line(Token *tok, int *ip)
{
    Token *start = tok = get_indent(tok, ip);

    const TokenType tok_type = tok->type;
    switch (tok_type) {
    case TOK_TEXT:
    case TOK_BOLD:
    case TOK_ITALIC:
    case TOK_BR:
        tok = tok->next;
        while (tok != &head) {
            switch (tok->type) {
            case TOK_TEXT:
            case TOK_BOLD:
            case TOK_ITALIC:
            case TOK_BR:
            case TOK_INDENT:
                break;
            case TOK_NL:
                return start;
            default:
                syntax (
                    tok->line,
                    "@%s not allowed inside paragraph\n",
                    keyword_name(tok->type)
                );
                break;
            }
            tok = tok->next;
        }
        break;
    case TOK_H1:
    case TOK_H2:
    case TOK_TITLE:
        if (*ip) {
            goto indentation;
        }

        // Check arguments
        tok = tok->next;
        while (tok != &head) {
            switch (tok->type) {
            case TOK_TEXT:
            case TOK_INDENT:
                break;
            case TOK_NL:
                return start;
            default:
                syntax (
                    tok->line,
                    "@%s can contain only text\n",
                    keyword_name(tok_type)
                );
                break;
            }
            tok = tok->next;
        }
        break;
    case TOK_LI:
        // Check arguments
        tok = tok->next;
        while (tok != &head) {
            switch (tok->type) {
            case TOK_TEXT:
            case TOK_BOLD:
            case TOK_ITALIC:
            case TOK_INDENT:
                break;
            case TOK_NL:
                return start;
            default:
                syntax (
                    tok->line,
                    "@%s not allowed inside @li\n",
                    keyword_name(tok->type)
                );
                break;
            }
            tok = tok->next;
        }
        break;
    case TOK_PRE:
        // Checked later
        break;
    case TOK_RAW:
        if (*ip) {
            goto indentation;
        }
        // Checked later
        break;
    case TOK_ENDPRE:
    case TOK_ENDRAW:
        syntax(tok->line, "@%s not expected\n", keyword_name(tok->type));
        break;
    case TOK_NL:
    case TOK_INDENT:
        BUG();
        break;
    }
    return start;
indentation:
    syntax(tok->line, "indentation before @%s\n", keyword_name(tok->type));
}

static void insert_nl_before(Token *next)
{
    Token *prev = next->prev;
    Token *new = new_token(TOK_NL);
    new->prev = prev;
    new->next = next;
    prev->next = new;
    next->prev = new;
}

static void normalize(void)
{
    Token *tok = head.next;
    /*
     * >= 0 if previous line was text (== amount of indent)
     *   -1 if previous block was @pre (amount of indent doesn't matter)
     *   -2 otherwise (@h1 etc., indent was 0)
     */
    int prev_indent = -2;

    while (tok != &head) {
        bool new_para = false;

        // Remove empty lines
        while (is_empty_line(tok)) {
            tok = remove_line(tok);
            new_para = true;
            if (tok == &head) {
                return;
            }
        }

        // Skips indent
        Token *start = tok;
        int i;
        tok = check_line(tok, &i);

        switch (tok->type) {
        case TOK_TEXT:
        case TOK_ITALIC:
        case TOK_BOLD:
        case TOK_BR:
            // Normal text
            if (new_para && prev_indent >= -1) {
                // Previous line/block was text or @pre
                // and there was a empty line after it
                insert_nl_before(start);
            }

            if (!new_para && prev_indent == i) {
                // Join with previous line
                Token *nl = start->prev;

                if (nl->type != TOK_NL) {
                    BUG();
                }

                if (
                    (nl->prev != &head && nl->prev->type == TOK_BR)
                    || tok->type == TOK_BR
                ) {
                    // Don't convert \n after/before @br to ' '
                    free_token(nl);
                } else {
                    // Convert "\n" to " "
                    nl->type = TOK_TEXT;
                    nl->text = " ";
                    nl->len = 1;
                }

                // Remove indent
                while (start->type == TOK_INDENT) {
                    Token *next = start->next;
                    free_token(start);
                    start = next;
                }
            }

            prev_indent = i;
            tok = get_next_line(tok);
            break;
        case TOK_PRE:
        case TOK_RAW:
            // These can be directly after normal text
            // but not joined with the previous line
            if (new_para && prev_indent >= -1) {
                // Previous line/block was text or @pre
                // and there was a empty line after it
                insert_nl_before(start);
            }
            tok = skip_after(tok->next, tok->type + 1);
            prev_indent = -1;
            break;
        case TOK_H1:
        case TOK_H2:
        case TOK_LI:
        case TOK_TITLE:
            // Remove white space after H1, H2, L1 and TITLE
            tok = tok->next;
            while (tok != &head) {
                TokenType type = tok->type;
                Token *next;

                if (type == TOK_TEXT) {
                    while (tok->len && *tok->text == ' ') {
                        tok->text++;
                        tok->len--;
                    }
                    if (tok->len) {
                        break;
                    }
                }
                if (type != TOK_INDENT) {
                    break;
                }

                // Empty TOK_TEXT or TOK_INDENT
                next = tok->next;
                free_token(tok);
                tok = next;
            }
            // Not normal text. can't be joined
            prev_indent = -2;
            tok = get_next_line(tok);
            break;
        case TOK_NL:
        case TOK_INDENT:
        case TOK_ENDPRE:
        case TOK_ENDRAW:
            BUG();
            break;
        }
    }
}

#define output(str) fputs(str, stdout)

static void output_buf(const char *const buf, size_t len)
{
    size_t ret = fwrite(buf, 1, len, stdout);
    if (ret != len) {
        die("fwrite failed\n");
    }
}

static void output_text(const Token *tok)
{
    char buf[1024];
    const char *str = tok->text;
    size_t len = tok->len;
    size_t pos = 0;

    while (len) {
        const char c = *str++;
        if (pos >= sizeof(buf) - 1) {
            output_buf(buf, pos);
            pos = 0;
        }
        if (c == '-') {
            buf[pos++] = '\\';
        }
        buf[pos++] = c;
        len--;
    }

    if (pos) {
        output_buf(buf, pos);
    }
}

static int bold = 0;
static int italic = 0;
static int indent = 0;

static Token *output_pre(Token *tok)
{
    bool bol = true;

    if (tok->type != TOK_NL) {
        syntax(tok->line, "newline expected after @pre\n");
    }

    output(".nf\n");
    tok = tok->next;
    while (tok != &head) {
        if (bol) {
            int i;
            tok = get_indent(tok, &i);
            if (i != indent && tok->type != TOK_NL) {
                syntax(tok->line, "indent changed in @pre\n");
            }
        }

        switch (tok->type) {
        case TOK_TEXT:
            if (bol && tok->len && tok->text[0] == '.') {
                output("\\&");
            }
            output_text(tok);
            break;
        case TOK_NL:
            output("\n");
            bol = true;
            tok = tok->next;
            continue;
        case TOK_ITALIC:
            output("`");
            break;
        case TOK_BOLD:
            output("*");
            break;
        case TOK_INDENT:
            // FIXME: warn
            output(" ");
            break;
        case TOK_ENDPRE:
            output(".fi\n");
            tok = tok->next;
            if (tok != &head && tok->type == TOK_NL) {
                tok = tok->next;
            }
            return tok;
        default:
            BUG();
            break;
        }
        bol = false;
        tok = tok->next;
    }
    return tok;
}

static Token *output_raw(Token *tok)
{
    if (tok->type != TOK_NL) {
        syntax(tok->line, "newline expected after @raw\n");
    }

    tok = tok->next;
    while (tok != &head) {
        switch (tok->type) {
        case TOK_TEXT:
            if (tok->len == 2 && !strncmp(tok->text, "\\\\", 2)) {
                // Ugly special case.
                // "\\" (\) was converted to "\\\\" (\\) because
                // nroff does escaping too.
                output("\\");
            } else {
                output_buf(tok->text, tok->len);
            }
            break;
        case TOK_NL:
            output("\n");
            break;
        case TOK_ITALIC:
            output("`");
            break;
        case TOK_BOLD:
            output("*");
            break;
        case TOK_INDENT:
            output("\t");
            break;
        case TOK_ENDRAW:
            tok = tok->next;
            if (tok != &head && tok->type == TOK_NL)
                tok = tok->next;
            return tok;
        default:
            BUG();
            break;
        }
        tok = tok->next;
    }
    return tok;
}

static Token *output_para(Token *tok)
{
    bool bol = true;
    while (tok != &head) {
        switch (tok->type) {
        case TOK_TEXT:
            output_text(tok);
            break;
        case TOK_ITALIC:
            italic ^= 1;
            if (italic) {
                output("\\fI");
            } else {
                output("\\fR");
            }
            break;
        case TOK_BOLD:
            bold ^= 1;
            if (bold) {
                output("\\fB");
            } else {
                output("\\fR");
            }
            break;
        case TOK_BR:
            if (bol) {
                output(".br\n");
            } else {
                output("\n.br\n");
            }
            bol = true;
            tok = tok->next;
            continue;
        case TOK_NL:
            output("\n");
            return tok->next;
        case TOK_INDENT:
            output(" ");
            break;
        default:
            BUG();
            break;
        }
        bol = false;
        tok = tok->next;
    }
    return tok;
}

static Token *title(Token *tok, const char *const cmd)
{
    output(cmd);
    return output_para(tok->next);
}

static Token *dump_one(Token *tok)
{
    int i;
    tok = get_indent(tok, &i);
    if (tok->type != TOK_RAW) {
        while (indent < i) {
            output(".RS\n");
            indent++;
        }
        while (indent > i) {
            output(".RE\n");
            indent--;
        }
    }

    switch (tok->type) {
    case TOK_TEXT:
    case TOK_ITALIC:
    case TOK_BOLD:
    case TOK_BR:
        if (tok->type == TOK_TEXT && tok->len && tok->text[0] == '.') {
            output("\\&");
        }
        tok = output_para(tok);
        break;
    case TOK_H1:
        tok = title(tok, ".SH ");
        break;
    case TOK_H2:
        tok = title(tok, ".SS ");
        break;
    case TOK_LI:
        tok = title(tok, ".TP\n");
        break;
    case TOK_PRE:
        tok = output_pre(tok->next);
        break;
    case TOK_RAW:
        tok = output_raw(tok->next);
        break;
    case TOK_TITLE:
        tok = title(tok, ".TH ");
        // Must be after ".TH".
        // No hyphenation, adjust left.
        output(".nh\n.ad l\n");
        break;
    case TOK_NL:
        output("\n");
        tok = tok->next;
        break;
    case TOK_ENDPRE:
    case TOK_ENDRAW:
    case TOK_INDENT:
        BUG();
        break;
    }
    return tok;
}

static void dump(void)
{
    Token *tok = head.next;
    while (tok != &head) {
        tok = dump_one(tok);
    }
}

static void process(void)
{
    char *buf = NULL;
    size_t alloc = 0;
    size_t size = 0;

    while (1) {
        if (alloc - size == 0) {
            alloc += 8192;
            buf = realloc(buf, alloc);
            if (!buf) {
                oom(alloc);
            }
        }
        ssize_t rc = read(0, buf + size, alloc - size);
        if (rc < 0) {
            if (errno == EINTR) {
                continue;
            }
            die("read: %s\n", strerror(errno));
        }
        if (!rc) {
            break;
        }
        size += rc;
    }

    tokenize(buf, size);
    normalize();
    dump();
}

int main(int argc, char *argv[])
{
    program = argv[0];
    if (argc != 1) {
        fprintf(stderr, "Usage: %s\n", program);
        return 1;
    }
    process();
    return 0;
}
