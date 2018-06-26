Default Key Bindings
--------------------

### Editor

`Alt`+`x`
:   Enter command mode (see `dterc` for available commands)

`Ctrl`+`q`
:   Quit editor (use `quit -f` in command mode to quit with unsaved buffers)

`Alt`+`z`
:   Suspend editor (resume by running `fg` in the shell)

### Buffers

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
:   Find next occurence of word under cursor

`Alt`+`Shift`+`f`
:   Find previous occurence of word under cursor

### Editing

`Ctrl`+`r`
:   Find and replace text (prompt)

`Alt`+`s`
:   Begin selecting characters

`Alt`+`l`
:   Begin selecting whole lines

`Alt`+`u`, `Esc`
:   Stop selecting

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
:   Wrap paragraph under the cursor to 72 columns

`Alt`+`t`
:   Insert a tab character (useful when using the `expand-tab` option)

`Alt`+`-`
:   Decrease indent level of selection (or line)

`Alt`+`=`
:   Increase indent level of selection (or line)
