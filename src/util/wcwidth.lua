local progname = arg[0]
local RangeTable = {}
RangeTable.__index = RangeTable

function RangeTable.new()
    return setmetatable({n = 0}, RangeTable)
end

function RangeTable:insert(min, max)
    local n = self.n + 1
    self[n] = {min = assert(min), max = assert(max)}
    self.n = n
end

function RangeTable:remove(index)
    table.remove(self, index)
    self.n = self.n - 1
end

function RangeTable:merge_adjacent()
    for i = self.n, 1, -1 do
        local cur, prev = self[i], self[i - 1]
        if prev and 1 + prev.max == cur.min then
            prev.max = cur.max
            self:remove(i)
        end
    end
    return self
end

function RangeTable:print_ranges(name, output)
    local n = self.n
    output:write("static const CodepointRange ", name, "[] = {\n    ")
    for i = 1, n do
        local min, max, sep = self[i].min, self[i].max, ", "
        if i == n then
            sep = ""
        elseif i % 3 == 0 then
            sep = ",\n    "
        end
        output:write(("{0x%04x, 0x%04x}%s"):format(min, max, sep))
    end
    output:write("\n};\n\n")
end

local function read_ucd(path, pattern)
    assert(pattern)
    if not path then
        io.stderr:write (
            "Usage: ", progname, " UnicodeData.txt EastAsianWidth.txt\n",
            "(available from: https://unicode.org/Public/11.0.0/ucd/)\n"
        )
        os.exit(1)
    end
    local file = assert(io.open(path, "r"))
    local text = assert(file:read("*a"))
    file:close()
    assert(text:find(pattern) == 1, "Unrecognized input format")
    return text
end

local unidata = read_ucd(arg[1], "^0000;")
local eaw = read_ucd(arg[2], "^# EastAsianWidth")

local zero_width_categories = {
    Cf = true, -- Format (includes U+00AD SOFT HYPHEN, which is excluded below)
    Me = true, -- Enclosing_Mark
    Mn = true -- Nonspacing_Mark
}

local zero_width = RangeTable.new()
for codepoint, category in unidata:gmatch "(%x+);[^;]*;(%u%a);[^\n]*\n" do
    codepoint = assert(tonumber(codepoint, 16))
    assert(category)
    if
        (zero_width_categories[category] and codepoint ~= 0x00AD)
        -- ZERO WIDTH SPACE
        or codepoint == 0x200B
        -- Hangul Jamo medial vowels and final consonants
        or (codepoint >= 0x1160 and codepoint <= 0x11FF)
    then
        zero_width:insert(codepoint, codepoint)
    end
end

local double_width = RangeTable.new()
for min, max in eaw:gmatch "\n(%x+)%.*(%x*);[WF]" do
    min = assert(tonumber(min, 16))
    max = tonumber(max, 16)
    double_width:insert(min, max or min)
end

zero_width:merge_adjacent():print_ranges("zero_width", io.stdout)
double_width:merge_adjacent():print_ranges("double_width", io.stdout)
