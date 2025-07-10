#!/bin/sh

filename_to_ident() {
    echo "builtin_${1#config/}" | sed 's|[+/.-]|_|g'
}

template='CONFIG_SECTION static const char %s[] = {
    #embed "%s"
};
'

for file in "$@"; do
    ident=$(filename_to_ident "$file")
    # shellcheck disable=SC2059
    printf "$template\n" "$ident" "$file"
done

echo 'CONFIG_SECTION static const BuiltinConfig builtin_configs[] = {'

for file in "$@"; do
    name="${file#config/}"
    ident=$(filename_to_ident "$file")
    echo "    CFG(\"$name\", $ident),"
done

echo '};'
