local util = {}

local function usage(ucdname)
    local usage = "Usage: %s path/to/%s\n(available from: %s/%s)\n"
    local baseurl = "https://www.unicode.org/Public/11.0.0/ucd"
    io.stderr:write(usage:format(arg[0], ucdname, baseurl, ucdname))
end

function util.read_ucd(path, ucdname, pattern)
    assert(ucdname)
    assert(pattern)
    if not path then
        usage(ucdname)
        os.exit(1)
    end
    local file = assert(io.open(path, "r"))
    local text = assert(file:read("*a"))
    file:close()
    if text:find(pattern) ~= 1 then
        usage(ucdname)
        os.exit(1)
    end
    return text
end

function util.merge_adjacent(t)
    for i = #t, 1, -1 do
        local this, prev = t[i], t[i - 1]
        if prev and 1 + prev.max == this.min then
            prev.max = this.max
            table.remove(t, i)
        end
    end
end

function util.print_ranges(t)
    local write = assert(io.write)
    for i = 1, #t do
        local min, max = t[i].min, t[i].max
        local sep = (i % 3 == 0) and "\n" or " "
        write(("{0x%04x, 0x%04x},%s"):format(min, max, sep))
    end
    write("\n")
end

return util
