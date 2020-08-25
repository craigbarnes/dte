#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include "mode.h"
#include "util/debug.h"
#include "util/macros.h"

static bool initialized;
static struct termios cooked, raw, raw_isig;

bool term_mode_init(void)
{
    BUG_ON(initialized);
    if (unlikely(tcgetattr(STDIN_FILENO, &cooked) != 0)) {
        return false;
    }

    // Set up "raw" mode (roughly equivalent to cfmakeraw(3) on Linux/BSD)
    raw = cooked;
    raw.c_iflag &= ~(
        ICRNL | IXON | IXOFF
        | IGNBRK | BRKINT | PARMRK
        | ISTRIP | INLCR | IGNCR
    );
    raw.c_oflag &= ~OPOST;
    raw.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    raw.c_cflag &= ~(CSIZE | PARENB);
    raw.c_cflag |= CS8;
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    // Set up raw+ISIG mode
    raw_isig = raw;
    raw_isig.c_lflag |= ISIG;
    raw_isig.c_cc[VSUSP] = _POSIX_VDISABLE;

    initialized = true;
    return true;
}

static void xtcsetattr(const struct termios *t)
{
    if (unlikely(!initialized)) {
        return;
    }

    int ret;
    do {
        ret = tcsetattr(STDIN_FILENO, TCSANOW, t);
    } while (unlikely(ret != 0 && errno == EINTR));

    if (unlikely(ret != 0)) {
        fatal_error("tcsetattr", errno);
    }
}

void term_raw(void)
{
    xtcsetattr(&raw);
}

void term_raw_isig(void)
{
    xtcsetattr(&raw_isig);
}

void term_cooked(void)
{
    xtcsetattr(&cooked);
}
