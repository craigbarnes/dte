local concat = table.concat
local Buffer = {}
Buffer.__index = Buffer

function Buffer:write(...)
    local length = self.length
    for i = 1, select("#", ...) do
        length = length + 1
        self[length] = select(i, ...)
    end
    self.length = length
end

function Buffer:tostring()
    return concat(self)
end

local function new_buffer()
    return setmetatable({length = 0}, Buffer)
end

setmetatable(Buffer, {__call = new_buffer})

local function escape(s, in_attribute)
    return (s:gsub("[\\-]", "\\%1"))
end

local toc = Buffer()

function Doc(body, metadata, variables)
    local title = assert(metadata.title)
    local section = assert(metadata.section)
    local description = assert(metadata.description)
    local date = assert(metadata.date)
    local seealso = assert(metadata.seealso)
    local authors = assert(metadata.author)
    local buffer = Buffer()
    buffer:write (
        ".TH ", title:upper(), " ", section, ' "', date, '"\n',
        ".nh\n.ad l\n.\n",
        ".SH NAME\n", title, " \\- ", description, "\n",
        ".SH SYNOPSIS\n", toc:tostring(),
        body,
        ".SH SEE ALSO\n", concat(seealso, ",\n"),
        "\n.SH AUTHORS\n", concat(authors, "\n.br\n")
    )
    return buffer:tostring()
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
            return ".SH DESCRIPTION\n"
        end
        if generate_toc then
            if s == "Commands" then
                toc:write(".P\n", s, ":\n.br\n")
                in_toc_heading = true
            elseif s == "Options" then
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
        ["dte-syntax"] = "(5)"
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

--[[ Not implemented:

function Subscript(s)
function Superscript(s)
function SmallCaps(s)
function Strikeout(s)
function Image(s, src, tit, attr)
function InlineMath(s)
function DisplayMath(s)
function Note(s)
function Span(s, attr)
function RawInline(format, str)
function Table(caption, aligns, widths, headers, rows)
function BlockQuote(s)
function HorizontalRule()
function LineBlock(ls)
function CaptionedImage(src, tit, caption, attr)
function RawBlock(output_format, str)
function Div(s, attr)
function Cite(s, cs)
function DoubleQuoted(s)
--]]
