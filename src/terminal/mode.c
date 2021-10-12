#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include "mode.h"
#include "util/debug.h"

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

static bool xtcsetattr(const struct termios *t)
{
    if (unlikely(!initialized)) {
        return true;
    }

    int r;
    do {
        r = tcsetattr(STDIN_FILENO, TCSANOW, t);
    } while (unlikely(r != 0 && errno == EINTR));

    return (r == 0);
}

bool term_raw(void)
{
    return xtcsetattr(&raw);
}

bool term_raw_isig(void)
{
    return xtcsetattr(&raw_isig);
}

bool term_cooked(void)
{
    return xtcsetattr(&cooked);
}
