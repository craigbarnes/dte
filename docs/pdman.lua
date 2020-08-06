local concat = table.concat
local Buffer = {}
Buffer.__index = Buffer

function Buffer:write(...)
    local n = self.n
    for i = 1, select("#", ...) do
        n = n + 1
        self[n] = select(i, ...)
    end
    self.n = n
end

function Buffer:tostring()
    return concat(self)
end

local function new_buffer()
    return setmetatable({n = 0}, Buffer)
end

setmetatable(Buffer, {__call = new_buffer})

local function escape(s, in_attribute)
    return (s:gsub("[\\-]", "\\%1"))
end

local toc = Buffer()

local template = [[
.TH %s %s "%s"
.nh
.ad l
.
.SH NAME
%s \- %s
%s%s.
.SH SEE ALSO
%s
.SH AUTHORS
%s]]

function Doc(body, metadata, variables)
    local title = assert(metadata.title)
    local section = assert(metadata.section)
    local description = assert(metadata.description)
    local date = assert(metadata.date)
    local seealso = assert(metadata.seealso)
    local authors = assert(metadata.author)
    return template:format (
        title:upper(), section, date,
        title, description:gsub(" dte", " \\fBdte\\fR(1)"),
        toc:tostring(), body,
        concat(seealso, ",\n"),
        concat(authors, "\n.br\n")
    )
end

local generate_toc = false
local in_toc_heading = false
local prev_heading_level = 0

function Header(level, s, attr)
    assert(level < 4)

    local prefix = ""
    if prev_heading_level > 2 then
        prefix = ".RE\n"
    end
    prev_heading_level = level

    if level == 1 then
        if s == "dterc" or s == "dte\\-syntax" then
            generate_toc = true
            toc:write(".SH SYNOPSIS\n")
            return ".SH DESCRIPTION\n"
        end
        if generate_toc then
            if s == "Commands" or s == "Options" then
                toc:write(".P\n", s, ":\n.br\n")
                in_toc_heading = true
            else
                in_toc_heading = false
            end
        end
        return prefix .. ".SH " .. s:upper() .. "\n"
    elseif level == 2 then
        if in_toc_heading then
            toc:write(".P\n", s, ":\n.br\n")
        end
        return prefix .. ".SS " .. s .. "\n"
    elseif level == 3 then
        if in_toc_heading then
            toc:write("   ", s, "\n.br\n")
        end
        return prefix .. s .. "\n.RS\n"
    end
end

function DefinitionList(items)
    local buffer = Buffer()
    for i, item in ipairs(items) do
        local term, definitions = next(item)
        assert(type(term) == "string")
        assert(type(definitions) == "table")
        assert(#definitions == 1)
        local definition = assert(definitions[1])
        buffer:write(".TP\n", term, "\n", definition, "\n.PP\n")
    end
    return buffer:tostring()
end

function BulletList(items)
    local buffer = Buffer()
    for i, item in ipairs(items) do
        buffer:write("\\(bu ", item, "\n.br\n")
    end
    buffer:write(".P\n")
    return buffer:tostring()
end

function OrderedList(items)
    local buffer = Buffer()
    for i, item in ipairs(items) do
        buffer:write(tostring(i), ". ", item, "\n.br\n")
    end
    buffer:write(".P\n")
    return (buffer:tostring():gsub("\n\n+", "\n.\n"))
end

function CodeBlock(s, attr)
    local code = s:gsub("[ \\-]", "\\%1")
    return ".IP\n.nf\n\\f[C]\n" .. code .. "\n\\f[]\n.fi\n.PP\n"
end

function Code(s, attr)
    local crossrefs = {
        dte = "(1)",
        dterc = "(5)",
        ["dte-syntax"] = "(5)",
        execvp = "(3)",
        glob = "(7)",
        regex = "(7)",
        stdout = "(3)",
        stderr = "(3)",
        sysexits = "(3)",
        ctags = "(1)",
        fmt = "(1)",
        terminfo = "(5)",
    }
    return "\\fB" .. escape(s) .. "\\fR" .. (crossrefs[s] or "")
end

function Strong(s)
    return "\\fB" .. s .. "\\fR"
end

function Emph(s)
    return "\\fI" .. s .. "\\fR"
end

function Link(text, href, title, attr)
    return text
end

function Str(s)
    return escape(s)
end

function Para(s)
    return s .. "\n.P\n"
end

function Plain(s)
    return s
end

function Blocksep()
    return ""
end

function LineBreak()
    return "\n.br\n"
end

function SoftBreak()
    return "\n"
end

function Space()
    return " "
end

setmetatable(_G, {
    __index = function(_, key)
        error(("Error: undefined function '%s'"):format(key))
    end
})

--[[
Not implemented:
* Subscript(s)
* Superscript(s)
* SmallCaps(s)
* Strikeout(s)
* Image(s, src, title, attr)
* InlineMath(s)
* DisplayMath(s)
* Note(s)
* Span(s, attr)
* RawInline(format, str)
* Table(caption, aligns, widths, headers, rows)
* BlockQuote(s)
* HorizontalRule()
* LineBlock(ls)
* CaptionedImage(src, title, caption, attr)
* RawBlock(output_format, str)
* Div(s, attr)
* Cite(s, cs)
* DoubleQuoted(s)
--]]
