#include <errno.h>
#include <stdbool.h>
#include <termios.h>
#include <unistd.h>
#include "mode.h"
#include "../debug.h"
#include "../util/macros.h"

static struct termios termios_save;

static void xtcgetattr(struct termios *t)
{
    int ret = tcgetattr(STDIN_FILENO, t);
    if (unlikely(ret != 0)) {
        fatal_error("tcgetattr", errno);
    }
}

static void xtcsetattr(const struct termios *t)
{
    int ret;
    do {
        ret = tcsetattr(STDIN_FILENO, TCSANOW, t);
    } while (ret != 0 && errno == EINTR);

    if (unlikely(ret != 0)) {
        fatal_error("tcsetattr", errno);
    }
}

void term_raw(void)
{
    static bool saved;
    struct termios termios;
    xtcgetattr(&termios);
    if (!saved) {
        termios_save = termios;
        saved = true;
    }

    // Enter "raw" mode (roughly equivalent to cfmakeraw(3) on Linux/BSD)
    termios.c_iflag &= ~(
        ICRNL | IXON | IXOFF
        | IGNBRK | BRKINT | PARMRK
        | ISTRIP | INLCR | IGNCR
    );
    termios.c_oflag &= ~OPOST;
    termios.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    termios.c_cflag &= ~(CSIZE | PARENB);
    termios.c_cflag |= CS8;

    // Read at least 1 char on each read()
    termios.c_cc[VMIN] = 1;

    // Read blocks until there are MIN(VMIN, requested) bytes available
    termios.c_cc[VTIME] = 0;

    xtcsetattr(&termios);
}

void term_cooked(void)
{
    xtcsetattr(&termios_save);
}
