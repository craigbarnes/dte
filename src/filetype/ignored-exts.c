static const char ignored_extensions[][12] = {
    "bak",
    "dpkg-backup",
    "dpkg-bak",
    "dpkg-dist",
    "dpkg-new",
    "dpkg-old",
    "dpkg-remove",
    "dpkg-tmp",
    "new",
    "old",
    "orig",
    "pacnew",
    "pacorig",
    "pacsave",
    "rpmnew",
    "rpmorig",
    "rpmsave",
    "ucf-dist",
    "ucf-new",
    "ucf-old",
};

static bool is_ignored_extension(StringView sv)
{
    strview_remove_matching_suffix(&sv, "~");
    if (sv.length < 3 || sv.length >= sizeof(ignored_extensions[0])) {
        return false;
    }
    return !!BSEARCH(&sv, ignored_extensions, ft_compare);
}
