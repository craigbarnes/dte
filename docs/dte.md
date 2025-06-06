---
NOTE: See https://craigbarnes.gitlab.io/dte/dte.html for a fully rendered version of this document
title: dte
section: 1
date: January 2025
description: A small, configurable text editor
author: [Craig Barnes, Timo Hirvonen]
seealso: ["`dterc`", "`dte-syntax`"]
---

# Synopsis

**dte**
\[**-H**]
\[**-R**|**-r** _rcfile_]
\[**-c** _command_]
\[**-t** _ctag_]
\[\[+_line_\[,_column_]] _file_]...\
**dte** \[**-h**|**-B**|**-K**|**-V**|**-b** _rcname_|**-s** _file_]

# Options

`-c` _command_
:   Run _command_, after reading the [rc file] and opening any _file_
    arguments. See [`dterc`] for available commands.

`-t` _ctag_
:   Jump to the source location of _ctag_, in the same way as the [`tag`]
    command. These options are always processed after `-c` options, so
    `-c 'tag ctag; ...'` should be used if a specific ordering is needed.

`-r` _rcfile_
:   Read configuration from _rcfile_ instead of `~/.dte/rc`.

`-s` _file_
:   Load _file_ as a [`dte-syntax`] file and exit. Any errors
    encountered are printed to `stderr` and the [exit status] is
    set appropriately.

`-b` _rcname_
:   Dump the contents of the built-in config or syntax file named
    _rcname_ and exit.

`-B`
:   Print a list of all built-in config names that can be used with the
    `-b` option and exit.

`-H`
:   Don't load [history files] at startup or save history files on
    exit (see below). History features will work as usual but will be
    in-memory only and not persisted to the filesystem.

`-R`
:   Don't read the [rc file].

`-K`
:   Start in a special mode that continuously reads input and prints the
    symbolic name of each pressed key.

`-P`
:   Print the terminal color palette to `stdout` and exit. This can be
    used as a quick, visual reference for the palette colors accepted by
    the [`hi`] command (see [`dterc`]).

`-h`
:   Display the help summary and exit.

`-V`
:   Display the version number and exit.

+_line_,_column_ arguments can be used to specify the initial cursor
position of the next _file_ argument. These can also be specified in
the format +_line_:_column_.

# Key Bindings

There are 3 editor modes, each having a different set of key bindings.
Bindings can be customized using the [`bind`] command (see [`dterc`])
or displayed using the [`show bind`] command.

The key bindings listed below are in the same format as accepted by
the [`bind`] command. In particular, key combinations are represented
as follows:

* `M-x` is Alt+x
* `C-v` (or `^V`) is Ctrl+v
* `S-left` is Shift+left
* `C-M-S-left` is Ctrl+Alt+Shift+left

## Normal Mode

Normal mode is the mode the editor starts in. Pressing basic keys
(i.e. without modifiers) simply inserts text into the buffer. There
are also various key combinations bound by default:

`S-up`, `S-down`, `S-left`, `S-right`
:   [Move] cursor and select characters

`C-S-left`, `C-S-right`
:   Move cursor and select whole words

`C-S-up`, `C-S-down`
:   Move cursor and select whole lines

`C-c`
:   [Copy] current line or selection

`C-x`
:   [Cut] current line or selection

`C-v`
:   [Paste]

`C-z`
:   [Undo]

`C-y`
:   [Redo]

`M-x`
:   Enter [command mode]

`C-f`
:   Enter [search mode]

`F3`
:   [Search] next

`F4`
:   [Search] previous

`C-t`, `C-n`
:   [Open] new buffer

`M-1`, `M-2` ... `M-9`
:   [View] buffer 1 (or 2, 3, 4, etc.)

`C-w`
:   [Close] current buffer

`C-s`
:   [Save]

`C-q`
:   [Quit]

## Command Mode

Command mode allows running various editor commands using a language
similar to Unix shell and can be accessed by pressing `M-x` (Alt+x)
in [normal mode]. The [`next`] and [`prev`] commands switch to the
next/previous file. The [`open`], [`save`] and [`quit`] commands
should be somewhat self-explanatory. For a full list of available
commands, see [`dterc`].

The key bindings for command mode are:

`up`, `down`
:   Browse previous command history.

`tab`
:   Auto-complete current command or argument

`C-a`, `home`
:   Go to beginning of command line

`C-b`, `left`
:   Move left

`C-c`, `C-g`, `Esc`
:   Exit command mode

`C-d`, `delete`
:   Delete

`C-e`, `end`
:   Go to end of command line

`C-f`, `right`
:   Move right

`C-k`, `M-delete`
:   Delete to end of command line

`C-u`
:   Delete to beginning of command line

`C-w`, `C-backspace`, `M-backspace`
:   Erase word

