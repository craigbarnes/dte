#include "common.h"
#include "editor.h"

size_t count_nl(const char *buf, size_t size)
{
    const char *end = buf + size;
    size_t nl = 0;

    while (buf < end) {
        buf = memchr(buf, '\n', end - buf);
        if (!buf) {
            break;
        }
        buf++;
        nl++;
    }
    return nl;
}

size_t count_strings(char **strings)
{
    size_t count;
    for (count = 0; strings[count]; count++) {
        ;
    }
    return count;
}

void free_strings(char **strings)
{
    for (size_t i = 0; strings[i]; i++) {
        free(strings[i]);
    }
    free(strings);
}

int number_width(long n)
{
    int width = 0;

    if (n < 0) {
        n *= -1;
        width++;
    }
    do {
        n /= 10;
        width++;
    } while (n);
    return width;
}

bool buf_parse_long(const char *str, size_t size, size_t *posp, long *valp)
{
    size_t pos = *posp;
    size_t count = 0;
    int sign = 1;
    long val = 0;

    if (pos < size && str[pos] == '-') {
        sign = -1;
        pos++;
    }
    while (pos < size && ascii_isdigit(str[pos])) {
        long old = val;

        val *= 10;
        val += str[pos++] - '0';
        count++;
        if (val < old) {
            // Overflow
            return false;
        }
    }
    if (count == 0) {
        return false;
    }
    *posp = pos;
    *valp = sign * val;
    return true;
}

bool parse_long(const char **strp, long *valp)
{
    const char *str = *strp;
    size_t size = strlen(str);
    size_t pos = 0;

    if (buf_parse_long(str, size, &pos, valp)) {
        *strp = str + pos;
        return true;
    }
    return false;
}

bool str_to_long(const char *str, long *valp)
{
    return parse_long(&str, valp) && *str == 0;
}

bool str_to_int(const char *str, int *valp)
{
    long val;

    if (!str_to_long(str, &val) || val < INT_MIN || val > INT_MAX) {
        return false;
    }
    *valp = val;
    return true;
}

char *xvsprintf(const char *format, va_list ap)
{
    char buf[4096];
    vsnprintf(buf, sizeof(buf), format, ap);
    return xstrdup(buf);
}

char *xsprintf(const char *format, ...)
{
    va_list ap;
    char *str;
    va_start(ap, format);
    str = xvsprintf(format, ap);
    va_end(ap);
    return str;
}

ssize_t xread(int fd, void *buf, size_t count)
{
    char *b = buf;
    size_t pos = 0;

    do {
        ssize_t rc = read(fd, b + pos, count - pos);
        if (rc == -1) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (rc == 0) {
            // eof
            break;
        }
        pos += rc;
    } while (count - pos > 0);
    return pos;
}

ssize_t xwrite(int fd, const void *buf, size_t count)
{
    const char *b = buf;
    size_t count_save = count;

    do {
        int rc;

        rc = write(fd, b, count);
        if (rc == -1) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        b += rc;
        count -= rc;
    } while (count > 0);
    return count_save;
}

// Returns size of file or -1 on error.
// For empty file *bufp is NULL otherwise *bufp is NUL-terminated.
ssize_t read_file(const char *filename, char **bufp)
{
    struct stat st;
    return stat_read_file(filename, bufp, &st);
}

ssize_t stat_read_file(const char *filename, char **bufp, struct stat *st)
{
    int fd = open(filename, O_RDONLY);
    *bufp = NULL;
    if (fd == -1) {
        return -1;
    }
    if (fstat(fd, st) == -1) {
        close(fd);
        return -1;
    }
    char *buf = xnew(char, st->st_size + 1);
    ssize_t r = xread(fd, buf, st->st_size);
    close(fd);
    if (r > 0) {
        buf[r] = 0;
        *bufp = buf;
    } else {
        free(buf);
    }
    return r;
}

char *buf_next_line(char *buf, ssize_t *posp, ssize_t size)
{
    ssize_t pos = *posp;
    ssize_t avail = size - pos;
    char *line = buf + pos;
    char *nl = memchr(line, '\n', avail);
    if (nl) {
        *nl = 0;
        *posp += nl - line + 1;
    } else {
        line[avail] = 0;
        *posp += avail;
    }
    return line;
}

void term_cleanup(void)
{
    if (
        !editor.child_controls_terminal
        && editor.status != EDITOR_INITIALIZING
    ) {
        ui_end();
    }
}

NORETURN void bug (
    const char *file,
    int line,
    const char *func,
    const char *fmt,
    ...
) {
    term_cleanup();

    fprintf(stderr, "\n%s:%d: **BUG** in %s() function: '", file, line, func);

    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    fputs("'\n", stderr);

    abort(); // For core dump
}

void debug_print(const char *function, const char *fmt, ...)
{
    static int fd = -1;
    if (fd < 0) {
        char *filename = editor_file("debug.log");
        fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0666);
        free(filename);
        BUG_ON(fd < 0);

        // Don't leak file descriptor to parent processes
        int r = fcntl(fd, F_SETFD, FD_CLOEXEC);
        BUG_ON(r == -1);
    }

    char buf[4096];
    size_t write_max = ARRAY_COUNT(buf);
    const int len1 = snprintf(buf, write_max, "%s: ", function);
    BUG_ON(len1 <= 0 || len1 > write_max);
    write_max -= len1;

    va_list ap;
    va_start(ap, fmt);
    const int len2 = vsnprintf(buf + len1, write_max, fmt, ap);
    va_end(ap);
    BUG_ON(len2 <= 0 || len2 > write_max);

    xwrite(fd, buf, len1 + len2);
}
