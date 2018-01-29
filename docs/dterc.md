---
title: dterc
section: 5
date: November 2017
description: Command and configuration language used by `dte`
author: [Craig Barnes, Timo Hirvonen]
seealso: ["`dte`", "`dte-syntax`"]
---

# dterc

dterc is the language used in `dte` configuration files and also in the
command mode of the editor. The syntax of the language is quite similar
to shell, but much simpler.

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

### **$WORD**

The selected text or the word under the cursor.

## Single quoted strings

Single quoted strings can't contain single quotes or escaped characters.

## Double quoted strings

Double quoted strings may contain the following escapes:

`\a`, `\b`, `\t`, `\n`, `\v`, `\f`, `\r`, `\\`
:   Control characters (same as in C)

`\x0a`
:   Hexadecimal byte value 0x0a. Note that `\x00` is not supported because
    strings are NUL-terminated.

`\u20ac`
:   Four hex digit unicode code point U+20AC.

`\U000020ac`
:   Eight hex digit unicode code point U+20AC.

# Commands

### **alias** _name_ _command_

Create an alias _name_ for _command_.

Example:

    alias read "pass-through cat"

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

Key chains are supported. For example `"^X c"` (press `^X` and then `c`).
Keys are separated by spaces.

### **bof**

Move to beginning of file.

### **bol**

Move to beginning of line.

### **bolsf**

Incrementally move cursor to beginning of line, then beginning
of screen, then beginning of file.

### **case** [**-lu**]

Change text case. The default is to change lower case to upper case and
vice versa.

`-l`
:   Lower case

`-u`
:   Upper case

### **cd** _directory_

Change the working directory and update `$PWD` and `$OLDPWD`. Running
`cd -` changes to the previous directory (`$OLDPWD`).

### **center-view**

Center view to cursor.

### **clear**

Clear current line.

### **close** [**-fqw**]

Close file.

`-f`
:   Close file even if it hasn't been saved after last modification

`-q`
:   Quit if closing the last open file

`-w`
:   Close parent window if closing its last contained file

### **command** [_text_]

Enter command mode. If _text_ is given then it is written to the command
line (see the default `^L` key binding for why this is useful).

### **compile** [**-1ps**] _errorfmt_ _command_ [_parameters_]...

Run external _command_ and collect error messages. This can be
used to run `make`(1) and `grep`(1).

`-1`
:   Read error messages from stdout instead of stderr

`-p`
:   Display "Press any key to continue" prompt

`-s`
:   Silent. Both `stderr` and `stdout` are redirected to `/dev/null`

See also: `errorfmt` and `msg` commands.

### **copy**

Copy current line or selection.

### **cut**

Cut current line or selection.

### **delete**

Delete character or selection.

### **delete-eol** [**-n**]

Delete to end of line.

`-n`
:   Delete newline if cursor is at end of line

### **delete-word** [**-s**]

Delete word after cursor.

`-s`
:   Be more "aggressive"

### **down**

Move cursor down.

### **eof**

Move cursor to end of file.

### **eol**

Move cursor to end of line.

### **eolsf**

Incrementally move cursor to end of line, then end of screen, then
end of file.

### **erase**

Erase character before cursor.

### **erase-bol**

Erase to beginning of line.

### **erase-word** [**-s**]

Erase word before cursor.

`-s`
:   Be more "aggressive"

### **errorfmt** [**-i**] _compiler_ _regexp_ [file|line|column|message]...

`-i`
:   Ignore this error

See `compile` and `msg` commands for more information.

### **filter** _command_ [_parameter_]...

Filter selected text or whole file through external _command_.

Example:

    filter sort -r

### **format-paragraph** [_width_]

Format the current selection or paragraph under the cursor. If
paragraph _width_ is not given then the `text-width` option is
used.

This command merges the selection into one paragraph. To format
multiple paragraphs use the external `fmt`(1) program with the
`filter` command, e.g. `filter fmt -w 60`.

### **ft** _filetype_ _extension_...

Associate filename _extension_ with _filetype_.

Filetypes are used to determine which syntax highlighter and local
options to use when opening files.

Example:

    ft sh sh bash ksh zsh

See also:

* The `option` command (below)
* The `filetype` option (below)
* The [`dte-syntax`] man page
* The [built-in filetype associations][`config/filetype`]
  (which can be listed by running `dte -b filetype`)

### **ft** **-b** _filetype_ _basename_...

Associate file _basename_ with _filetype_.

### **ft** **-c** _filetype_ _contents-regexp_...

