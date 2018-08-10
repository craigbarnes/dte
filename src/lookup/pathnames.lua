local pathnames = {
    ["/boot/grub/menu.lst"] = "CONFIG",
    ["/etc/fstab"] = "CONFIG",
    ["/etc/hosts"] = "CONFIG",
}

return {
    keys = pathnames,
    function_name = "filetype_from_pathname",
    return_type = "FileTypeEnum",
    default_return = 0,
}
