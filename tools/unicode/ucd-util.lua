local util = {}

function util.read_ucd_file(path, ucdname)
    assert(ucdname)
    local baseurl = "https://www.unicode.org/Public/10.0.0/ucd"
    local usage = "Usage: %s path/to/%s\n(available from: %s/%s)\n"
    if not path then
        io.stderr:write(usage:format(arg[0], ucdname, baseurl, ucdname))
        os.exit(1)
    end
    local file = assert(io.open(path, "r"))
    local text = assert(file:read("*a"))
    file:close()
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
