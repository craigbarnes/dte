---
title: dterc
section: 5
date: March 2021
description: Command and configuration language used by dte
author: [Craig Barnes, Timo Hirvonen]
seealso: ["`dte`", "`dte-syntax`"]
---

# dterc

dterc is the language used in `dte` configuration files (`~/.dte/rc`)
and also in the command mode of the editor (Alt+x). The syntax of the
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

    alias x "run chmod 755 $FILE"

`$FILE` is expanded when the alias _x_ is executed. The command works even
if `$FILE` contains whitespace.

## Special variables

These variables are always defined and override environment variables of
the same name.

### **$FILE**

The filename of the current buffer (or an empty string if unsaved).

### **$FILETYPE**

The value of the [`filetype`] option for the current buffer.

### **$LINENO**

The line number of the cursor in the current buffer.

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
:   Control characters (same as in C)

`\e`
:   Escape character

`\\`
:   Backslash

`\"`
:   Double quote

`\x0a`
:   Hexadecimal byte value 0x0a. Note that `\x00` is not supported
    because strings are NUL-terminated.

`\u20ac`
:   Four hex digit Unicode code point U+20AC.

`\U000020ac`
:   Eight hex digit Unicode code point U+20AC.

# Commands

## Configuration Commands

Configuration commands are used to customize certain aspects of the
editor, for example adding key bindings, setting options, etc. These
are the only commands allowed in user config files.

### **alias** _name_ _command_

Create an alias _name_ for _command_.

Example:

    alias read 'pipe-from cat'

Now you can run `read file.txt` to insert `file.txt` into the current
buffer.

### **bind** _key_ [_command_]

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
* `begin` (keypad "5" with Num Lock off)
* `enter`
* `tab`
* `space`
* `F1`..`F12`

Modifiers:

Ctrl:
:   `C-X` or `^X`

Alt:
:   `M-X`

Shift:
:   `S-left`

### **set** [**-gl**] _option_ [_value_] ...

Set _value_ for _option_. Value can be omitted for boolean option to set
it true. Multiple options can be set at once but then _value_ must be
given for every option.

There are three kinds of options.

1. Global options.

2. Local options. These are file specific options. Each open file has
   its own copies of the option values.

3. Options that have both global and local values. The Global value is
   just a default local value for opened files and is never used for
   anything else. Changing the global value does not affect any already
   opened files.

By default `set` changes both global and local values.

`-g`
:   Change only global option value

`-l`
:   Change only local option value of current file

In configuration files only global options can be set (no need
to specify the `-g` flag).

See also: [`toggle`] and [`option`] commands.

### **setenv** _name_ [_value_]

Set (or unset) environment variable.

### **hi** [**-c**] _name_ [_fg-color_ [_bg-color_]] [_attribute_]...

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
* A true color value in CSS-style #RRGGBB notation (e.g. `#ab90df`)
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

Colors `16` to `231` correspond to R/G/B colors. Colors `232` to `255`
are grayscale values.

If the terminal has limited support for rendering colors, the _fg-color_
and _bg-color_ arguments will fall back to the nearest supported color
(unless the `-c` flag is used).

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

`-c`
:   Do nothing at all if the terminal can't display _fg-color_ and/or
    _bg-color_ with full precision

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
See the `-B` and `-b` flags in the [`dte`] man page for more information.

### **errorfmt** [**-i**] _compiler_ _regexp_ [file|line|column|message|_]...

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

For a basic example of usage, see the output of `dte -b compiler/go`.

`-i`
:   Ignore this error

### **load-syntax** _filename_|_filetype_

Load a [`dte-syntax`] file into the editor. If the argument contains a
`/` character it's considered a filename.

Note: this command only loads a syntax file ready for later use. To
actually apply a syntax highlighter to the current buffer, use the
[`set`] command to change the [`filetype`] of the buffer instead, e.g.
`set filetype html`.

## Editor Commands

### **quit** [**-f**|**-p**] [_exitcode_]

Quit the editor.

The exit status of the process is set to _exitcode_, which can be
in the range `0`..`125`, or defaults to `0` if unspecified.

