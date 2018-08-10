local attributes = {
    keep = "ATTR_KEEP",
    underline = "ATTR_UNDERLINE",
    reverse = "ATTR_REVERSE",
    blink = "ATTR_BLINK",
    dim = "ATTR_DIM",
    lowintensity = "ATTR_DIM",
    bold = "ATTR_BOLD",
    invisible = "ATTR_INVIS",
    italic = "ATTR_ITALIC",
}

return {
    keys = attributes,
    function_name = "lookup_attr",
    return_type = "unsigned short",
    default_return = 0,
}
