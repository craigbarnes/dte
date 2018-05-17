#!/usr/bin/env lua
package.path = "./tools/unicode/?.lua;" .. package.path
local util = require "ucd-util"
local ucd = util.read_ucd_file(arg[1], "UnicodeData.txt")
assert(ucd:find("^0000;") == 1, "File format not recognized")

local zero_width = {
    Cf = true, -- Format (includes U+00AD SOFT HYPHEN, which is excluded below)
    Me = true, -- Enclosing_Mark
    Mn = true -- Nonspacing_Mark
}

local t, n = {}, 0
for codepoint, category in ucd:gmatch "(%x+);[^;]*;(%u%a);[^\n]*\n" do
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

util.merge_adjacent(t)
util.print_ranges(t)
