Packaging
=========

Installation targets
--------------------

Running `make install` is equivalent to running the following `make`
targets:

* `install-bin`: Install `dte` binary
* `install-man`: Install man pages
* `install-bash-completion`: Install [bash] auto-completion script
* `install-fish-completion`: Install [fish] auto-completion script
* `install-zsh-completion`: Install [zsh] auto-completion script
* `install-desktop-file`: Install [desktop entry] file (excluded on macOS/Android)
* `install-icons`: Install SVG/PNG icon files (excluded on macOS/Android)
* `install-appstream`: Install [AppStream] metadata (excluded on macOS/Android)

The last 3 targets are excluded on macOS/Android because they'd typically
just be unused clutter on those platforms. However, `make install-full`
can be used to run *all* of the above targets, regardless of platform.

For more information about available `make` targets, run `make help`.

Installation variables
----------------------

The following Make variables may be useful when packaging `dte`:

* `prefix`: Top-level installation prefix (defaults to `/usr/local`)
* `bindir`: Installation prefix for `dte` binary
* `mandir`: Installation prefix for manual pages
* `bashcompletiondir`: Installation prefix for [bash] auto-completion script
* `fishcompletiondir`: Installation prefix for [fish] auto-completion script
* `zshcompletiondir`: Installation prefix for [zsh] auto-completion script
* `appdir`: Installation prefix for [desktop entry] file
* `metainfodir`: Installation prefix for [AppStream] metadata file
* `DESTDIR`: Standard variable used for [staged installs]
* `V=1`: Enable verbose build output

These are defined in accordance with [section 7.2.5] of the
[GNU Coding Standards] and the full list (along with default values)
can be found at the top of [`GNUmakefile`].

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
[`GNUmakefile`] and *must* be valid GNU make syntax. This can be considered
a stable aspect of the build system and will *not* be renamed or removed
unless the major version number is bumped.

Stable release tarballs
-----------------------

The [releases] page contains a short summary of changes for each
stable version and links to the corresponding source tarballs.

Note: auto-generated tarballs from GitHub/GitLab can (and
[do][libgit issue #4343]) change over time and cannot be guaranteed to
have long-term stable checksums. Use the tarballs from the [releases]
page, unless you're prepared to deal with future checksum failures.


[`GNUmakefile`]: ../GNUmakefile
[syntax highlighters]: ../config/syntax
[desktop entry]: https://specifications.freedesktop.org/desktop-entry-spec/desktop-entry-spec-latest.html
[AppStream]: https://www.freedesktop.org/software/appstream/docs/
[bash]: https://www.gnu.org/software/bash/
[fish]: https://fishshell.com
[zsh]: https://www.zsh.org
[staged installs]: https://www.gnu.org/prep/standards/html_node/DESTDIR.html
[section 7.2.5]: https://www.gnu.org/prep/standards/html_node/Directory-Variables.html
[GNU Coding Standards]: https://www.gnu.org/prep/standards/html_node/index.html
[iconv]: https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/iconv.h.html
[releases]: https://craigbarnes.gitlab.io/dte/releases.html
[libgit issue #4343]: https://github.com/libgit2/libgit2/issues/4343
