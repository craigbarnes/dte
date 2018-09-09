local colors = {
    keep = -2,
    default = -1,
    black = 0,
    red = 1,
    green = 2,
    yellow = 3,
    blue = 4,
    magenta = 5,
    cyan = 6,
    gray = 7,
    darkgray = 8,
    white = 15,
}

return {
    keys = colors,
    function_name = "lookup_color",
    return_type = "short",
    default_return = -3,
}
