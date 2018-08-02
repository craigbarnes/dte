-- Replace standard Lua "print" function with one that writes to the
-- status bar instead of stdout
print = function(...)
    local buffer = {}
    for i = 1, select("#", ...) do
        buffer[i] = tostring(select(i, ...))
    end
    editor.info_msg(table.concat(buffer, "    "))
end

-- The locale should be set at editor startup and never touched again
os.setlocale = function()
    error("setlocale() is disabled", 2)
end