Detect _filetype_ by matching _contents-regexp_ against first line of file.

### **ft** **-f** _filetype_ _filename-regexp_...

Detect _filetype_ by matching _filename-regexp_ against filename.

### **ft** **-i** _filetype_ _interpreter_...

Associate _interpreter_ with _filetype_. Interpreters are parsed
from the `#!` line in many scripts.

### **git-open**

Interactive file opener. Lists all files in a git repository.

Same keys work as in command mode, but with these changes:

`up`
:   Move up in file list.

`down`
:   Move down in file list.

`enter`
:   Open file.

`^O`
:   Open file but don't close git-open.

`M-e`
:   Go to end of file list.

`M-t`
:   Go to top of file list.

### **hi** _name_ [_fg-color_ [_bg-color_]] [_attribute_]...

Set highlight color.

Colors:

* `keep` (`-2`)
* `default` (`-1`)
* `black` (`0`)
* `red`
* `green`
* `yellow`
* `blue`
* `magenta`
* `cyan`
* `gray`
* `darkgray`
* `lightred`
* `lightgreen`
* `lightyellow`
* `lightblue`
* `lightmagenta`
* `lightcyan`
* `white`

Color can be given as a numeric value too (`-2`..`255`).

Colors `16`-`255` are supported by modern [`xterm`]-compatible terminal
emulators. There's a 6x6x6 color cube at indexes `16`..`231`. For these
colors it is easiest to use the R/G/B syntax where R, G and B are values
between `0` and `5`.

Indexes `232`..`255` contain 24 grayscale values that can be used
to specify grayscale value more accurately than using the R/G/B
syntax.

Attributes:

* `bold`
* `lowintensity`
* `italic`
* `underline`
* `blink`
* `reverse`
* `invisible`
* `keep`

The color and attribute value `keep` is useful in selected text
to keep _fg-color_ and attributes and change only _bg-color_.

NOTE: Because `keep` is both a color and an attribute you need to
specify both _fg-color_ and _bg-color_ if you want to set the `keep`
_attribute_.

If you omit any color it is set to `default` (`-1`).

Unset fg/bg colors are inherited from highlight color `default`.
If you don't set fg/bg for the highlight color `default` then
terminal's default fg/bg is used.

### **include** [**-b**] _file_

Read commands from _file_.

`-b`
:   Read built-in _file_ instead of reading from the filesystem

### **insert** [**-km**] _text_

Insert _text_ into the buffer.

`-k`
:   Insert one character at a time as if it has been typed

`-m`
:   Move after inserted text

### **insert-special**

Enter an input mode that allows inserting special characters or byte
values. After running the command, type a decimal value or use one of
the following prefixes:

`o`
:   Insert 3-digit octal byte value

`x`
:   Insert 2-digit hexadecimal byte value

`u`
:   Insert 6-digit hexadecimal unicode value

### **join**

Join selection or next line to current.

### **left**

Move left.

### **line** _number_

Go to line.

### **load-syntax** _filename_|_filetype_

Load a `dte-syntax` file into the editor. If the argument contains a
`/` character it's considered a filename.

Note: this command only loads a syntax file ready for later use. To
actually apply a syntax highlighter to the current buffer, use the
`set` command to change the `filetype` of the buffer instead, e.g.
`set filetype html`.

### **move-tab** _N_|left|right

Move current tab to position _N_ or 1 position left or right.

### **msg** [**-np**]

Show latest, next (`-n`) or previous (`-p`) message. If its location
is known (compile error or tag message) then the file will be
opened and cursor moved to the location.

`-n`
:   Next message

`-p`
:   Previous message

See also `compile` and `tag` commands.

### **new-line**

Insert empty line under current line.

### **next**

Display next file.

### **open** [**-e** _encoding_] [_file_]...

Open _file_. If filename is omitted, a new file is opened.

`-e` _encoding_
:   Set file _encoding_. See `iconv -l` for list of supported encodings.

### **option** _filetype_ _option_ _value_...

Add automatic _option_ for _filetype_ (as previously registered
with the `ft` command). Automatic options are set when files are
are opened.

### **option** **-r** _regexp_ _option_ _value_...

Add automatic _option_ for filenames that match _regexp_.

### **pass-through** [**-ms**] _command_ [_parameter_]...

Run external _command_ and insert its output.

`-m`
:   Move after the inserted text

`-s`
:   Strip newline from end of the command output

### **paste**

Paste.

`-c`
:   Paste at the cursor position

### **pgdown**

Move cursor page down. See also `scroll-pgdown`.

