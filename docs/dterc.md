---
NOTE: See https://craigbarnes.gitlab.io/dte/dterc.html for a fully rendered version of this document
title: dterc
section: 5
date: March 2025
description: Command and configuration language used by dte
author: [Craig Barnes, Timo Hirvonen]
seealso: ["`dte`", "`dte-syntax`"]
---

# dterc

dterc is the language used in `dte` [configuration files][] (`~/.dte/rc`)
and also in the [command mode] of the editor (Alt+x). The syntax of the
language is quite similar to shell, but much simpler.

Commands are separated either by a newline or `;` character. To make a
command span multiple lines in an rc file, escape the newline (put `\`
at the end of the line).

Rc files can contain comments at the start of a line. Comments begin
with a `#` character and can be indented, but they can't be put on the
same line as a command.

Commands can contain environment variables. Variables always expand into
a single argument even if they contain whitespace. Variables inside
single or double quotes are NOT expanded. This makes it possible to bind
keys to commands that contain variables (inside single or double
quotes), which will be expanded just before the command is executed.

Example:

    alias x "exec -s chmod 755 $FILE"

`$FILE` is expanded when the alias _x_ is executed. The command works even
if `$FILE` contains whitespace.

## Special variables

These variables are always defined and override environment variables of
the same name.

### **$FILE**

The absolute filename of the current buffer (or an empty string if unsaved).

### **$RFILE**

The relative filename of the current buffer (or an empty string if unsaved).

### **$FILEDIR**

The directory part of `$FILE`.

### **$RFILEDIR**

The directory part of `$RFILE`, or `.`.

### **$FILETYPE**

The value of the [`filetype`] option for the current buffer.

### **$LINENO**

The line number of the cursor in the current buffer.

### **$COLNO**

The column number of the cursor in the current buffer.

### **$WORD**

The selected text or the word under the cursor.

### **$DTE_HOME**

The user configuration directory. This is either the value of [`$DTE_HOME`]
when the editor first started, or the default value (`$HOME/.dte`).

## Single quoted strings

Single quoted strings can't contain single quotes or escaped characters.

## Double quoted strings

Double quoted strings may contain the following escapes:

`\a`, `\b`, `\t`, `\n`, `\v`, `\f`, `\r`
:   Control characters (same [as in C][`ascii(7)`])

`\e`
:   Escape character

`\\`
:   Backslash

`\"`
:   Double quote

`\x0a`
:   Hexadecimal byte value 0x0A (note that `\x00` is not supported,
    because strings are null-terminated)

`\u20ac`
:   Four hex digit Unicode code point U+20AC

`\U000020ac`
:   Eight hex digit Unicode code point U+20AC

# Commands

## Configuration Commands

Configuration commands are used to customize certain aspects of the
editor, for example adding [key bindings], setting [options], etc.
These are the only commands allowed in user [configuration files].

### **alias** _name_ [_command_]

Create an alias _name_ for _command_. If no _command_ is given then any
existing alias for _name_ is removed.

Aliases can be used in [command mode] or [bound][`bind`] to keys, just
as normal commands can. When aliases are used in place of commands, they
are first recursively expanded (to allow aliases of aliases) and any
additional arguments are then added to the end of the expanded command.

For example, if the following alias is created:

    alias read 'pipe-from cat'

this can then be invoked as e.g. `read file.txt`, which will expand to
[`pipe-from cat file.txt`] and thus cause `file.txt` to be inserted into
the current buffer.

### **bind** [**-cns**] _key_ [_command_]

Bind _command_ to _key_. If no _command_ is given then any existing
binding for _key_ is removed.

Special keys:

* `left`
* `right`
* `up`
* `down`
* `insert`
* `delete`
* `home`
* `end`
* `pgup`
* `pgdown`
* `escape`
* `backspace`
* `enter`
* `tab`
* `space`
* `begin` (keypad "5" with Num Lock off)
* `F1`..`F20`

Modifiers:

Ctrl:
:   `C-x` (or `^X`)

Alt:
:   `M-x`

Shift:
:   `S-left`

Super:
:   `s-left`

Hyper:
:   `H-left`

The _key_ is bound in [normal mode] by default, unless one or more of the
following flags are used:

`-c`
:   Add binding for [command mode]

`-n`
:   Add binding for [normal mode]

`-s`
:   Add binding for [search mode]

The commands available in normal mode are the ones listed in the main
sections of this manual.

The commands available in command/search mode are as follows:

* `left`
* `right`
* `word-bwd`
* `word-fwd`
* `bol`
* `eol`
* `delete`
* `delete-word`
* `delete-eol`
* `erase`
* `erase-word`
* `erase-bol`
* `clear`
* `copy` [**-bip**]
* `paste` [**-m**]
* `toggle` [**-g**] _option_ [_values_]...
* `accept` [**-eH**] - Execute command or perform search
* `cancel` - Return to normal mode
* `history-next` [**-S**] - Find next history item matching current prefix
* `history-prev` [**-S**] - Find previous history item matching current prefix
* `complete-next` - Select next auto-completion in command mode
* `complete-prev` - Select previous auto-completion in command mode
* `direction` - Toggle search direction in search mode

Most of these commands behave in a similar fashion to the normal mode
commands of the same name. The exceptions to this have been given a
short description above. Most of the command flags also behave similarly
to the normal mode equivalents, except for the following:

* `accept -H` - Accept the current text without adding a history entry
* `accept -e` (search mode only) - Escape all [`regex`] special
  characters, before performing a (plain-text) search
* `history-next -S` - Get next history item (without prefix matching)
* `history-prev -S` - Get previous history item (without prefix matching)

Note that, due to the use of several input protocols, some key
combinations may correspond to slightly different _key_ strings,
depending on which terminal is in use. To see the appropriate _key_
string for a specific key combination, start the editor in [`dte -K`]
mode. In some cases this may mean that the same key binding has to be
added in 2 different forms (e.g. `C-S-1` and `C-S-!`) and in other cases
it may mean certain keys cannot be bound at all. Terminals that support
[Kitty's keyboard protocol] should be preferred (if possible), for
maximum functionality.

See also:

* The [key bindings] section in the [`dte`] man page
* The [`-K`][`dte -K`] option in the [`dte`] man page
* The [`show bind`] command (below)
* The [output][`binding/default`] of `dte -b binding/default`
* [Kitty's keyboard protocol] documentation (if seeking technical details)

### **set** [**-gl**] _option_ [_value_] ...

Set _option_ to the specified _value_.

For boolean options, an omitted _value_ is equivalent to `true`. Multiple
options can be set at once, but then a _value_ must be given for every
_option_.

There are three kinds of [options]:

1. [Global options][global options].

2. [Local options][local options]. These are file specific options.
   Each open file has its own copies of the option values.

3. [Options that have both global and local values][common options].
   The Global value is just a default local value for opened files
   and is never used for anything else. Changing the global value
   does not affect any already opened files.

Both global and local values are set by default (as applicable), except
in [configuration files], where only global values can be set (no need
to specify the `-g` flag).

`-g`
:   Change only global option values

`-l`
:   Change only local option values (of the current file)

Examples:

    set indent-width 4
    set expand-tab true
    set emulate-tab true
    set ws-error auto-indent,trailing,special
    set set-window-title
    set editorconfig

See also: [`toggle`], [`option`] and [`show set`] commands.

### **setenv** _name_ [_value_]

Set (or unset) environment variable.

### **hi** [**-c**] [_name_] [_fg-color_ [_bg-color_]] [_attribute_]...

Set highlight color.

The _name_ argument can be a token name defined by a `dte-syntax` file
or one of the following, built-in highlight names:

* `default`
* `nontext`
* `noline`
* `wserror`
* `selection`
* `currentline`
* `linenumber`
* `statusline`
* `commandline`
* `errormsg`
* `infomsg`
* `tabbar`
* `activetab`
* `inactivetab`
* `dialog`

The _fg-color_ and _bg-color_ arguments can be one of the following:

* No value (equivalent to `default`)
* A numeric value between `-2` and `255`
* A 256-color palette value in R/G/B notation (e.g. `0/3/5`)
* A true color value in #RRGGBB notation (e.g. `#ab90df`)
* A true color value in #RGB notation (e.g. `#1ef`)
* `keep` (`-2`)
* `default` (`-1`)
* `black` (`0`)
* `red` (`1`)
* `green` (`2`)
* `yellow` (`3`)
* `blue` (`4`)
* `magenta` (`5`)
* `cyan` (`6`)
* `gray` (`7`)
* `darkgray` (`8`)
* `lightred` (`9`)
* `lightgreen` (`10`)
* `lightyellow` (`11`)
* `lightblue` (`12`)
* `lightmagenta` (`13`)
* `lightcyan` (`14`)
* `white` (`15`)

Colors `16` to `231` correspond to R/G/B colors and `232` to `255` are
grayscale values (see https://www.ditig.com/256-colors-cheat-sheet for
more details).

If the terminal has limited support for rendering colors, the _fg-color_
and _bg-color_ arguments will fall back to the nearest supported color
(unless the `-c` flag is used; see below).

The _attribute_ argument(s) can be any combination of the following:

* `bold`
* `dim`
* `italic`
* `underline`
* `strikethrough`
* `blink`
* `reverse`
* `invisible`
* `keep`

The color and attribute value `keep` is useful in selected text
to keep _fg-color_ and attributes and change only _bg-color_.

NOTE: Because `keep` is both a color and an attribute you need to
specify both _fg-color_ and _bg-color_ if you want to set the `keep`
_attribute_.

Unset fg/bg colors are inherited from highlight color `default`.
If you don't set fg/bg for the highlight color `default` then
terminal's default fg/bg is used.

If `hi` is run without any arguments, all highlight colors are
removed and a baseline set of defaults is then loaded (as if by
running [`include -b`] on the built-in [`color/reset`] config).

`-c`
:   Do nothing at all if the terminal can't display _fg-color_ and/or
    _bg-color_ with full precision (this can be used to set up tiered
    color schemes that use the best supported colors automatically)

### **ft** [**-b**|**-c**|**-f**|**-i**] _filetype_ _string_...

Add a filetype association. Filetypes are used to determine which
syntax highlighter and local options to use when opening files.

By default _string_ is interpreted as one or more filename extensions.

`-b`
:   Interpret _string_ as a file basename

`-c`
:   Interpret _string_ as a [`regex`] pattern and match against the
    contents of the first line of the file

`-f`
:   Interpret _string_ as a [`regex`] pattern and match against the
    full (absolute) filename

`-i`
:   Interpret _string_ as a command interpreter name and match against
    the Unix shebang line (after removing any path prefix and/or version
    suffix)

Examples:

    ft c c h
    ft -b make Makefile GNUmakefile
    ft -c xml '<\?xml'
    ft -f mail '/tmpmsg-.*\.txt$'
    ft -i lua lua luajit

See also:

* The [`option`] command (below)
* The [`filetype`] option (below)
* The [`dte-syntax`] man page

### **option** [**-r**] _filetype_ _option_ _value_...

Add automatic _option_ for _filetype_ (as previously registered
with the [`ft`] command). Automatic options are set when files are
are opened.

`-r`
:   Interpret _filetype_ argument as a [`regex`] pattern instead of a
    filetype and match against full filenames

### **include** [**-bq**] _file_

Read and execute commands from _file_.

`-b`
:   Read built-in _file_ instead of reading from the filesystem

`-q`
:   Don't show an error message if _file_ doesn't exist

Note: "built-in files" are config files bundled into the program binary.
See the `-B` and `-b` flags in the [`dte`] man page and the [`show include`]
command for more information.

### **errorfmt** [**-i**] _compiler_ [_regexp_] [file|line|column|message|_]...

Register a [`regex`] pattern, for later use with the [`compile`] command.

When the `compile` command is invoked with a specific _compiler_ name,
the _regexp_ pattern(s) previously registered with that name are used to
parse messages from it's program output.

The _regexp_ pattern should contain as many capture groups as there are
extra arguments. These capture groups are used to parse the file, line,
message, etc. from the output and, if possible, jump to the corresponding
file position. To use parentheses in _regexp_ but ignore the capture, use
`_` as the extra argument.

Running `errorfmt` multiple times with the same _compiler_ name appends
each _regexp_ to a list. When running `compile`, the entries in the
specified list are checked for a match in the same order they were added.

If only 1 argument (i.e. _compiler_) is given, all patterns previously
added for that compiler name will be removed.

For a basic example of usage, see the [output][`compiler/go`] of
`dte -b compiler/go`.

`-i`
:   Ignore this error

## Editor Commands

### **quit** [**-f**|**-p**] [**-CSFH**] [_exitcode_]

Quit the editor.

The exit status of the process is set to _exitcode_, which can be
in the range `0`..`125`, or defaults to `0` if unspecified.

`-f`
:   Force quit, even if there are unsaved files

`-p`
:   Prompt for confirmation if there are unsaved files

`-C`
:   Don't write [`command`][] [history file]

`-S`
:   Don't write [`search`][] [history file]

`-F`
:   Don't write file [history file]

`-H`
:   Don't write any history files (equivalent to `-CSF`)

See also:

* The `dte` [`-H`][`dte -H`] flag
* The [`file-history`] option

### **suspend**

Suspend the editor (run `fg` in the shell to resume).

### **cd** _directory_

Change the working directory and update `$PWD` and `$OLDPWD`. Running
`cd -` changes to the previous directory (`$OLDPWD`).

### **command** [_text_]

Enter [command mode]. If _text_ is given then it is written to the command
line (see the default `^L` key binding for why this is useful).

### **refresh**

Trigger a full redraw of the screen.

## Buffer Management Commands

### **open** [**-g**|**-t**] [**-e** _encoding_] [_filename_]...

Open file. If _filename_ is omitted, a new file is opened.

`-e` _encoding_
:   Set file _encoding_ (see `iconv -l` for list of supported encodings)

`-g`
:   Perform [`glob`] expansion on _filename_

`-t`
:   Mark buffer as "temporary" (always closeable, without warnings for
    "unsaved changes")

### **save** [**-fp**] [**-d**|**-u**] [**-b**|**-B**] [**-e** _encoding_] [_filename_]

Save current buffer.

`-b`
:   Write byte order mark (BOM)

`-B`
:   Don't write byte order mark

`-d`
:   Save with DOS/CRLF line-endings

`-f`
:   Force saving read-only file

`-u`
:   Save with Unix/LF line-endings

`-p`
:   Open a command prompt if there's no specified or existing _filename_

`-e` _encoding_
:   Set file _encoding_ (see `iconv -l` for list of supported encodings)

See also: [`newline`] and [`utf8-bom`] global options

### **close** [**-qw**] [**-f**|**-p**]

Close file.

`-f`
:   Force close file, even if it has unsaved changes

`-p`
:   Prompt for confirmation if the file has unsaved changes

`-q`
:   Quit if closing the last open file

`-w`
:   Close parent window if closing its last contained file

### **next**

Display next file.

### **prev**

Display previous file.

### **view** _N_|**last**

Display *N*th or last open file.

### **move-tab** _N_|**left**|**right**

Move current tab to position _N_ or 1 position left or right.

## Window Management Commands

### **wsplit** [**-bghr**] [**-n**|**-t**|_filename_...]

Split the current window.

_filename_ arguments will be opened in a manner similar to the [`open`]
command. If there are no _filename_ arguments, the contents of the new
window will be an additional view of the current buffer.

`-b`
:   Add new window before current instead of after

`-g`
:   Perform [`glob`] expansion on _filename_

`-h`
:   Split horizontally instead of vertically

`-n`
:   Create a new, empty buffer

`-r`
:   Split root instead of current window

`-t`
:   Create a new, empty buffer and mark it as "temporary" (always
    closeable, without warnings for "unsaved changes")

### **wclose** [**-f**|**-p**]

Close window.

`-f`
:   Force close window, even if it contains unsaved files

`-p`
:   Prompt for confirmation if there are unsaved files in the window

### **wnext**

Next window.

### **wprev**

Previous window.

### **wresize** [**-h**|**-v**] [_N_|+_N_|-- -_N_]

If no parameter given, equalize window sizes in current frame.

`-h`
:   Resize horizontally

`-v`
:   Resize vertically

_N_
:   Set size of current window to _N_ columns/rows

`+`_N_
:   Increase size of current window by _N_ columns/rows

`-`_N_
:   Decrease size of current window by _N_ columns/rows (use `--` to
    prevent the minus symbol being parsed as an option flag, e.g.
    `wresize -- -5`)

### **wflip**

Change from vertical layout to horizontal and vice versa.

### **wswap**

Swap positions of this and next frame.

## Movement Commands

Movement commands are used to move the cursor position.

Several of these commands also have `-c` and `-l` flags to allow
creating character/line selections. These 2 flags are noted in the
command summaries below, but are only described once, as follows:

`-c`
:   Select characters

`-l`
:   Select whole lines

### **left** [**-c**|**-l**]

Move one column left.

### **right** [**-c**|**-l**]

Move one column right.

### **up** [**-c**|**-l**]

Move one line up.

### **down** [**-c**|**-l**]

Move one line down.

### **pgup** [**-c**|**-l**]

Move one page up.

### **pgdown** [**-c**|**-l**]

Move one page down.

### **blkup** [**-c**|**-l**]

Move one block up.

Note: a "block", in this context, is somewhat akin to a paragraph.
Blocks are delimited by one or more blank lines

### **blkdown** [**-c**|**-l**]

Move one block down.

### **word-fwd** [**-c**|**-l**] [**-s**]

Move forward one word.

`-s`
:   Skip special characters

### **word-bwd** [**-c**|**-l**] [**-s**]

Move backward one word.

`-s`
:   Skip special characters

### **bol** [**-c**|**-l**] [**-s**|**-t**]

Move to beginning of current line.

`-s`
:   Move to beginning of indented text or beginning of line, depending
    on current cursor position

`-t`
:   Like `-s`, but with the additional behavior of moving back and forth
    between the two positions

### **eol** [**-c**|**-l**]

Move to end of current line.

### **bof** [**-c**|**-l**]

Move to beginning of file.

### **eof** [**-c**|**-l**]

Move to end of file.

### **bolsf** [**-c**|**-l**]

Incrementally move to beginning of line, then beginning of screen, then
beginning of file.

### **eolsf** [**-c**|**-l**]

Incrementally move to end of line, then end of screen, then end of file.

### **scroll-up**

Scroll view up one line. Keeps cursor position unchanged if possible.

### **scroll-down**

Scroll view down one line. Keeps cursor position unchanged if possible.

### **scroll-pgup**

Scroll one page up. Cursor position relative to top of screen is
maintained. See also [`pgup`].

### **scroll-pgdown**

Scroll one page down. Cursor position relative to top of screen is
maintained. See also [`pgdown`].

### **center-view**

Center view to cursor.

### **match-bracket** [**-c**|**-l**]

Move to the bracket character paired with the one under the cursor.
The character under the cursor should be one of `{}[]()<>`.

### **search** [**-Her**] [**-n**|**-p**|**-w**|_pattern_]

If no flags (or just `-r` and no _pattern_) are given then dte changes to
[search mode], where you can type a [regular expression][`regex`] to search.

`-H`
:   Don't add _pattern_ to search history

`-e`
:   Escape [`regex`] special characters in _pattern_, so that the search
    becomes effectively "plain text"

`-r`
:   Start searching backwards

`-n`
:   Search next

`-p`
:   Search previous

`-w`
:   Search word under cursor

### **line** [**-c**|**-l**] _lineno_[`,`_colno_]

Move the cursor to the line number specified by _lineno_ and
(optionally) the column number specified by _colno_. The delimiter
between the two numbers can either be a comma (`,`) or a colon (`:`).

Examples:

    line 19
    line 20,15
    line 20:15

### **bookmark** [**-r**]

Save the current file/cursor location to a stack.

`-r`
:   Jump back to the previous location (and pop it off the stack)

### **tag** [**-r**|_tag_...]

Save the current file/cursor location to a stack and jump to the
location of _tag_. The location for _tag_ is determined by parsing a
[`tags`] file (as generated by [`ctags`]) from the current directory
or any of its parent directories.

If multiple tags are found, either because multiple tags exists with the
same name or multiple _tag_ arguments are given, the first location will
be jumped to and the rest can be navigated with the [`msg`] command.

If no _tag_ arguments are given, the word under the cursor is used
instead (unless `-r` is used).

`-r`
:   Jump back to the previous location (and pop it off the stack)

Note that the saving of the cursor location described above is much the
same as running [`bookmark`] and `tag -r` is identical to `bookmark -r`.

### **msg** [**-n**|**-p**|_number_]

Display and/or navigate messages, as generated by the [`compile`]
and [`tag`] commands. If the activated message has an associated
file location, the file will be opened and the cursor moved to
the appropriate position.

Messages are displayed in the bottom row of the screen (i.e. the
[`command`] line) and thus truncated to the width of the terminal.
The [`show msg`] command can be used to display a numbered list of
messages, without any truncation and with the cursor placed on the
current message.

If this command is used without flags or arguments (e.g. as `msg`)
the current message will be re-displayed, which can be useful after
other input clears it.

`-n`
:   Next message

`-p`
:   Previous message

## Editing Commands

### **cut**

Cut current line or selection.

### **copy** [**-bikp**]

Copy current line or selection.

`-b`
:   Copy to system clipboard

`-i`
:   Copy to internal copy buffer (this is the default, if no **-bip**
    flags are used)

`-k`
:   Keep selection (by default, selections are lost after copying)

`-p`
:   Copy to system "primary selection"

Note that the **-b** and **-p** flags depend upon the terminal
supporting [OSC 52] escape sequences. If the terminal lacks this
support, these flags will simply do nothing. OSC 52 sends data over
the wire, so it can be used over SSH and still work as expected,
unlike most other methods of copying to the system clipboard.

The **-i**, **-b** and **-p** flags can be used together, to allow
copying to multiple targets in a single command. For example:

    copy -ib

### **paste** [**-m**] [**-a**|**-c**]

Insert text previously copied by the [`copy`] or [`cut`] commands.

If the text to be inserted was copied from a whole-line selection
(e.g. `down -l; copy`) or as a whole, single line (e.g. `unselect; copy`)
the default behaviour is to insert the text at the start of the line
below the cursor.

`-a`
:   Paste above the cursor (instead of below), if pasting whole lines

`-c`
:   Always paste directly at the cursor position, even when pasting
    whole lines (instead of below/above the cursor)

`-m`
:   Move after the pasted text

### **undo** [**-m**]

Undo latest change.

`-m`
:   Move to the change location, without undoing it

### **redo** [_choice_]

Redo changes done by the [`undo`] command. If there are multiple
possibilities a message is displayed:

    Redoing newest (2) of 2 possible changes.

If the change was not the one you wanted, just run `undo` and
then, for example, `redo 1`.

### **clear**

Clear current line.

`-i`
:   Do not [`auto-indent`] the line after clearing

### **join** [_delimiter_]

Join selection or next line to current using _delimiter_. If _delimiter_
is not provided, a space is used.

### **new-line** [**-a**]

Insert empty line below current line.

`-a`
:   Insert above current line, instead of below

### **delete**

Delete character after cursor (or selection).

### **erase**

Delete character before cursor (or selection).

### **delete-eol** [**-n**]

Delete to end of line.

`-n`
:   Delete newline if cursor is at end of line

### **erase-bol**

Erase to beginning of line.

### **delete-word** [**-s**]

Delete word after cursor.

`-s`
:   Be more "aggressive"

### **erase-word** [**-s**]

Erase word before cursor.

`-s`
:   Be more "aggressive"

### **delete-line** [**-S**]

Delete whole lines touched by the selection, or the current line.

`-S`
:   If there's a character-wise selection, only delete the selected
    characters (instead of treating it like a line selection)

### **case** [**-l**|**-u**]

Change text case. The default is to change lower case to upper case and
vice versa.

`-l`
:   Lower case

`-u`
:   Upper case

### **insert** [**-k**|**-m**] _text_

Insert _text_ into the buffer.

`-k`
:   Insert one character at a time, as if manually typed (normally
    _text_ is inserted exactly as specified, but this option allows
    it to be affected by special input handling like auto-indents,
    whitespace trimming, line-by-line undo, etc.)

`-m`
:   Move after inserted text

### **replace** [**-bcegi**] _pattern_ [_replacement_]

Replace all instances of text matching _pattern_ with the _replacement_
text. Matching is confined to the current selection, if there is one.

The _pattern_ argument is a POSIX extended [`regex`].

The _replacement_ argument is treated like a template and may contain
several, special substitutions:

* Backslash sequences `\1` through `\9` are replaced by sub-strings
  that were "captured" (via parentheses) in the _pattern_.
* The special character `&` is replaced by the full string that was
  matched by _pattern_.
* Literal `\` and `&` characters can be inserted in _replacement_
  by escaping them (as `\\` and `\&`).
* All other characters in _replacement_ represent themselves.

Note: extra care must be taken when using [double quotes] for the
_pattern_ argument, since double quoted arguments have their own
(higher precedence) backslash sequences.

`-b`
:   Use basic (instead of extended) regex syntax

`-c`
:   Ask for confirmation before each replacement

`-e`
:   Escape [`regex`] special characters in _pattern_, so that it's
    matched literally (i.e. as "plain text")

`-g`
:   Replace all matches for each line (instead of just the first)

`-i`
:   Ignore case

Examples:

    replace ^ #
    replace 'Hello World' '& (Hallo Welt)'
    replace "[ \t]+$" ''
    replace -cg '([^ ]+) +([^ ]+)' '\2 \1'

### **shift** _count_

Shift current or selected lines by _count_ indentation levels.
Count is usually `-1` (decrease indent) or `1` (increase indent).

To specify a negative number, it's necessary to first disable
option parsing with `--`, e.g. `shift -- -1`.

### **wrap-paragraph** [_width_]

Format the current selection or paragraph under the cursor. If
paragraph _width_ is not given then the [`text-width`] option is
used.

This command merges the selection into one paragraph. To format
multiple paragraphs use the external `fmt` program with the
[`filter`] command, e.g. `filter fmt -w 60`.

### **macro** _action_

Record and replay command macros.

The _action_ argument can be one of:

`record`
:   Begin recording

`stop`
:   Stop recording

`toggle`
:   Toggle recording on/off

`cancel`
:   Stop recording, without overwriting the previous macro

`play`
:   Replay the previously recorded macro

Once a macro has been recorded, it can be viewed in text form
by running [`show macro`].

### **select** [**-kl**]

Enter selection mode. All basic [movement commands] while in this mode
extend the selected area, until either the [`unselect`] command is used
(e.g. by pressing `Esc`) or some other operation (e.g. [`delete`],
[`insert`], etc.) clears the selection.

Note: A better way to create selections is to hold the Shift key while
moving the cursor. The `select` command exists mostly as a fallback,
for terminals with limited key binding support.

`-k`
:   Keep existing selections

`-l`
:   Select whole lines

### **unselect**

Cancel selection.

## External Commands

### **exec** [**-pstmn**] [**-ioe** _action_]... _command_ [_argument_]...

Execute external _command_, with custom actions for [standard streams].
The `-i`, `-o` and `-e` options represent standard input, output and
error respectively and each one can be given a specific _action_, as
described below.

The following _action_ arguments are supported by all **-ioe** options:

* `null` - redirect to `/dev/null`
* `tty` - redirect to controlling terminal (default action)

Actions for stdin (`-i`):

* `buffer` - pipe selected text or whole buffer (to _command_)
* `line` - pipe selected text or current line
* `word` - pipe selected text or current word
* `command` - pipe command history
* `search` - pipe search history
* `msg` - pipe list of numbered messages (see [`msg`] command)

Actions for stdout (`-o`):

* `buffer` - [`insert`] output (from _command_) into buffer
* `eval` - execute output as dterc commands
* `msg` - run [`msg`] command with numerical argument parsed from first
  line of output
* `open` - run [`open`] command with each line of output as an argument
* `tag` - run [`tag`] command with first line of output as an argument

Actions for stderr (`-e`):

* `errmsg` - if _command_ exits non-zero, display first line of stderr
  output as an error message

`-i` _action_
:   Specify standard input action

`-o` _action_
:   Specify standard output action

`-e` _action_
:   Specify standard error action

`-p`
:   Display "press any key to continue" prompt

`-s`
:   Don't yield terminal control to child processes and transparently
    convert `tty` actions to `null` (use this to avoid screen flicker
    when _command_ doesn't need terminal access)

`-t`
:   Cancel the effects of `-s` (last one wins)

`-m`
:   Move cursor after inserted text (if any)

`-n`
:   Strip newline from end of inserted text (if any)

For convenience, there are several built-in aliases to simplify common
uses of `exec`:

    alias filter 'exec -s -i buffer -o buffer -e errmsg'
    alias pipe-from 'exec -s -o buffer -e errmsg'
    alias pipe-to 'exec -s -i buffer -e errmsg'
    alias run 'exec'
    alias eval 'exec -o eval'
    alias exec-open 'exec -o open'
    alias exec-tag 'exec -o tag'
    alias exec-msg 'exec -o msg -i msg'

Examples:

    filter sort -r
    filter sh -c 'tr a-z A-Z | sed s/foo/bar/'
    pipe-to xsel -b
    exec-open -s find . -type f -name *.h
    exec-open -s git ls-files --modified
    exec-open fzf -m --reverse
    exec-tag -s echo main
    exec-tag sh -c 'readtags -l | cut -f1 | sort | uniq | fzf --reverse'

When passing the buffer through a `filter` command, the cursor is
moved to line 1 and the whole contents is replaced with the output.
It may be useful to save and restore the cursor position, in cases
where line numbers remain mostly unchanged. This can be done by
wrapping the command with [`bookmark`] and [`bookmark -r`], e.g.:

    bookmark; filter expand --tabs=4; bookmark -r

Note that _command_ is executed directly using [`execvp`]. To use shell
features like pipes or redirection, use a shell interpreter as the
_command_ (see second example above). If complex commands become
difficult to read (e.g. due to nested/escaped quotes), it's recommended
to create [external scripts] and execute those instead.

### **compile** [**-1ps**] _errorfmt_ _command_ [_argument_]...

Run external _command_ and collect output messages. This can be
used to run e.g. compilers, build systems, code search utilities,
etc. and then jump to a file/line position for each message.

The _errorfmt_ argument corresponds to a regex capture pattern
previously specified by the [`errorfmt`] command. After _command_
exits successfully, parsed messages can be navigated using the
[`msg`] command.

`-1`
:   Read error messages from stdout instead of stderr

`-p`
:   Display "Press any key to continue" prompt

`-s`
:   Don't echo command output to the terminal during execution;
    just silently collect messages (use this to avoid screen
    flicker, e.g. for commands that typically complete quickly)

## Other Commands

### **repeat** _count_ _command_ [_argument_]...

Run _command_ _count_ times.

### **toggle** [**-gv**] _option_ [_values_]...

Toggle _option_ between several _values_.

If no _values_ are specified and the [option][options] is of enum or
boolean type, toggle between all enumerated values. If _option_ has
both [local and global][common options] values then the local value
is toggled, by default.

`-g`
:   Toggle global values, instead of local

`-v`
:   Display new value

Examples:

    toggle -v show-line-numbers
    toggle -v display-special
    toggle -v text-width 60 70 80

See also: [`set`] command.

### **show** [**-c**] _type_ [_key_]

Display current values for various configurable types.

The _type_ argument can be one of:

`alias`
:   Show [command aliases][`alias`]

`bind`
:   Show [key bindings][`bind`]

`color`
:   Show [highlight colors][`hi`]

`command`
:   Show [command history][`command`]

`env`
:   Show environment variables

`errorfmt`
:   Show [compiler error formats][`errorfmt`]

`ft`
:   Show [filetype associations][`ft`]

`hi`
:   Show [highlight colors][`hi`]

`include`
:   Show [built-in configs][`include`]

`macro`
:   Show last recorded [macro][`macro`]

`msg`
:   Show [messages][`msg`]

`option`
:   Show [option values](#options) and automatic [`option`] entries

`search`
:   Show [search history][`search`]

`set`
:   Show [option values](#options)

`setenv`
:   Show environment variables, in terms of the [`setenv`] command

`wsplit`
:   Show [window dimensions][`wsplit`]

The _key_ argument is the name of the entry to look up (e.g. the alias
name). If this argument is omitted, the full list of entries of the
specified _type_ will be displayed in a new, temporary buffer (created
as if by [`open -t`]).

Note that the majority of _type_ arguments correspond to a [command] of
the same name and some _type_ arguments don't take any _key_ argument.

`-c`
:   write output to the command line (for most _type_ arguments) or to
    the current buffer (when _type_ is `errorfmt` or `include`)

# Options

Options can be changed using the [`set`] command. Enumerated options can
also be [`toggle`]d. To see which options are enumerated, type "toggle "
in [command mode] and press the tab key. You can also use the [`option`]
command to set default options for specific file types.

## Global options

### **case-sensitive-search** [true]

`false`
:   Search is case-insensitive

`true`
:   Search is case-sensitive

`auto`
:   If search string contains an uppercase letter search is
    case-sensitive, otherwise it is case-insensitive

### **display-special** [false]

Display special characters.

### **esc-timeout** [100] 0...2000

When single escape is read from the terminal dte waits some
time before treating the escape as a single keypress. The
timeout value is in milliseconds.

Too long timeout makes escape key feel slow and too small
timeout can cause escape sequences of for example arrow keys to
be split and treated as multiple key presses.

### **filesize-limit** [250]

Refuse to open any file with a size larger than this value (in
mebibytes). Useful to prevent accidentally opening very large
files, which can take a long time on some systems.

### **lock-files** [true]

Keep a record of open files, so that a warning can be shown if the
same file is accidentally opened in multiple dte processes.

See also: the `FILES` section in the [`dte`] man page.

### **newline** [unix]

Whether to use LF (**unix**) or CRLF (**dos**) line-endings in newly
created files.

Note: buffers opened from existing files will have their newline
type detected automatically.

### **optimize-true-color** [true]

If set to `true`, this option will cause the [`hi`] command to
automatically replace 24-bit #RRGGBB colors with palette colors 16-255,
but only if there's an exact color match among the default, extended
palette colors.

This allows defining color schemes in #RRGGBB notation while still
sending the shortest possible escape sequence to the terminal.

Note: this optimization only works if the terminal has not been
configured with custom values for colors 16-255. If you have changed
these extended palette colors, you should set this option to `false`.

### **select-cursor-char** [true]

Whether to include the character under the cursor in selections.

### **scroll-margin** [0]

Minimum number of lines to keep visible above/below the cursor.

Note that this minimum can only be honored if the window has enough
lines to accommodate both margins (upper and lower), plus 1 non-margin
line. If this condition isn't satisfied, it will be adjusted down to
the greatest value the window can accommodate.

### **set-window-title** [false]

Set the window title to the filename of the current buffer (if the
terminal supports it).

### **show-line-numbers** [false]

Show line numbers.

### **statusline-left** [" %f%s%m%s%r%s%M"]

Format string for the left aligned part of status line.

`%f`
:   Filename

`%m`
:   Prints `*` if file has been modified since last save

`%r`
:   Prints `RO` for read-only buffers or `TMP` for temporary buffers

`%y`
:   Cursor row

`%Y`
:   Total rows in file

`%x`
:   Cursor display column

`%X`
:   Cursor column as characters (if this differs from cursor display
    column then both are shown, as e.g. `2-9`)

`%p`
:   Position in percentage

`%E`
:   File encoding

`%b`
:   Prints `BOM` if file has a byte order mark

`%M`
:   Miscellaneous status information

`%n`
:   Line-ending (`LF` or `CRLF`)

`%N`
:   Line-ending (only if `CRLF`)

`%s`
:   Separator (a single space, unless the preceding format character
    expanded to an empty string)

`%S`
:   Like `%s`, but 3 spaces instead of 1

`%t`
:   [File type][`filetype`]

`%u`
:   Hexadecimal Unicode value value of character under cursor

`%o`
:   Prints `OVR` or `INS` for [`overwrite`] mode on or off respectively

`%%`
:   Literal `%`

### **statusline-right** [" %y,%X  %u  %o  %E%s%b%s%n %t   %p "]

Format string for the right aligned part of status line.

### **tab-bar** [true]

Whether to show the tab-bar at the top of each window.

### **utf8-bom** [false]

Whether to write a byte order mark (BOM) in newly created UTF-8
files.

Note: buffers opened from existing UTF-8 files will have their BOM
(or lack thereof) preserved as it was, unless overridden by the
[`save`] command.

## Local options

### **brace-indent** [false]

Scan for `{` and `}` characters when calculating indentation size.
Depends on the [`auto-indent`] option.

### **filetype** [none]

Type of file. Value must be previously registered using the [`ft`]
command.

### **indent-regex** [""]

If this [`regex`] pattern matches the current line when enter is
pressed and [`auto-indent`] is true then indentation is increased.
Set to `""` to disable.

## Local and global options

The global values for these options serve as the default values for
local (per-file) options.

### **auto-indent** [true]

Automatically insert indentation when pressing enter.
Indentation is copied from previous non-empty line. If also the
[`indent-regex`] local option is set then indentation is
automatically increased if the regular expression matches
current line.

### **detect-indent** [""]

Comma-separated list of indent widths (`1`-`8`) to detect automatically
when a file is opened. Set to `""` to disable. Tab indentation is
detected if the value is not `""`. Adjusts the following options if
indentation style is detected: [`emulate-tab`], [`expand-tab`],
[`indent-width`].

Example:

    set detect-indent 2,3,4,8

### **emulate-tab** [false]

Make [`delete`], [`erase`] and moving [`left`] and [`right`] inside
indentation feel as if there were tabs instead of spaces.

### **expand-tab** [false]

Convert tab to spaces on insert.

### **file-history** [true]

Save and restore cursor positions for previously opened files.

See also: the `FILES` section in the [`dte`] man page.

### **indent-width** [8]

Size of indentation in spaces.

### **overwrite** [false]

If set to `true`, typing will overwrite existing characters within current
line instead of inserting before them.

### **syntax** [true]

Use syntax highlighting.

### **tab-width** [8]

Width of tab. Recommended value is `8`. If you use other
indentation size than `8` you should use spaces to indent.

### **text-width** [72]

Preferred width of text. Used as the default argument for the
[`wrap-paragraph`] command.

### **ws-error** [special]

Comma-separated list of flags that describe which whitespace
errors should be highlighted. Set to `""` to disable.

`auto-indent`
:   If the [`expand-tab`] option is enabled then this is the
    same as `tab-after-indent,tab-indent`. Otherwise it's
    the same as `space-indent`.

`space-align`
:   Highlight spaces used for alignment after tab indents as errors.

`space-indent`
:   Highlight space indents as errors. Note that this still allows
    using less than [`tab-width`] spaces at the end of indentation
    for alignment.

`tab-after-indent`
:   Highlight tabs used anywhere other than indentation as errors.

`tab-indent`
:   Highlight tabs in indentation as errors. If you set this you
    most likely want to set "tab-after-indent" too.

`special`
:   Display all characters that look like regular space as errors.
    One of these characters is no-break space (U+00A0), which is often
    accidentally typed (AltGr+space in some keyboard layouts).

`trailing`
:   Highlight trailing whitespace characters at the end of lines as
    errors.


[`dte`]: dte.html
[`dte-syntax`]: dte-syntax.html
[`dte -H`]: dte.html#options
[`dte -K`]: dte.html#options
[normal mode]: dte.html#normal-mode
[command mode]: dte.html#command-mode
[search mode]: dte.html#search-mode
[history file]: dte.html#files
[configuration files]: dte.html#files:~:text=%24DTE_HOME/rc
[key bindings]: dte.html#key-bindings
[command]: #commands
[movement commands]: #movement-commands
[options]: #options
[global options]: #global-options
[local options]: #local-options
[common options]: #local-and-global-options
[`pipe-from cat file.txt`]: #exec:~:text=alias%20pipe%2Dfrom
[`$DTE_HOME`]: dte.html#environment
[double quotes]: #double-quoted-strings
[`binding/default`]: https://gitlab.com/craigbarnes/dte/-/blob/master/config/binding/default
[`color/reset`]: https://gitlab.com/craigbarnes/dte/-/blob/master/config/color/reset
[`compiler/go`]: https://gitlab.com/craigbarnes/dte/-/blob/master/config/compiler/go
[`ascii(7)`]: https://www.man7.org/linux/man-pages/man7/ascii.7.html#DESCRIPTION
[standard streams]: https://man7.org/linux/man-pages/man3/stdin.3.html#DESCRIPTION
[external scripts]: https://gitlab.com/craigbarnes/dte/-/tree/master/contrib
[`execvp`]: https://pubs.opengroup.org/onlinepubs/9699919799/functions/execvp.html
[`glob`]: https://pubs.opengroup.org/onlinepubs/9699919799/functions/glob.html
[`regex`]: https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap09.html#tag_09_04
[`ctags`]: https://en.wikipedia.org/wiki/Ctags
[`tags`]: https://docs.ctags.io/en/stable/man/tags.5.html
[Kitty's keyboard protocol]: https://sw.kovidgoyal.net/kitty/keyboard-protocol/
[OSC 52]: https://invisible-island.net/xterm/ctlseqs/ctlseqs.html#h3-Operating-System-Commands:~:text=5%202%20%20%E2%87%92%C2%A0%20Manipulate%20Selection%20Data

[`alias`]: #alias
[`bind`]: #bind
[`bookmark -r`]: #bookmark
[`bookmark`]: #bookmark
[`command`]: #command
[`compile`]: #compile
[`copy`]: #copy
[`cut`]: #cut
[`delete`]: #delete
[`erase`]: #erase
[`errorfmt`]: #errorfmt
[`filetype`]: #filetype
[`filter`]: #filter
[`ft`]: #ft
[`hi`]: #hi
[`include -b`]: #include
[`include`]: #include
[`insert`]: #insert
[`left`]: #left
[`macro`]: #macro
[`msg`]: #msg
[`open`]: #open
[`open -t`]: #open
[`option`]: #option
[`pgdown`]: #pgdown
[`pgup`]: #pgup
[`right`]: #right
[`save`]: #save
[`search`]: #search
[`set`]: #set
[`setenv`]: #setenv
[`show bind`]: #show
[`show include`]: #show
[`show macro`]: #show
[`show msg`]: #show
[`show set`]: #show
[`tag`]: #tag
[`toggle`]: #toggle
[`undo`]: #undo
[`unselect`]: #unselect
[`wrap-paragraph`]: #wrap-paragraph
[`wsplit`]: #wsplit

[`auto-indent`]: #auto-indent
[`emulate-tab`]: #emulate-tab
[`expand-tab`]: #expand-tab
[`file-history`]: #file-history
[`indent-regex`]: #indent-regex
[`indent-width`]: #indent-width
[`newline`]: #newline
[`overwrite`]: #overwrite
[`tab-width`]: #tab-width
[`text-width`]: #text-width
[`utf8-bom`]: #utf8-bom
