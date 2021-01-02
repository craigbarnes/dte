Packaging
=========

Installation variables
----------------------

The following Make variables may be useful when packaging `dte`:

* `prefix`: Top-level installation prefix (defaults to `/usr/local`).
* `bindir`: Installation prefix for program binary (defaults to
  `$prefix/bin`).
* `mandir`: Installation prefix for manual pages (defaults to
  `$prefix/share/man`).
* `DESTDIR`: Standard variable used for [staged installs].
* `V=1`: Enable verbose build output.

Example usage:

    make V=1
    make install V=1 prefix=/usr DESTDIR=pkg

Other variables
---------------

There are some other variables that may be useful in certain cases
(but typically shouldn't be used for general packaging):

* `ICONV_DISABLE=1`: Disable support for all file encodings except
  UTF-8, to avoid the need to link with the system [iconv] library.
  This can significantly reduce the size of statically linked builds.
* `BUILTIN_SYNTAX_FILES`: Specify the [syntax highlighters] to compile
  into the editor. The default value for this contributes about 100KiB
  to the binary size.

Example usage:

    make ICONV_DISABLE=1 BUILTIN_SYNTAX_FILES='dte config ini sh'

Persistent configuration
------------------------

The above variables can also be configured persistently by adding them
to a `Config.mk` file, for example:

    prefix = /usr
    mandir = $(prefix)/man
    V = 1

The `Config.mk` file should be in the project base directory alongside
`GNUmakefile` and *must* be valid GNU make syntax.

Stable release tarballs
-----------------------

The [releases] page contains a short summary of changes for each
stable version and links to the corresponding source tarballs.

Note: auto-generated tarballs from GitHub/GitLab can (and
[do][libgit issue #4343]) change over time and cannot be guaranteed to
have long-term stable checksums. Use the tarballs from the [releases]
page, unless you're prepared to deal with future checksum failures.

Desktop menu entry
------------------

A desktop menu entry for `dte` can be added by running:

    make install-desktop-file

Any variable overrides specified for `make install` must also be specified
for `make install-desktop-file`. The easiest way to do this is simply to
run both at the same time, e.g.:

    make install install-desktop-file V=1 prefix=/usr DESTDIR=PKG

**Note**: the `install-desktop-file` target requires [desktop-file-utils]
to be installed.


[staged installs]: https://www.gnu.org/prep/standards/html_node/DESTDIR.html
[iconv]: https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/iconv.h.html
[syntax highlighters]: https://gitlab.com/craigbarnes/dte/tree/master/config/syntax
[releases]: https://craigbarnes.gitlab.io/dte/releases.html
[libgit issue #4343]: https://github.com/libgit2/libgit2/issues/4343
[desktop-file-utils]: https://www.freedesktop.org/wiki/Software/desktop-file-utils/