`-f`
:   Force quit, even if there are unsaved files

`-p`
:   Prompt for confirmation if there are unsaved files

### **suspend**

Suspend the editor (run `fg` in the shell to resume).

### **cd** _directory_

Change the working directory and update `$PWD` and `$OLDPWD`. Running
`cd -` changes to the previous directory (`$OLDPWD`).

### **command** [_text_]

Enter command mode. If _text_ is given then it is written to the command
line (see the default `^L` key binding for why this is useful).

### **search** [**-Hnprw**] [_pattern_]

If no flags or just `-r` and no _pattern_ given then dte changes to
search mode where you can type a regular expression to search.

`-H`
:   Don't add _pattern_ to search history

`-n`
:   Search next

`-p`
:   Search previous

`-r`
:   Start searching backwards

`-w`
:   Search word under cursor

### **refresh**

Trigger a full redraw of the screen.

## Buffer Management Commands

### **open** [**-g**|**-t**] [**-e** _encoding_] [_filename_]...

Open file. If _filename_ is omitted, a new file is opened.

`-e` _encoding_
:   Set file _encoding_. See `iconv -l` for list of supported encodings.

`-g`
:   Perform [`glob`] expansion on _filename_.

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
:   Set file _encoding_. See `iconv -l` for list of supported encodings.

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

### **wsplit** [**-bghnrt**] [_filename_]...

Split the current window.

_filename_ arguments will be opened in a manner similar to the [`open`]
command. If there are no arguments, the contents of the new window will
be an additional view of the current buffer.

`-b`
:   Add new window before current instead of after.

`-g`
:   Perform [`glob`] expansion on _filename_.

`-h`
:   Split horizontally instead of vertically.

`-n`
:   Create a new, empty buffer.

`-r`
:   Split root instead of current window.

`-t`
:   Mark buffer as "temporary" (always closeable, without warnings
    for "unsaved changes")

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
:   Set size of current window to _N_ characters.

`+`_N_
:   Increase size of current window by _N_ characters.

`-`_N_
:   Decrease size of current window by _N_ characters. Use `--` to
    prevent the minus symbol being parsed as an option flag, e.g.
    `wresize -- -5`.

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

### **left** [**-c**]

Move one column left.

### **right** [**-c**]

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

### **word-fwd** [**-cs**]

Move forward one word.

`-s`
:   Skip special characters

### **word-bwd** [**-cs**]

Move backward one word.

`-s`
:   Skip special characters

### **bol** [**-cs**]

Move to beginning of current line.

`-s`
:   Move to beginning of indented text or beginning of line, depending
    on current cursor position.

### **eol** [**-c**]

Move to end of current line.

### **bof**

Move to beginning of file.

### **eof**

Move to end of file.

### **bolsf**

Incrementally move to beginning of line, then beginning of screen, then
beginning of file.

### **eolsf**

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

### **match-bracket**

Move to the bracket character paired with the one under the cursor.
The character under the cursor should be one of `{}[]()<>`.

### **line** _number_

Go to line.

### **tag** [**-r**] [_tag_]

Save the current location and jump to the location of _tag_. If no _tag_
argument is specified then the word under the cursor is used instead.

This command requires a `tags` file generated by [`ctags`]. `tags` files
are searched for in the current working directory and its parent
directories.

`-r`
:   jump back to the previous location

See also: [`msg`] command.

### **msg** [**-n**|**-p**]

Show latest, next (`-n`) or previous (`-p`) message. If its location
is known (compile error or tag message) then the file will be
opened and cursor moved to the location.

`-n`
:   Next message

`-p`
:   Previous message

See also: [`compile`] and [`tag`] commands.

## Editing Commands

### **cut**

Cut current line or selection.

### **copy** [**-k**]

Copy current line or selection.

`-k`
:   Keep selection (by default, selections are lost after copying)

### **paste** [**-c**]

Paste text previously copied by the [`copy`] or [`cut`] commands.

`-c`
:   Paste at the cursor position, even when the text was copied as
    a whole-line selection (where the usual default is to paste at
    the start of the next line)

### **undo**

Undo latest change.

### **redo** [_choice_]

