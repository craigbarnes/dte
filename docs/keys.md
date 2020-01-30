Default Key Bindings
--------------------

### Editor

`Alt`+`x`
:   Enter command mode (see [`dterc`] for available commands)

`Ctrl`+`q`
:   Quit editor

`Alt`+`z`
:   Suspend editor (resume by running `fg` in the shell)

### Buffer

`Ctrl`+`n`, `Ctrl`+`t`
:   Open new, empty buffer

`Ctrl`+`o`
:   Open file (prompt)

`Ctrl`+`s`
:   Save current buffer

`Ctrl`+`w`
:   Close current buffer

`Alt`+`1`, `Alt`+`2` ... `Alt`+`9`
:   Switch to buffer 1 (or 2, 3, 4, etc.)

`Alt`+`0`
:   Switch to last buffer

`Alt`+`,`
:   Switch to previous buffer

`Alt`+`.`
:   Switch to next buffer

### Navigation

`Alt`+`b`
:   Move cursor to beginning of file

`Alt`+`e`
:   Move cursor to end of file

`Ctrl`+`l`
:   Go to line (prompt)

`Ctrl`+`f`, `Alt`+`/`
:   Find text (prompt)

`Ctrl`+`g`, `F3`
:   Find next

`F4`
:   Find previous

`Alt`+`f`
:   Find next occurrence of word under cursor

`Alt`+`Shift`+`f`
:   Find previous occurrence of word under cursor

### Selection

`Shift`+`left`, `Shift`+`right`
:   Select one character left/right

`Ctrl`+`Shift`+`left`, `Ctrl`+`Shift`+`right`
:   Select one word left/right

`Shift`+`up`, `Shift`+`down`
:   Select to column above/below cursor

`Ctrl`+`Shift`+`up`, `Ctrl`+`Shift`+`down`
:   Select one whole line up/down

`Alt`+`Shift`+`left`, `Shift`+`Home`
:   Select from cursor to beginning of line

`Alt`+`Shift`+`right`, `Shift`+`End`
:   Select from cursor to end of line

`Shift`+`Page Up`, `Alt`+`Shift`+`up`
:   Select one page above cursor

`Shift`+`Page Down`, `Alt`+`Shift`+`down`
:   Select one page below cursor

`Ctrl`+`Shift`+`Page Up`, `Ctrl`+`Alt`+`Shift`+`up`
:   Select whole lines one page up

`Ctrl`+`Shift`+`Page Down`, `Ctrl`+`Alt`+`Shift`+`down`
:   Select whole lines one page down

`Tab`, `Shift`+`Tab`
:   Increase/decrease indentation level of selected (whole) lines

### Editing

`Ctrl`+`c`
:   Copy selection (or line)

`Ctrl`+`x`
:   Cut selection (or line)

`Ctrl`+`v`
:   Paste previously copied or cut text

`Ctrl`+`z`
:   Undo last action

`Ctrl`+`y`
:   Redo last undone action

`Alt`+`j`
:   Wrap paragraph under cursor to value of `text-width` option
    (72 by default)

`Alt`+`t`
:   Insert a tab character (useful when using the `expand-tab` option)

`Alt`+`-`
:   Decrease indent level of selection (or line)

`Alt`+`=`
:   Increase indent level of selection (or line)

### Command Mode

`Esc`, `Ctrl`+`c`, `Ctrl`+`g`
:   Exit command mode

`left`, `Ctrl`+`b`
:   Move one character left

`Ctrl`+`left`, `Alt`+`b`
:   Move one word left

`Home`, `Alt`+`left`, `Ctrl`+`a`
:   Move to beginning of line

`right`, `Ctrl`+`f`
:   Move one character right

`Ctrl`+`right`, `Alt`+`f`
:   Move one word right

`End`, `Alt`+`right`, `Ctrl`+`e`
:   Move to end of line

`Delete`, `Ctrl`+`d`
:   Delete character

`Alt`+`d`, `Alt`+`Delete`
:   Delete whole word

`Ctrl`+`k`
:   Delete to end of line

`Backspace`
:   Erase character

`Ctrl`+`w`, `Alt`+`Backspace`
:   Erase whole word

`Ctrl`+`u`
:   Erase to beginning of line

`Tab`
:   Auto-complete current command or argument

`up`, `down`
:   Go to next/previous history entry (matching current prefix)


[`dterc`]: https://craigbarnes.gitlab.io/dte/dterc.html