## Search Mode

Search mode allows entering a regular expression to search in the
current buffer and can be accessed by pressing `C-f` (Ctrl+f) in
[normal mode] or by running the [`search`] command in [command mode].

The key bindings for search mode are mostly the same as in [command mode],
plus these additional keys:

`M-c`
:   Toggle [`case-sensitive-search`] option.

`M-r`
:   Reverse search direction.

`Enter`
:   Perform [`regex`] search.

`M-Enter`
:   Perform plain-text search (escapes the regex).

# Environment

The following environment variables are inspected at startup:

`DTE_HOME`
:   User configuration directory. Defaults to `$HOME/.dte` if not set.

`HOME`
:   User home directory. Used when expanding `~/` in filenames and also
    to determine the default value for `DTE_HOME`.

`XDG_RUNTIME_DIR`
:   Directory used to store [lock files][`lock-files`]. Defaults to
    `$DTE_HOME` if not set.

`TERM`
:   Terminal identifier. Used to determine which terminal capabilities are
    supported.

`COLORTERM`
:   Enables support for 24-bit terminal colors, if set to `truecolor` or
    `24bit`.

The following environment variables affect various library routines used
by dte:

`PATH`
:   Colon-delimited list of directory prefixes, as used by [`execvp`]
    to find executables. This affects the [`exec`] and [`compile`]
    commands, but only when given a _command_ argument containing
    no slash characters (`/`).

`TZ`
:   Timezone specification, as used by [`tzset`] to initialize time
    conversion information. This affects file modification times shown
    by the [`show buffer`] command.

`LC_CTYPE`
:   [`locale`] specification for character types, as used by [`regcomp`],
    [`towlower`] and [`towupper`]. This affects the behavior of the
    [`case`] command, any command that takes a [`regex`] argument and
    any [`regex`] patterns used in [search mode].

`LANG`
:   Fallback value for `LC_CTYPE` (see [`locale`] for details).

`LC_ALL`
:   Overrides `LC_CTYPE` and/or `LANG`, if set (see [`locale`] for details).

The following environment variables are set by dte:

`DTE_VERSION`
:   Editor version string. This is set at startup to the same version
    string as shown by `dte -V | head -n1`.

`PWD`
:   Absolute path of the current working directory; set when changing
    directory with the [`cd`] command.

`OLDPWD`
:   Absolute path of the previous working directory; set when changing
    directory with the [`cd`] command and also used to determine which
    directory `cd -` switches to.

# Files

`$DTE_HOME/rc`
:   User configuration file. See [`dterc`] for a full list of available
    [commands][config-commands] and [options] or run `dte -b rc` to see
    the built-in, [default config].

`$DTE_HOME/syntax/*`
:   User syntax files. These override the [built-in syntax files] that
    come with the program. See [`dte-syntax`] for more information or
    run `dte -b syntax/dte` for a basic example.

`$DTE_HOME/file-history`
:   History of edited files and cursor positions. Used only if the
    [`file-history`] option is enabled.

`$DTE_HOME/command-history`
:   History of `dterc` commands used while in [command mode].

`$DTE_HOME/search-history`
:   History of search patterns used while in [search mode].

`$XDG_RUNTIME_DIR/dte-locks`
:   List of files currently open in a dte process (if the [`lock-files`]
    option is enabled).

# Standard Input

If [`stdin`] isn't the [controlling terminal] at startup, the editor will
attempt to read from it (until [end-of-file]) and populate a buffer with
its contents.

# Standard Output

If [`stdout`] isn't the [controlling terminal] at startup, a special
buffer will be opened for it. When the editor exits, the most recent
contents of this buffer will then be written to the corresponding file
descriptor.

If buffers are to be created for both `stdin` and `stdout` (as described
above), they will instead be merged into a single buffer that is both
populated at startup and written out at exit (see ["examples"] below).

# Standard Error

[`stderr`] is used only for diagnostic messages.

# Exit Status

The following exit codes are used by the editor, with values in
accordance with [`sysexits`]:

`0`
:   Program exited normally.

`64`
:   Command-line usage error (see ["synopsis"] above).

`65`
:   Input data error (e.g. data specified by the `-s` option).

`71`
:   Operating system error.

`74`
:   Input/output error.

`78`
:   Configuration error.

The editor itself can only exit non-zero during early startup (i.e.
because initialization failed). However, the exit code may also be
set to values in the range `0`..`125` by the [`quit`] command.

# Asynchronous Events

Signal actions configured at startup:

`SIGINT`, `SIGQUIT`, `SIGTSTP`, `SIGTTIN`, `SIGTTOU`, `SIGXFSZ`, `SIGPIPE`
:   Ignored.

