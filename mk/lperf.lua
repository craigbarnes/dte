local char, concat = string.char, table.concat
local Indent = {}

function Indent:__index(depth)
    assert(depth < 12)
    local indent = self[1]:rep(depth)
    self[depth] = indent
    return indent
end

function Indent.new(width)
    local i1 = (" "):rep(width or 4)
    return setmetatable({[0] = "", [1] = i1}, Indent)
end

local function make_trie(keys)
    local trie = {n = 0}
    for seq, key in pairs(keys) do
        local n = #seq
        if n > trie.n then
            trie.n = n
        end
        if not trie[n] then
            trie[n] = {}
        end
        local node = trie[n]
        for i = 1, n do
            local byte = assert(seq:sub(i, i):byte())
            if i == n then
                -- Leaf node
                assert(node[byte] == nil, "Clashing sequence: " .. seq)
                node[byte] = key
                break
            elseif type(node[byte]) ~= "table" then
                assert(node[byte] == nil, "Clashing sequence: " .. seq)
                node[byte] = {}
            end
            node = node[byte]
        end
    end
    return trie
end

local function compress(node)
    local buf, n = {}, 0
    while type(node) == "table" do
        local k = next(node)
        if next(node, k) then
            if n > 1 then
                return concat(buf), node
            end
            return false
        end
        n = n + 1
        buf[n] = char(k)
        node = node[k]
    end
    return concat(buf), node
end

local function memcmp(offset, str)
    local length = #str
    if length == 1 then
        return ("(s[%u] != '%s')"):format(offset, str)
    elseif offset == 0 then
        return ('memcmp(s, "%s", %u)'):format(str, length)
    end
    return ('memcmp(s + %d, "%s", %u)'):format(offset, str, length)
end

local function write_trie(node, buf, default_return)
    local indents = Indent.new(4)
    local function serialize(node, level, indent_depth)
        local indent = indents[indent_depth]
        if type(node) == "table" then
            local s, n = compress(node)
            if s then
                if type(n) == "table" then
                    buf:write (
                        indent, "if (", memcmp(level, s), ") {\n",
                        indent, "    return ", default_return, ";\n",
                        indent, "}\n"
                    )
                    node = n
                    level = level + #s
                else
                    buf:write (
                        indent, "return ", memcmp(level, s), " ? ",
                        default_return, " : ", n, ";\n"
                    )
                    return
                end
            end

            buf:write(indent, "switch (s[", tostring(level), "]) {\n")
            for i = ("0"):byte(), ("z"):byte() do
                local v = node[i]
                if v then
                    buf:write(indent, "case '", char(i), "':\n")
                    serialize(v, level + 1, indent_depth + 1)
                end
            end
            buf:write(indent, "}\n", indent, "return ", default_return, ";\n")
        else
            buf:write(indent, "return ", node, ";\n")
        end
    end

    local indent = indents[1]
    buf:write(indent, "switch (len) {\n")
    for i = 1, node.n do
        local v = node[i]
        if v then
            buf:write(indent, "case ", tostring(i), ":\n")
            serialize(v, 0, 2)
        end
    end
    buf:write(indent, "}\n", indent, "return ", default_return, ";\n")
end

local input_filename = assert(arg[1], "Usage: " .. arg[0] .. " INPUT-FILE")
local output_filename, output = arg[2]
if output_filename then
    output = assert(io.open(output_filename, "w"))
    output:setvbuf("full")
else
    output = assert(io.stdout)
end

local fn = assert(loadfile(input_filename, "t", {}))
local defs = assert(fn())
local keys = assert(defs.keys, "No keys defined")
local fname = assert(defs.function_name, "No function_name defined")
local rtype = assert(defs.return_type, "No return_type defined")
local rdefault = assert(defs.default_return, "No default_return defined")

local headers = "#include <stddef.h>\n#include <string.h>\n\n"
local proto = "%sstatic %s %s(const char *s, size_t len)\n{\n"
local prelude = proto:format(defs.includes and headers or "", rtype, fname)

local trie = assert(make_trie(keys))
output:write(prelude)
write_trie(trie, output, rdefault)
output:write("}\n")
output:flush()
