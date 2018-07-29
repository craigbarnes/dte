local xterm_keys = {
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

    -- Linux console
    ["[[A"] = "KEY_F1",
    ["[[B"] = "KEY_F2",
    ["[[C"] = "KEY_F3",
    ["[[D"] = "KEY_F4",
    ["[[E"] = "KEY_F5",
}

local bracket1_impl = [=[
if (i >= length) {
    return -1;
} else {
    const KeyCode mods = mod_enum_to_mod_mask(buf[i++]);
    if (mods == 0) {
        return 0;
    } else if (i >= length) {
        return -1;
    }
    switch(buf[i++]) {
    case 'A':
        *k = mods | KEY_UP;
        return i;
    case 'B':
        *k = mods | KEY_DOWN;
        return i;
    case 'C':
        *k = mods | KEY_RIGHT;
        return i;
    case 'D':
        *k = mods | KEY_LEFT;
        return i;
    case 'F':
        *k = mods | KEY_END;
        return i;
    case 'H':
        *k = mods | KEY_HOME;
        return i;
    }
}
return 0;
]=]

local template = [[
if (i >= length) {
    return -1;
} else {
    const KeyCode mods = mod_enum_to_mod_mask(buf[i++]);
    if (mods == 0) {
        return 0;
    }
    *k = mods | %s;
    goto check_trailing_tilde;
}
]]

local function make_generator_node(impl)
    return function(buf, indent)
        for line in impl:gmatch("[^\n]+") do
            buf:write(indent, line, "\n")
        end
    end
end

xterm_keys["[1;"] = make_generator_node(bracket1_impl)
xterm_keys["[2;"] = make_generator_node(template:format("KEY_INSERT"))
xterm_keys["[3;"] = make_generator_node(template:format("KEY_DELETE"))
xterm_keys["[5;"] = make_generator_node(template:format("KEY_PAGE_UP"))
xterm_keys["[6;"] = make_generator_node(template:format("KEY_PAGE_DOWN"))

-- Build a trie structure from tables, indexed by numeric byte values
local function make_trie(keys)
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
    return trie
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
        local node_type = type(node)
        if node_type == "table" then
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
        elseif node_type == "function" then
            node(buf, indent)
        else
            assert(node_type == "string")
            buf:write (
                indent, "*k = ", node, ";\n",
                indent, "return i;\n"
            )
        end
    end
    serialize(node, 1)
end

local xterm_trie = make_trie(xterm_keys)
local output = assert(io.stdout)

output:write [[
// Escape sequence parser for xterm function keys.
// Copyright 2018 Craig Barnes.
// SPDX-License-Identifier: GPL-2.0-only
// Generated by: src/lookup/xterm-keys.lua
// See also: https://invisible-island.net/xterm/ctlseqs/ctlseqs.html

#include <sys/types.h>
#include "../key.h"

static KeyCode mod_enum_to_mod_mask(char mod_enum)
{
    switch (mod_enum) {
    case '2': return MOD_SHIFT;
    case '3': return MOD_META;
    case '4': return MOD_SHIFT | MOD_META;
    case '5': return MOD_CTRL;
    case '6': return MOD_SHIFT | MOD_CTRL;
    case '7': return MOD_META | MOD_CTRL;
    case '8': return MOD_SHIFT | MOD_META | MOD_CTRL;
    default:  return 0;
    }
}

static ssize_t parse_xterm_key(const char *buf, size_t length, KeyCode *k)
{
    if (length == 0 || buf[0] != '\033') {
        return 0;
    }
    size_t i = 1;
]]

write_trie(xterm_trie, output)

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
