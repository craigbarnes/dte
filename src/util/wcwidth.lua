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
    local n = assert(self.n)
    output:write("static const CodepointRange ", name, "[] = {\n")
    for i = 1, n do
        local min = assert(self[i].min)
        local max = assert(self[i].max)
        output:write(("    {0x%04x, 0x%04x},\n"):format(min, max))
    end
    output:write("};\n\n")
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

local nonspacing_mark = RangeTable.new()
local special_whitespace = RangeTable.new()
local unprintable = RangeTable.new()
local double_width = RangeTable.new()
local exclude = RangeTable.new()

local mappings = {
    Zs = special_whitespace, -- Space_Separator (various non-zero width spaces)
    Zl = special_whitespace, -- Line_Separator (U+2028 only)
    Zp = special_whitespace, -- Paragraph_Separator (U+2029 only)
    Cc = unprintable, -- Control
    Cf = unprintable, -- Format
    Cs = unprintable, -- Surrogate
    Co = unprintable, -- Private_Use
    Me = nonspacing_mark, -- Enclosing_Mark
    Mn = nonspacing_mark, -- Nonspacing_Mark
    [0x0020] = exclude, -- ASCII space (Zs)
    [0x00AD] = special_whitespace, -- Soft hyphen (Cf)
    [0x1680] = exclude, -- Ogham space mark (Zs)
    [0x3000] = exclude, -- Ideographic space (Zs)
}

-- ASCII
for u = 0x00, 0x7F do
    mappings[u] = exclude
end

local prev_codepoint = -1
local range = false
for codepoint, name, category in unidata:gmatch "(%x+);([^;]*);(%u%a);[^\n]*\n" do
    codepoint = assert(tonumber(codepoint, 16))
    assert(codepoint > prev_codepoint)
    assert(category)
    local t = mappings[codepoint] or mappings[category]
    if t then
        if range then
            assert(name:match("^<.*, Last>$"))
            range = false
            t:insert(prev_codepoint, codepoint)
        elseif name:match("^<.*, First>$") then
            range = true
        else
            t:insert(codepoint, codepoint)
        end
    end
    prev_codepoint = codepoint
end

assert(prev_codepoint == 0x10FFFD)
assert(exclude.n == 127 + 3)

for min, max in eaw:gmatch "\n(%x+)%.*(%x*);[WF]" do
    min = assert(tonumber(min, 16))
    max = tonumber(max, 16)
    double_width:insert(min, max or min)
end

local stdout = io.stdout
special_whitespace:merge_adjacent():print_ranges("special_whitespace", stdout)
unprintable:merge_adjacent():print_ranges("unprintable", stdout)
nonspacing_mark:merge_adjacent():print_ranges("nonspacing_mark", stdout)
double_width:merge_adjacent():print_ranges("double_width", stdout)
