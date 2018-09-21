#!/usr/bin/env lua

local usage = [[
Usage: %s UnicodeData.txt EastAsianWidth.txt
(both files available from: https://unicode.org/Public/11.0.0/ucd/)
]]

local function read_ucd(path, pattern)
    assert(pattern)
    if not path then
        io.stderr:write(usage:format(arg[0]))
        os.exit(1)
    end
    local file = assert(io.open(path, "r"))
    local text = assert(file:read("*a"))
    file:close()
    assert(text:find(pattern) == 1, "Unrecognized input format")
    return text
end

local function merge_adjacent(t)
    for i = #t, 1, -1 do
        local this, prev = t[i], t[i - 1]
        if prev and 1 + prev.max == this.min then
            prev.max = this.max
            table.remove(t, i)
        end
    end
end

local function print_ranges(t, name)
    local n, write = #t, io.write
    write("static const CodepointRange ", name, "[] = {\n    ")
    for i = 1, n do
        local min, max, sep = t[i].min, t[i].max, ", "
        if i == n then
            sep = ""
        elseif i % 3 == 0 then
            sep = ",\n    "
        end
        write(("{0x%04x, 0x%04x}%s"):format(min, max, sep))
    end
    write("\n};\n\n")
end

local unidata = read_ucd(arg[1], "^0000;")
local eaw = read_ucd(arg[2], "^# EastAsianWidth")

do
    local zero_width = {
        Cf = true, -- Format (includes U+00AD SOFT HYPHEN, which is excluded below)
        Me = true, -- Enclosing_Mark
        Mn = true -- Nonspacing_Mark
    }
    local t, n = {}, 0
    for codepoint, category in unidata:gmatch "(%x+);[^;]*;(%u%a);[^\n]*\n" do
        codepoint = assert(tonumber(codepoint, 16))
        assert(category)
        if
            (zero_width[category] and codepoint ~= 0x00AD)
            -- ZERO WIDTH SPACE
            or codepoint == 0x200B
            -- Hangul Jamo medial vowels and final consonants
            or (codepoint >= 0x1160 and codepoint <= 0x11FF)
        then
            n = n + 1
            t[n] = {min = codepoint, max = codepoint}
        end
    end
    merge_adjacent(t)
    print_ranges(t, "zero_width")
end

do
    local t, n = {}, 0
    for min, max in eaw:gmatch "\n(%x+)%.*(%x*);[WF]" do
        min = assert(tonumber(min, 16))
        max = tonumber(max, 16)
        n = n + 1
        t[n] = {min = min, max = max or min}
    end
    merge_adjacent(t)
    print_ranges(t, "double_width")
end
