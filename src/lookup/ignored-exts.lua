local ignored_exts = {
    bak = "true",
    ["dpkg-dist"] = "true",
    ["dpkg-old"] = "true",
    new = "true",
    old = "true",
    orig = "true",
    pacnew = "true",
    pacorig = "true",
    pacsave = "true",
    rpmnew = "true",
    rpmsave = "true",
}

return {
    keys = ignored_exts,
    function_name = "is_ignored_extension",
    return_type = "bool",
    default_return = "false",
}
