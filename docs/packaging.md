Packaging
=========

Installation targets
--------------------

Running `make install` is equivalent to running the following `make`
targets:

* `install-bin`: Install `dte` binary
* `install-man`: Install man pages
* `install-bash-completion`: Install bash auto-completion script
* `install-desktop-file`: Install [desktop entry] file (excluded on macOS)
* `install-appstream`: Install [AppStream] metadata (excluded on macOS)

For more information about available `make` targets, run `make help`.

Installation variables
----------------------

The following Make variables may be useful when packaging `dte`:

* `prefix`: Top-level installation prefix (defaults to `/usr/local`).
* `bindir`: Installation prefix for program binary (defaults to
  `$prefix/bin`).
* `mandir`: Installation prefix for manual pages (defaults to
  `$prefix/share/man`).
* `bashcompletiondir`: Installation prefix for bash auto-completion
  scripts (defaults to `$prefix/share/bash-completion/completions`).
* `appdir`: Installation prefix for [desktop entry] files (defaults
  to `$prefix/share/applications`).
* `metainfodir`: Installation prefix for [AppStream] metadata files
  (defaults to `$prefix/share/metainfo`).
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


[desktop entry]: https://specifications.freedesktop.org/desktop-entry-spec/desktop-entry-spec-latest.html
[AppStream]: https://www.freedesktop.org/software/appstream/docs/
[staged installs]: https://www.gnu.org/prep/standards/html_node/DESTDIR.html
[iconv]: https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/iconv.h.html
[syntax highlighters]: https://gitlab.com/craigbarnes/dte/tree/master/config/syntax
[releases]: https://craigbarnes.gitlab.io/dte/releases.html
[libgit issue #4343]: https://github.com/libgit2/libgit2/issues/4343
