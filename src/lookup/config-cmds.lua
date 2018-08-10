local config_commands = {
    -- Commands allowed in config files:
    alias = "true",
    bind = "true",
    cd = "true",
    errorfmt = "true",
    ft = "true",
    hi = "true",
    include = "true",
    ["load-syntax"] = "true",
    option = "true",
    setenv = "true",
    set = "true",
}

return {
    keys = config_commands,
    function_name = "lookup_config_command",
    return_type = "bool",
    default_return = "false",
}