### **pgup**

Move cursor page up. See also `scroll-pgup`.

### **prev**

Display previous file.

### **quit** [**-f**]

Quit.

`-f`
:   Force quitting even if there are unsaved files

### **redo** [_choice_]

Redo changes done by the `undo` command. If there are multiple
possibilities an informative message is displayed:

    Redoing newest (2) of 2 possible changes.

If the change was not the one you wanted, just run `undo` and
then, for example, `redo 1`.

### **repeat** _count_ _command_ [_parameters_]...

Run _command_ _count_ times.

### **replace** [**-bcgi**] _pattern_ _replacement_

Replace all instances of text matching _pattern_ with the _replacement_
text.

The _pattern_ is a POSIX extended **regex**(7).

`-b`
:   Use basic instead of extended regex syntax

`-c`
:   Ask for confirmation before each replacement

`-g`
:   Replace all matches for each line (instead of just the first)

`-i`
:   Ignore case

### **right**

Move right.

### **run** [**-ps**] _command_ [_parameters_]...

Run external _command_.

`-p`
:   Display "Press any key to continue" prompt

`-s`
:   Silent -- both `stderr` and `stdout` are redirected to `/dev/null`

### **save** [**-dfup**] [**-e** _encoding_] [_filename_]

Save file. By default line-endings (LF vs CRLF) are preserved.

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

### **scroll-down**

Scroll view down one line. Keeps cursor position unchanged if possible.

### **scroll-pgdown**

Scroll page down. Cursor position relative to top of screen is
maintained. See also `pgdown`.

### **scroll-pgup**

Scroll page up. Cursor position relative to top of screen is
maintained. See also `pgup`.

### **scroll-up**

Scroll view up one line. Keeps cursor position unchanged if possible.

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

### **select** [**-bkl**]

Start selecting an area of text.

`-b`
:   Select block between opening `{` and closing `}` curly braces

`-k`
:   Keep existing selections

`-l`
:   Select whole lines

### **set** [**-gl**] _option_ [_value_] ...

Set _value_ for _option_. Value can be omitted for boolean option to set
it true. Multiple options can be set at once but then _value_ must be
given for every option.

There are three kinds of options.

1. Global options.

2. Local options. These are file specific options. Each open file has
   its own copies of the option values.

3. Options that have both global and local values. The Global value is
   just a default local value for opened files and never used for
   anything else. Changing the global value does not affect any already
   opened files.

By default `set` changes both global and local values.

`-g`
:   Change only global option value

`-l`
:   Change only local option value of current file

In configuration files only global options can be set (no need
to specify the `-g` flag).

To automatically set options for specific filetypes and filenames use
the `option` command.

### **setenv** _name_ _value_

Set environment variable.

### **shift** _count_

Shift current or selected lines by _count_ indentation levels.
Count is usually `-1` (decrease indent) or `1` (increase indent).

To specify a negative number, it's necessary to first disable
option parsing with `--`, e.g. `shift -- -1`.

### **suspend**

Suspend program.

### **tag** [**-r**] [_tag_]

Save current location to stack and go to the location of _tag_.
Requires tags file generated by Exuberant Ctags. If no _tag_ is
given then word under cursor is used as a tag instead.

`-r`
:   return back to previous location

Tag files are searched from current working directory and its
parent directories.

See also `msg` command.

### **toggle** [**-gv**] _option_ [_values_]...

Toggle _option_. If list of _values_ is not given then the option
must be either boolean or enum.

`-g`
:   toggle global option instead of local

`-v`
:   display new value

If _option_ has both local and global value then local is toggled
unless -g is given.

### **undo**

Undo latest change.

### **unselect**

Unselect.

### **up**

Move cursor up.

### **view** _N_|last

Display _N_th or last open file.

### **wclose** [**-f**]

Close window.

`-f`
:   Close even if there are unsaved files in the window

### **wflip**

Change from vertical layout to horizontal and vice versa.

### **wnext**

Next window.

### **word-bwd** [**-s**]

Move cursor backward one word.

`-s`
:   Skip special characters

### **word-fwd** [**-s**]

Move cursor forward one word.

`-s`
:   Skip special characters

### **wprev**

Previous window.

### **wresize** [**-hv**] [_N_|+_N_|-- -_N_]

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

### **wsplit** [**-bhr**] [_file_]...

Like `open` but at first splits current window vertically.

`-b`
:   Add new window before current instead of after.

`-h`
:   Split horizontally instead of vertically.

`-r`
:   Split root instead of current window.