Redo changes done by the [`undo`] command. If there are multiple
possibilities a message is displayed:

    Redoing newest (2) of 2 possible changes.

If the change was not the one you wanted, just run `undo` and
then, for example, `redo 1`.

### **clear**

Clear current line.

### **join**

Join selection or next line to current.

### **new-line**

Insert empty line under current line.

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

### **delete-line**

Delete current line.

### **case** [**-l**|**-u**]

Change text case. The default is to change lower case to upper case and
vice versa.

`-l`
:   Lower case

`-u`
:   Upper case

### **insert** [**-km**] _text_

Insert _text_ into the buffer.

`-k`
:   Insert one character at a time, as if manually typed. Normally
    _text_ is inserted exactly as specified, but this option allows
    it to be affected by special input handling like auto-indents,
    whitespace trimming, line-by-line undo, etc.

`-m`
:   Move after inserted text

### **replace** [**-bcgi**] _pattern_ _replacement_

Replace all instances of text matching _pattern_ with the _replacement_
text.

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
:   Use basic instead of extended regex syntax

`-c`
:   Ask for confirmation before each replacement

`-g`
:   Replace all matches for each line (instead of just the first)

`-i`
:   Ignore case

Examples:

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

### **select** [**-bkl**]

Enter selection mode. All movement commands while in this mode extend
the selected area.

Note: A better way to create selections is to hold the Shift key whilst
moving the cursor. The `select` command exists mostly as a fallback,
for terminals with limited key binding support.

`-b`
:   Select block between opening `{` and closing `}` curly braces

`-k`
:   Keep existing selections

`-l`
:   Select whole lines

### **unselect**

Unselect.

## External Commands

### **filter** [**-l**] _command_ [_parameter_]...

Filter selected text or whole file through external _command_.

Example:

    filter sort -r

Note that _command_ is executed directly using [`execvp`]. To use shell
features like pipes or redirection, use a shell interpreter as the
_command_. For example:

    filter sh -c 'tr a-z A-Z | sed s/foo/bar/'

`-l`
:   Operate on current line instead of whole file, if there's no selection

### **pipe-from** [**-ms**] _command_ [_parameter_]...

Run external _command_ and insert its standard output.

`-m`
:   Move after the inserted text

`-s`
:   Strip newline from end of output

### **pipe-to** [**-l**] _command_ [_parameter_]...

Run external _command_ and pipe the selected text (or whole file) to
its standard input.

Can be used to e.g. write text to the system clipboard:

    pipe-to xsel -b

`-l`
:   Operate on current line instead of whole file, if there's no selection

### **run** [**-ps**] _command_ [_parameters_]...

Run external _command_.

`-p`
:   Display "Press any key to continue" prompt

`-s`
:   Silent -- both `stderr` and `stdout` are redirected to `/dev/null`

### **compile** [**-1ps**] _errorfmt_ _command_ [_parameters_]...

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
:   Silent. Both `stderr` and `stdout` are redirected to `/dev/null`

See also: [`errorfmt`] and [`msg`] commands.

### **eval** _command_ [_parameter_]...

Run external _command_ and execute its standard output text as dterc
commands.

### **exec-open** [**-s**] _command_ [_parameter_]...

Run external _command_ and open all filenames listed on its standard
output.

`-s`
:   Don't yield terminal control to the child process

Example uses:

    exec-open -s find . -type f -name *.h
    exec-open -s git ls-files --modified
    exec-open fzf -m --reverse

### **exec-tag** [**-s**] _command_ [_parameter_]...

Run external _command_ and then execute the `tag` command with its
first line of standard output as the argument.

`-s`
:   Don't yield terminal control to the child process

Example uses:

    exec-tag -s echo main
    exec-tag sh -c 'readtags -l | cut -f1 | sort | uniq | fzf --reverse'

## Other Commands

### **repeat** _count_ _command_ [_parameters_]...

Run _command_ _count_ times.

### **toggle** [**-gv**] _option_ [_values_]...

Toggle _option_. If list of _values_ is not given then the option
must be either boolean or enum.

`-g`
:   toggle global option instead of local

`-v`
:   display new value

