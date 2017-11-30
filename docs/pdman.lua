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
    return (s:gsub("[\\-]", {
        ["-"] = "\\-",
        ["\\"] = "\\\\"
    }))
end

local toc = Buffer()

function Doc(body, metadata, variables)
    local title = assert(metadata.title)
    local section = assert(metadata.section)
    local description = assert(metadata.description)
    local date = assert(metadata.date)
    local authors = assert(metadata.author)
    local buffer = Buffer()
    buffer:write (
        ".TH ", escape(title:upper()), " ", section, ' "', date, '"\n',
        ".nh\n.ad l\n.\n",
        ".SH NAME\n", title, " \\- ", description, "\n",
        ".SH SYNOPSIS\n", toc:tostring(),
        body,
        ".SH AUTHORS\n", concat(authors, "\n.br\n")
    )
    return buffer:tostring()
end

local in_toc_heading = false

function Header(level, s, attr)
    assert(level < 4)
    if level == 1 then
        if s:find("Commands", 1, true) then
            toc:write(".P\n", s, ":\n.br\n")
            in_toc_heading = true
        elseif s:find("Options", 1, true) then
            in_toc_heading = true
        else
            in_toc_heading = false
        end
        return ".SH " .. s:upper() .. "\n"
    elseif level == 2 then
        if in_toc_heading then
            toc:write("\n.P\n", s, ":\n.br\n")
        end
        return ".SS " .. s .. "\n"
    elseif level == 3 then
        if in_toc_heading then
            toc:write("   ", s, "\n.br\n")
        end
        return "\n.RE\n" .. s .. "\n.RS"
    end
end

function DefinitionList(items)
    local buffer = Buffer()
    for i, item in ipairs(items) do
        local term = assert(item[1])
        local definitions = assert(item[2])
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
    return buffer:tostring()
end

function CodeBlock(s, attr)
    return ".RS\n" .. s .. "\n.RE\n.P\n"
end

function Code(s, attr)
    return "\\fB" .. escape(s) .. "\\fR"
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
    return "\n"
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
