local function make_generator_node(impl)
    return function(buf, indent)
        for line in impl:gmatch("[^\n]+") do
            buf:write(indent, line, "\n")
        end
    end
end

local function set_key_and_goto(key, label)
    local template = "tmp = %s;\ngoto %s;\n"
    return make_generator_node(template:format(key, label))
end

local xterm_keys = {
    ["[A"] = "KEY_UP",
    ["[B"] = "KEY_DOWN",
    ["[C"] = "KEY_RIGHT",
    ["[D"] = "KEY_LEFT",
    ["[F"] = "KEY_END",
    ["[H"] = "KEY_HOME",
    ["[L"] = "KEY_INSERT",
    ["[Z"] = "MOD_SHIFT | '\\t'",

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
    ["OX"] = "'='",
    ["Oj"] = "'*'",
    ["Ok"] = "'+'",
    ["Ol"] = "','",
    ["Om"] = "'-'",
    ["Oo"] = "'/'",
    ["O "] = "' '",

    ["[1~"] = "KEY_HOME",
    ["[2~"] = "KEY_INSERT",
    ["[4~"] = "KEY_END",

    -- Generators for Ctrl/Meta/Shift key combinations
    ["[1;"] = make_generator_node("return parse_csi1(buf, length, i, k);"),
    ["[2;"] = set_key_and_goto("KEY_INSERT", "check_modifiers"),
    ["[3"] = set_key_and_goto("KEY_DELETE", "check_delim"),
    ["[5"] = set_key_and_goto("KEY_PAGE_UP", "check_delim"),
    ["[6"] = set_key_and_goto("KEY_PAGE_DOWN", "check_delim"),
    ["[15"] = set_key_and_goto("KEY_F5", "check_delim"),
    ["[17"] = set_key_and_goto("KEY_F6", "check_delim"),
    ["[18"] = set_key_and_goto("KEY_F7", "check_delim"),
    ["[19"] = set_key_and_goto("KEY_F8", "check_delim"),
    ["[20"] = set_key_and_goto("KEY_F9", "check_delim"),
    ["[21"] = set_key_and_goto("KEY_F10", "check_delim"),
    ["[23"] = set_key_and_goto("KEY_F11", "check_delim"),
    ["[24"] = set_key_and_goto("KEY_F12", "check_delim"),

    -- These 4 sequences are "deprecated" in xterm, even though they
    -- were used by default in older releases. They are still used by
    -- some other, xterm-like terminal emulators and can still be
    -- activated in xterm by setting the "oldXtermFKeys" resource.
    ["[11~"] = "KEY_F1",
    ["[12~"] = "KEY_F2",
    ["[13~"] = "KEY_F3",
    ["[14~"] = "KEY_F4",

    -- Linux console
    ["[[A"] = "KEY_F1",
    ["[[B"] = "KEY_F2",
    ["[[C"] = "KEY_F3",
    ["[[D"] = "KEY_F4",
    ["[[E"] = "KEY_F5",
}

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
                    indent, "tmp = ", node[("~"):byte()], ";\n",
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
// Generated by: src/terminal/xterm-keys.lua
// See also: https://invisible-island.net/xterm/ctlseqs/ctlseqs.html

#include "xterm.h"

// These values are used in xterm escape sequences to indicate
// modifier+key combinations.
// See also: user_caps(5)
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
    }
    return 0;
}

static ssize_t parse_csi1(const char *buf, size_t length, size_t i, KeyCode *k)
{
    if (i >= length) {
        return -1;
    }
    const KeyCode mods = mod_enum_to_mod_mask(buf[i++]);
    if (mods == 0) {
        return 0;
    } else if (i >= length) {
        return -1;
    }
    switch(buf[i++]) {
    case 'A': *k = mods | KEY_UP; return i;
    case 'B': *k = mods | KEY_DOWN; return i;
    case 'C': *k = mods | KEY_RIGHT; return i;
    case 'D': *k = mods | KEY_LEFT; return i;
    case 'F': *k = mods | KEY_END; return i;
    case 'H': *k = mods | KEY_HOME; return i;
    case 'P': *k = mods | KEY_F1; return i;
    case 'Q': *k = mods | KEY_F2; return i;
    case 'R': *k = mods | KEY_F3; return i;
    case 'S': *k = mods | KEY_F4; return i;
    }
    return 0;
}

ssize_t xterm_parse_key(const char *buf, size_t length, KeyCode *k)
{
    if (length == 0 || buf[0] != '\033') {
        return 0;
    }
    KeyCode tmp;
    size_t i = 1;
]]

write_trie(xterm_trie, output)

output:write [[
check_delim:
    if (i >= length) return -1;
    switch(buf[i++]) {
    case ';':
        goto check_modifiers;
    case '~':
        *k = tmp;
        return i;
    }
    return 0;
check_modifiers:
    if (i >= length) {
        return -1;
    }
    const KeyCode mods = mod_enum_to_mod_mask(buf[i++]);
    if (mods == 0) {
        return 0;
    }
    tmp |= mods;
check_trailing_tilde:
    if (i >= length) {
        return -1;
    } else if (buf[i++] == '~') {
        *k = tmp;
        return i;
    }
    return 0;
}
]]