`SIGUSR1`, `SIGUSR2`
:   Ignored (reserved for future use).

`SIGCHLD`, `SIGURG`, `SIGCONT`
:   Restored to the respective defaults (in case ignored actions were
    inherited; see below).

All other signals for which the [default action] is `T` (terminate)
or `A` (terminate with core dump) are caught and handled explicitly.
This is to allow the state of the terminal be cleaned up (as if
exiting normally), prior to the process being terminated.

Signals marked as "ignored" (above) are restored to their default
actions in child processes created by the [`exec`] and [`compile`]
commands, so that they inherit a clean [execution environment].

# Examples

Open `/etc/passwd` with cursor on line 3, column 8:

    dte +3:8 /etc/passwd

Run several commands at startup:

    dte -c 'set filetype sh; insert -m "#!/bin/sh\n"'

Read a buffer from [standard input](#standard-input):

    echo 'Hello, World!' | dte

Interactively filter a shell pipeline:

    echo 'A B C D E F' | tr ' ' '\n' | dte | tac

# Notes

It's advised to NOT run shell pipelines with multiple interactive
programs that try to control the terminal. For example:

    echo "Don't run this example" | dte | less

A shell will run these processes in parallel and both `dte` and `less`
will then try to control the terminal at the same time; clobbering the
input/output of both.


[`dterc`]: dterc.html
[`dte-syntax`]: dte-syntax.html
[config-commands]: dterc.html#configuration-commands
[options]: dterc.html#options
[`case-sensitive-search`]: dterc.html#case-sensitive-search
[default config]: https://gitlab.com/craigbarnes/dte/-/blob/master/config/rc
[built-in syntax files]: https://gitlab.com/craigbarnes/dte/-/tree/master/config/syntax
[rc file]: #files
[exit status]: #exit-status
[history files]: #files
[normal mode]: #normal-mode
[command mode]: #command-mode
[search mode]: #search-mode
["synopsis"]: #synopsis
["examples"]: #examples

[Close]: dterc.html#close
[Copy]: dterc.html#copy
[Cut]: dterc.html#cut
[Move]: dterc.html#movement-commands
[Open]: dterc.html#open
[Paste]: dterc.html#paste
[Quit]: dterc.html#quit
[Redo]: dterc.html#redo
[Save]: dterc.html#save
[Search]: dterc.html#search
[Undo]: dterc.html#undo
[View]: dterc.html#view
[`bind`]: dterc.html#bind
[`case`]: dterc.html#case
[`cd`]: dterc.html#cd
[`compile`]: dterc.html#compile
[`exec`]: dterc.html#exec
[`hi`]: dterc.html#hi
[`next`]: dterc.html#next
[`open`]: dterc.html#open
[`prev`]: dterc.html#prev
[`quit`]: dterc.html#quit
[`save`]: dterc.html#save
[`search`]: dterc.html#search
[`show bind`]: dterc.html#show
[`show buffer`]: dterc.html#show
[`tag`]: dterc.html#tag
[`lock-files`]: dterc.html#lock-files
[`file-history`]: dterc.html#file-history

[`sysexits`]: https://man.freebsd.org/cgi/man.cgi?query=sysexits
[`regex`]: https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap09.html#tag_09_04
[`execvp`]: https://pubs.opengroup.org/onlinepubs/9699919799/functions/exec.html
[`tzset`]: https://pubs.opengroup.org/onlinepubs/9699919799/functions/tzset.html
[`regcomp`]: https://pubs.opengroup.org/onlinepubs/9699919799/functions/regcomp.html
[`towlower`]: https://pubs.opengroup.org/onlinepubs/9699919799/functions/towlower.html
[`towupper`]: https://pubs.opengroup.org/onlinepubs/9699919799/functions/towupper.html
[`locale`]: https://man7.org/linux/man-pages/man7/locale.7.html
[`stdin`]: https://man7.org/linux/man-pages/man3/stdin.3.html#DESCRIPTION
[`stdout`]: https://man7.org/linux/man-pages/man3/stdout.3.html#DESCRIPTION
[`stderr`]: https://man7.org/linux/man-pages/man3/stderr.3.html#DESCRIPTION
[controlling terminal]: https://pubs.opengroup.org/onlinepubs/9799919799/basedefs/V1_chap11.html#tag_11_01_03
[end-of-file]: https://pubs.opengroup.org/onlinepubs/9799919799/functions/read.html#:~:text=after%20the-,end%2Dof%2Dfile,-%2C%200%20shall
[default action]: https://pubs.opengroup.org/onlinepubs/9799919799/basedefs/signal.h.html#:~:text=Signal-,Default%20Action,-Description
[execution environment]: https://pubs.opengroup.org/onlinepubs/9799919799/utilities/V3_chap02.html#tag_19_13
