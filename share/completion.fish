complete -c dte -s 'H' -d "Don't load/save history files at startup/exit"
complete -c dte -s 'R' -d "Don't read the user config file"

complete -c dte -s 'c' -x -d 'Run a dterc(5) command, after the editor starts'
complete -c dte -s 't' -xa "(readtags -F '(list \$name #t)' -l 2>/dev/null)" -d 'Jump to the source location of a ctags(1) tag'
complete -c dte -s 'b' -xa "(dte -B)" -d 'Dump a built-in config and exit'
complete -c dte -s 'r' -r -d 'Read user config from a specific file, instead of the default'

complete -c dte -s 's' -r -d 'Validate a dte-syntax(5) file and exit'
complete -c dte -s 'K' -d 'Start editor in "showkey" mode'
complete -c dte -s 'B' -d 'Print list of built-in config names and exit'
complete -c dte -s 'h' -d 'Display help summary and exit'
complete -c dte -s 'V' -d 'Display version number and exit'