### **wswap**

Swap positions of this and next frame.

# Options

Options can be changed using the `set` command. Enumerated options can
also be `toggle`d. To see which options are enumerated, type "toggle "
in command mode and press the tab key. You can also use the `option`
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

### **display-special** [false]

Display special characters.

### **esc-timeout** [100] 0...2000

When single escape is read from the terminal dte waits some
time before treating the escape as a single keypress. The
timeout value is in milliseconds.

Too long timeout makes escape key feel slow and too small
timeout can cause escape sequences of for example arrow keys to
be split and treated as multiple key presses.

### **lock-files** [true]

Lock files using `$DTE_HOME/file-locks`. Only protects from your
own mistakes (two processes editing same file).

### **newline** [unix]

Whether to use LF (_unix_) or CRLF (_dos_) line-endings. This is
just a default value for new files.

### **scroll-margin** [0]

Minimum number of lines to keep visible before and after cursor.

### **show-line-numbers** [false]

Show line numbers.

### **statusline-left** [" %f%s%m%r%s%M"]

Format string for the left aligned part of status line.

`%f`
:   Filename.

`%m`
:   Prints `*` if file is has been modified since last save.

`%r`
:   Prints `RO` if file is read-only.

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
:   Line-ending (LF or CRLF).

`%s`
:   Add separator.

`%t`
:   File type.

`%u`
:   Hexadecimal unicode value value of character under cursor.

`%%`
:   Literal `%`.

### **statusline-right** [" %y,%X   %u   %E %n %t   %p "]

Format string for the right aligned part of status line.

### **tab-bar** [horizontal]

`hidden`
:   Hide tab bar.

`horizontal`
:   Show tab bar on top.

`vertical`
:   Show tab bar on left if there's enough space, hide otherwise.

`auto`
:   Show tab bar on left if there's enough space, on top otherwise.

### **tab-bar-max-components** [0]

Maximum number of path components displayed in vertical tab bar.
Set to `0` to disable.

### **tab-bar-width** [25]

Width of vertical tab bar. Note that width of tab bar is
automatically reduced to keep editing area at least 80
characters wide. Vertical tab bar is shown only if there's
enough space.

## Local options

### **brace-indent** [false]

Scan for `{` and `}` characters when calculating indentation size.
Depends on the `auto-indent` option.

### **filetype** [none]

Type of file. Value must be previously registered using the `ft`
command.

### **indent-regex** [""]

If this regular expression matches current line when enter is
pressed and `auto-indent` is true then indentation is increased.
Set to `""` to disable.

## Local and global options

The global values for these options serve as the default values for
local (per-file) options.

### **auto-indent** [true]

Automatically insert indentation when pressing enter.
Indentation is copied from previous non-empty line. If also the
`indent-regex` local option is set then indentation is
automatically increased if the regular expression matches
current line.

### **detect-indent** [""]

Comma-separated list of indent widths (`1`-`8`) to detect automatically
when a file is opened. Set to `""` to disable. Tab indentation is
detected if the value is not `""`. Adjusts the following options if
indentation style is detected: `emulate-tab`, `expand-tab`,
`indent-width`.

Example:

    set detect-indent 2,3,4,8

### **emulate-tab** [false]

Make `delete`, `erase` and moving `left` and `right` inside
indentation feel as if there were tabs instead of spaces.

### **expand-tab** [false]

Convert tab to spaces on insert.

### **file-history** [true]

Save line and column for each file to `$DTE_HOME/file-history`.

### **indent-width** [8]

Size of indentation in spaces.

### **syntax** [true]

Use syntax highlighting.

### **tab-width** [8]

Width of tab. Recommended value is `8`. If you use other
indentation size than `8` you should use spaces to indent.

### **text-width** [72]

Preferred width of text. Used as the default argument for the
`format-paragraph` command.

### **ws-error** [special]

Comma-separated list of flags that describe which whitespace
errors should be highlighted. Set to `""` to disable.

`auto-indent`
:   If the `expand-tab` option is enabled then this is the
    same as `tab-after-indent,tab-indent`. Otherwise it's
    the same as `space-indent`.

`space-align`
:   Highlight spaces used for alignment after tab
    indents as errors.

`space-indent`
:   Highlight space indents as errors. Note that this still allows
    using less than `tab-width` spaces at the end of indentation
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


[`dte-syntax`]: dte-syntax.html
[`config/filetype`]: https://github.com/craigbarnes/dte/blob/master/config/filetype
[`xterm`]: https://invisible-island.net/xterm/
