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
    local trie = {}
    for key, value in pairs(keys) do
        local key_length = #key
        local node = trie[key_length]
        if not node then
            node = {type = "branch", child_count = 0}
            trie[key_length] = node
        end

        for i = 1, key_length do
            local byte = assert(key:sub(i, i):byte())
            local min, max = node.min, node.max
            if not min or byte < min then
                node.min = byte
            end
            if not max or byte > max then
                node.max = byte
            end

            if i == key_length then
                assert(node[byte] == nil)
                node[byte] = {type = "leaf", key = key, value = value}
                node.child_count = node.child_count + 1
            else
                if not node[byte] then
                    node[byte] = {type = "branch", child_count = 0}
                    node.child_count = node.child_count + 1
                end
                node = node[byte]
            end
        end
    end
    return trie
end

local function compress(node)
    local skip = 0
    while node.type == "branch" do
        if node.child_count > 1 then
            if skip > 1 then
                return skip, node
            end
            return false
        end
        skip = skip + 1
        node = node[node.min]
    end
    return skip, node
end

local function make_index(keys)
    local index, n, longest = {}, 0, 0
    for k in pairs(keys) do
        assert(k:match("^[#-~]+$"), k)
        n = n + 1
        index[n] = k
        local klen = #k
        if klen > longest then
            longest = klen
        end
    end
    table.sort(index)
    for i = 1, n do
        local key = index[i]
        index[key] = i
    end
    index.n = n
    index.longest_key = longest
    return index
end

local function write_index(index, buf, defs)
    local key_type
    if index.longest_key <= 8 then
        key_type = "const char key[8]"
    elseif index.longest_key <= 16 then
        key_type = "const char key[16]"
    else
        key_type = "const char *const key"
    end
    local n = #index
    buf:write (
        "static const struct {\n",
        "    ", key_type, ";\n",
        "    const ", defs.return_type, " val;\n",
        "} ", defs.function_name, "_table[", tostring(n), "] = {\n"
    )
    for i = 1, n do
        local k = index[i]
        buf:write('    {"', k, '", ', defs.keys[k], "},\n")
    end
    buf:write("};\n\n")
end

local function write_trie(node, buf, defs, index)
    local default_return = defs.default_return
    local indents = Indent.new(4)
    local function serialize(node, level, indent_depth)
        local indent = indents[indent_depth]
        if node.type == "branch" then
            local skip, n = compress(node)
            if skip then
                if n.type == "branch" then
                    node = n
                    level = level + skip
                else
                    assert(n.type == "leaf")
                    buf:write(" CMP(", index[n.key] - 1, "); // ", n.key, "\n")
                    return
                end
            end

            buf:write("\n", indent, "switch (s[", tostring(level), "]) {\n")
            for i = node.min, node.max do
                local v = node[i]
                if v then
                    buf:write(indent, "case '", char(i), "':")
                    serialize(v, level + 1, indent_depth + 1)
                end
            end
            buf:write(indent, "}\n", indent, "break;\n")
        else
            assert(node.type == "leaf")
            buf:write(" return ", node.value, ";\n")
        end
    end

    local indent = indents[1]
    buf:write(indent, "switch (len) {\n")
    for i = 1, index.longest_key do
        local v = node[i]
        if v then
            buf:write(indent, "case ", tostring(i), ":")
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
local index = assert(make_index(keys))
local trie = assert(make_trie(keys))

local prelude = [[
#define CMP(i) idx = i; goto compare

static %s %s(const char *s, size_t len)
{
    size_t idx;
    const char *key;
    %s val;
]]

local postlude = [[
compare:
    key = %s_table[idx].key;
    val = %s_table[idx].val;
    return memcmp(s, key, len) ? %s : val;
}

#undef CMP
]]

write_index(index, output, defs)
output:write(prelude:format(rtype, fname, rtype))
write_trie(trie, output, defs, index)
output:write(postlude:format(fname, fname, rdefault))
output:flush()
