#!/usr/bin/env lua
package.path = "./tools/unicode/?.lua;" .. package.path
local util = require "ucd-util"
local ucd = util.read_ucd_file(arg[1], "EastAsianWidth.txt")
assert(ucd:find("^# EastAsianWidth") == 1, "File format not recognized")

local t, n = {}, 0
for min, max in ucd:gmatch "\n(%x+)%.*(%x*);[WF]" do
    min = assert(tonumber(min, 16))
    max = tonumber(max, 16)
    n = n + 1
    t[n] = {min = min, max = max or min}
end

util.merge_adjacent(t)
util.print_ranges(t)