If _option_ has both local and global values then local is toggled
unless `-g` is used.

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

`include`
:   Show [built-in configs][`include`]

`macro`
:   Show last recorded [macro][`macro`]

`option`
:   Show [option values](#options)

`search`
:   Show [search history][`search`]

`wsplit`
:   Show [window dimensions][`wsplit`]

The _key_ argument is the name of the entry to look up (e.g. the alias
name). If this argument is omitted, the full list of entries of the
specified _type_ will be displayed in a new buffer.

`-c`
:   write value to command line (if possible)

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

# Options

Options can be changed using the [`set`] command. Enumerated options can
also be [`toggle`]d. To see which options are enumerated, type "toggle "
in command mode and press the tab key. You can also use the [`option`]
command to set default options for specific file types.

## Global options

### **case-sensitive-search** [true]

`false`
:   Search is case-insensitive.

`true`
:   Search is case-sensitive.

`auto`
:   If search string contains an uppercase letter search is
    case-sensitive, otherwise it is case-insensitive.

### **display-invisible** [false]

Display invisible characters.

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

### **select-cursor-char** [true]

Whether to include the character under the cursor in selections.

### **scroll-margin** [0]

Minimum number of lines to keep visible before and after cursor.

### **set-window-title** [false]

Set the window title to the filename of the current buffer (if the
terminal supports it).

### **show-line-numbers** [false]

Show line numbers.

### **statusline-left** [" %f%s%m%r%s%M"]

Format string for the left aligned part of status line.

`%f`
:   Filename.

`%m`
:   Prints `*` if file is has been modified since last save.

`%r`
:   Prints `RO` for read-only buffers or `TMP` for temporary buffers.

`%y`
:   Cursor row.

`%Y`
:   Total rows in file.

`%x`
:   Cursor display column.

`%X`
:   Cursor column as characters. If it differs from cursor display
    column then both are shown (e.g. `2-9`).

`%p`
:   Position in percentage.

`%E`
:   File encoding.

`%M`
:   Miscellaneous status information.

`%n`
:   Line-ending (`LF` or `CRLF`).

`%N`
:   Line-ending (only if `CRLF`).

`%s`
:   Separator (a single space, unless the preceding format character
    expanded to an empty string).

`%S`
:   Like `%s`, but 3 spaces instead of 1.

`%t`
:   File type.

`%u`
:   Hexadecimal Unicode value value of character under cursor.

`%%`
:   Literal `%`.

### **statusline-right** [" %y,%X   %u   %E%s%b%s%n %t   %p "]

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
:   Highlight spaces used for alignment after tab
    indents as errors.

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
[`$DTE_HOME`]: dte.html#environment
[`execvp`]: https://pubs.opengroup.org/onlinepubs/9699919799/functions/execvp.html
[`glob`]: https://pubs.opengroup.org/onlinepubs/9699919799/functions/glob.html
[`regex`]: https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap09.html#tag_09_04
[`xterm`]: https://invisible-island.net/xterm/
[`ctags`]: https://ctags.io/
[double quotes]: #double-quoted-strings

[`alias`]: #alias
[`bind`]: #bind
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
[`include`]: #include
[`left`]: #left
[`macro`]: #macro
[`message`]: #message
[`msg`]: #msg
[`open`]: #open
[`option`]: #option
[`pgdown`]: #pgdown
[`pgup`]: #pgup
[`right`]: #right
[`save`]: #save
[`scroll-pgdown`]: #scroll-pgdown
[`scroll-pgup`]: #scroll-pgup
[`search`]: #search
[`set`]: #set
[`show macro`]: #show
[`tag`]: #tag
[`toggle`]: #toggle
[`undo`]: #undo
[`wrap-paragraph`]: #wrap-paragraph
[`wsplit`]: #wsplit

[`auto-indent`]: #auto-indent
[`emulate-tab`]: #emulate-tab
[`expand-tab`]: #expand-tab
[`indent-regex`]: #indent-regex
[`indent-width`]: #indent-width
[`newline`]: #newline
[`tab-width`]: #tab-width
[`text-width`]: #text-width
[`utf8-bom`]: #utf8-bom
