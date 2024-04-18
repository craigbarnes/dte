#compdef dte

_arguments -s -S -C \
    -c'[run dterc(5) command]:()' \
    -t'[jump to ctags(1) tag location]::ctag:->ctags' \
    -r'[load user config from specified file, instead of $DTE_HOME/rc]:rcfile:_files' \
    -s'[lint syntax file]:syntax:_files' \
    -b'[dump built-in config]:rcname:->builtins' \
    -B'[print list of built-in configs]' \
    -H"[don't load or save history files]" \
    -R"[don't read the rc file]" \
    -K"[start in a special mode that prints key names as they're pressed]" \
    -V'[show version number]' \
    -h'[show help summary]' \
    '*::files:_files'

case ${state} in
    ctags)
        IFS=$'\n'
        # TODO: Explicitly handle case where no tags file exists
        # TODO: Use `readtags --prefix-match`?
        _values 'tags' $(readtags -l 2>/dev/null | cut -f1 | head -n50000)
        unset IFS
        ;;

    builtins)
        # TODO: Use specific dte binary, instead of first found in $PATH?
        _values 'built-in configs' $(dte -B)
        ;;
esac
