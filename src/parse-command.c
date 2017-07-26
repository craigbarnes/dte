#include "command.h"
#include "editor.h"
#include "error.h"
#include "gbuf.h"
#include "ptr-array.h"
#include "env.h"
#include "common.h"
#include "uchar.h"

static GBUF(arg);

static void parse_home(const char *cmd, int *posp)
{
	int len, pos = *posp;
	const char *username = cmd + pos + 1;
	struct passwd *passwd;
	char buf[64];

	for (len = 0; username[len]; len++) {
		char ch = username[len];
		if (isspace(ch) || ch == '/' || ch == ':')
			break;
		if (!isalnum(ch))
			return;
	}

	if (!len) {
		gbuf_add_str(&arg, home_dir);
		*posp = pos + 1;
		return;
	}

	if (len >= sizeof(buf))
		return;

	memcpy(buf, username, len);
	buf[len] = 0;
	passwd = getpwnam(buf);
	if (!passwd)
		return;

	gbuf_add_str(&arg, passwd->pw_dir);
	*posp = pos + 1 + len;
}

static void parse_sq(const char *cmd, int *posp)
{
	int pos = *posp;

	while (1) {
		if (cmd[pos] == '\'') {
			pos++;
			break;
		}
		if (!cmd[pos])
			break;
		gbuf_add_byte(&arg, cmd[pos++]);
	}
	*posp = pos;
}

static int unicode_escape(const char *str, int count)
{
	unsigned int u = 0;
	int i;

	for (i = 0; i < count && str[i]; i++) {
		int x = hex_decode(str[i]);
		if (x < 0)
			break;
		u = u << 4 | x;
	}
	if (u_is_unicode(u)) {
		gbuf_add_ch(&arg, u);
	}
	return i;
}

static void parse_dq(const char *cmd, int *posp)
{
	int pos = *posp;

	while (cmd[pos]) {
		char ch = cmd[pos++];

		if (ch == '"')
			break;

		if (ch == '\\' && cmd[pos]) {
			ch = cmd[pos++];
			switch (ch) {
			case '\\':
			case '"':
				gbuf_add_byte(&arg, ch);
				break;
			case 'a':
				gbuf_add_ch(&arg, '\a');
				break;
			case 'b':
				gbuf_add_ch(&arg, '\b');
				break;
			case 'f':
				gbuf_add_ch(&arg, '\f');
				break;
			case 'n':
				gbuf_add_ch(&arg, '\n');
				break;
			case 'r':
				gbuf_add_ch(&arg, '\r');
				break;
			case 't':
				gbuf_add_ch(&arg, '\t');
				break;
			case 'v':
				gbuf_add_ch(&arg, '\v');
				break;
			case 'x':
				if (cmd[pos]) {
					int x1, x2;
					x1 = hex_decode(cmd[pos]);
					if (x1 >= 0 && cmd[++pos]) {
						x2 = hex_decode(cmd[pos]);
						if (x2 >= 0) {
							pos++;
							gbuf_add_byte(&arg, x1 << 4 | x2);
						}
					}
				}
				break;
			case 'u':
				pos += unicode_escape(cmd + pos, 4);
				break;
			case 'U':
				pos += unicode_escape(cmd + pos, 8);
				break;
			default:
				gbuf_add_ch(&arg, '\\');
				gbuf_add_byte(&arg, ch);
				break;
			}
		} else {
			gbuf_add_byte(&arg, ch);
		}
	}
	*posp = pos;
}

static void parse_var(const char *cmd, int *posp)
{
	int si = *posp;
	int ei = si;
	char *name, *value;

	while (1) {
		char ch = cmd[ei];

		if (isalpha(ch) || ch == '_') {
			ei++;
			continue;
		}
		if (ei > si && isdigit(ch)) {
			ei++;
			continue;
		}
		break;
	}
	*posp = ei;
	if (ei == si) {
		return;
	}
	name = xstrslice(cmd, si, ei);
	value = expand_builtin_env(name);
	if (value != NULL) {
		gbuf_add_str(&arg, value);
		free(value);
	} else {
		const char *val = getenv(name);
		if (val != NULL) {
			gbuf_add_str(&arg, val);
		}
	}
	free(name);
}

char *parse_command_arg(const char *cmd, bool tilde)
{
	int pos = 0;

	if (tilde && cmd[pos] == '~')
		parse_home(cmd, &pos);
	while (1) {
		char ch = cmd[pos];

		if (!ch || ch == ';' || isspace(ch))
			break;

		pos++;
		if (ch == '\'') {
			parse_sq(cmd, &pos);
		} else if (ch == '"') {
			parse_dq(cmd, &pos);
		} else if (ch == '$') {
			parse_var(cmd, &pos);
		} else if (ch == '\\') {
			if (!cmd[pos])
				break;
			gbuf_add_byte(&arg, cmd[pos++]);
		} else {
			gbuf_add_byte(&arg, ch);
		}
	}
	return gbuf_steal_cstring(&arg);
}

int find_end(const char *cmd, int pos, struct error **err)
{
	while (1) {
		char ch = cmd[pos];

		if (!ch || ch == ';' || isspace(ch))
			break;

		pos++;
		if (ch == '\'') {
			while (1) {
				if (cmd[pos] == '\'') {
					pos++;
					break;
				}
				if (!cmd[pos]) {
					*err = error_create("Missing '");
					return -1;
				}
				pos++;
			}
		} else if (ch == '"') {
			while (1) {
				if (cmd[pos] == '"') {
					pos++;
					break;
				}
				if (!cmd[pos]) {
					*err = error_create("Missing \"");
					return -1;
				}
				if (cmd[pos++] == '\\') {
					if (!cmd[pos])
						goto unexpected_eof;
					pos++;
				}
			}
		} else if (ch == '\\') {
			if (!cmd[pos])
				goto unexpected_eof;
			pos++;
		} else {
			switch (ch) {
			case '*':
			case '?':
			case '[':
			case '{':
				// Reserved for glob patterns.  Note that ] and } need not to be
				// reserved because those can be used alone (without [ or {).
				//
				// In shell these characters are not reserved. You can run "echo *"
				// in an empty directory and it prints "*". If globbing will be
				// implemented in this program, it would behave differently.
				// Patterns should always expand to "nothing" if it didn't match.
				// E.g. "open *" in an empty directory would expand to "open".
				*err = error_create("You need to escape %c (characters *?[{ are reserved)", ch);
				return -1;
			}
		}
	}
	return pos;
unexpected_eof:
	*err = error_create("Unexpected EOF");
	return -1;
}

bool parse_commands(struct ptr_array *array, const char *cmd, struct error **err)
{
	int pos = 0;

	while (1) {
		int end;

		while (isspace(cmd[pos]))
			pos++;

		if (!cmd[pos])
			break;

		if (cmd[pos] == ';') {
			ptr_array_add(array, NULL);
			pos++;
			continue;
		}

		end = find_end(cmd, pos, err);
		if (*err != NULL)
			return false;

		ptr_array_add(array, parse_command_arg(cmd + pos, true));
		pos = end;
	}
	ptr_array_add(array, NULL);
	return true;
}

char **copy_string_array(char **src, int count)
{
	char **dst = xnew(char *, count + 1);
	int i;

	for (i = 0; i < count; i++)
		dst[i] = xstrdup(src[i]);
	dst[i] = NULL;
	return dst;
}
