#!/bin/sh

filename_to_ident() {
    echo "builtin_${1#config/}" | sed 's|[+/.-]|_|g'
}

echo \
'#ifdef __linux__
    #define CONFIG_SECTION SECTION(".dte.config") MAXALIGN
#else
    #define CONFIG_SECTION
#endif'

template='
static CONFIG_SECTION const char %s[] = {
    #embed "%s"
};'

for file in "$@"; do
    ident=$(filename_to_ident "$file")
    # shellcheck disable=SC2059
    printf "$template\n" "$ident" "$file"
done

echo '
#define cfg(n, t) { \
    .name = n, \
    .text = {.data = t, .length = sizeof(t)} \
}

static CONFIG_SECTION const BuiltinConfig builtin_configs[] = {'

for file in "$@"; do
    name="${file#config/}"
    ident=$(filename_to_ident "$file")
    echo "    cfg(\"$name\", $ident),"
done

echo '};'
