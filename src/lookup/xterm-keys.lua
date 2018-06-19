local keys = {
    -- CSI
    ["[A"] = "KEY_UP",
    ["[B"] = "KEY_DOWN",
    ["[C"] = "KEY_RIGHT",
    ["[D"] = "KEY_LEFT",
    ["[F"] = "KEY_END",
    ["[H"] = "KEY_HOME",
    ["[L"] = "KEY_INSERT",
    ["[Z"] = "MOD_SHIFT | '\\t'",

    ["[1~"] = "KEY_HOME",
    ["[2~"] = "KEY_INSERT",
    ["[3~"] = "KEY_DELETE",
    ["[4~"] = "KEY_END",
    ["[5~"] = "KEY_PAGE_UP",
    ["[6~"] = "KEY_PAGE_DOWN",

    -- These 4 sequences are "deprecated" in xterm, even though they
    -- were used by default in older releases. They are still used by
    -- some other, xterm-like terminal emulators and can still be
    -- activated in xterm by setting the "oldXtermFKeys" resource.
    ["[11~"] = "KEY_F1",
    ["[12~"] = "KEY_F2",
    ["[13~"] = "KEY_F3",
    ["[14~"] = "KEY_F4",

    ["[15~"] = "KEY_F5",
    ["[17~"] = "KEY_F6",
    ["[18~"] = "KEY_F7",
    ["[19~"] = "KEY_F8",
    ["[20~"] = "KEY_F9",
    ["[21~"] = "KEY_F10",
    ["[23~"] = "KEY_F11",
    ["[24~"] = "KEY_F12",

    -- SS3
    ["OA"] = "KEY_UP",
    ["OB"] = "KEY_DOWN",
    ["OC"] = "KEY_RIGHT",
    ["OD"] = "KEY_LEFT",
    ["OF"] = "KEY_END",
    ["OH"] = "KEY_HOME",
    ["OI"] = "'\\t'",
    ["OM"] = "'\\r'",
    ["OP"] = "KEY_F1",
    ["OQ"] = "KEY_F2",
    ["OR"] = "KEY_F3",
    ["OS"] = "KEY_F4",
    ["O "] = "' '",

    ["Oj"] = "'*'",
    ["Ok"] = "'+'",
    ["Om"] = "'-'",
    ["Oo"] = "'/'",
}

local key_templates = {
    ["[1;_A"] = "KEY_UP",
    ["[1;_B"] = "KEY_DOWN",
    ["[1;_C"] = "KEY_RIGHT",
    ["[1;_D"] = "KEY_LEFT",
    ["[1;_F"] = "KEY_END",
    ["[1;_H"] = "KEY_HOME",
    ["[2;_~"] = "KEY_INSERT",
    ["[3;_~"] = "KEY_DELETE",
    ["[5;_~"] = "KEY_PAGE_UP",
    ["[6;_~"] = "KEY_PAGE_DOWN",
}

local modifiers = {
    ["2"] = "MOD_SHIFT",
    ["3"] = "MOD_META",
    ["4"] = "MOD_SHIFT | MOD_META",
    ["5"] = "MOD_CTRL",
    ["6"] = "MOD_SHIFT | MOD_CTRL",
    ["7"] = "MOD_META | MOD_CTRL",
    ["8"] = "MOD_SHIFT | MOD_META | MOD_CTRL",
}

-- Expand key templates with modifier combos and insert into keys table
for template, key in pairs(key_templates) do
    for num, mod in pairs(modifiers) do
        local seq = assert(template:gsub("_", num))
        keys[seq] = mod .. " | " .. key
    end
end

-- Build a trie structure from tables, indexed by numeric byte values
local trie = {}
for seq, key in pairs(keys) do
    local node = trie
    local n = #seq
    for i = 1, n do
        local char = assert(seq:sub(i, i))
        local byte = assert(char:byte())
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

-- Create a caching lookup table of indent strings
local indents = setmetatable({[0] = "", [1] = "    "}, {
    __index = function(self, depth)
        assert(depth < 10)
        local indent = self[1]:rep(depth)
        self[depth] = indent
        return indent
    end
})

local function has_only_tilde_leaf(node)
    for i = ("\0"):byte(), ("}"):byte() do
        if node[i] then
            return false
        end
    end
    return type(node[("~"):byte()]) == "string"
end

-- Iterate over trie nodes and generate equivalent, nested C switch statements
local function write_trie(node, buffer)
    local buf = buffer or Buffer()
    local function serialize(node, depth)
        local indent = indents[depth]
        if type(node) == "table" then
            if has_only_tilde_leaf(node) then
                buf:write (
                    -- This should really set a temporary and decide whether
                    -- or not to set *k after the goto, but for some reason
                    -- GCC bloats the jump table by 100 extra instructions.
                    indent, "*k = ", node[("~"):byte()], ";\n",
                    indent, "goto check_trailing_tilde;\n"
                )
                return
            end
            buf:write (
                -- Return -1 to indicate truncation, if buffer matches a
                -- prefix but isn't long enough to form a complete sequence
                indent, "if (i >= length) return -1;\n",
                indent, "switch(buf[i++]) {\n"
            )
            for i = ("\0"):byte(), ("~"):byte() do
                local v = node[i]
                if v then
                    buf:write(indent, "case '", string.char(i), "':\n")
                    serialize(v, depth + 1)
                end
            end
            buf:write(indent, "}\n", indent, "return 0;\n")
        else
            buf:write (
                indent, "*k = ", node, ";\n",
                indent, "return i;\n"
            )
        end
    end
    serialize(node, 1)
end

local output = assert(io.stdout)

output:write [[
// Escape sequence parser for xterm function keys.
// Copyright 2018 Craig Barnes.
// SPDX-License-Identifier: GPL-2.0-only
// Generated by: src/lookup/xterm-keys.lua
// See also: https://invisible-island.net/xterm/ctlseqs/ctlseqs.html

static ssize_t parse_xterm_key_sequence(const char *buf, size_t length, Key *k)
{
    if (length == 0 || buf[0] != '\033') {
        return 0;
    }
    size_t i = 1;
]]

write_trie(trie, output)

output:write [[
check_trailing_tilde:
    if (i >= length) {
        return -1;
    } else if (buf[i++] == '~') {
        return i;
    }
    return 0;
}
]]
