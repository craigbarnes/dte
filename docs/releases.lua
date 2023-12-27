-- This Pandoc filter modifies link anchors in public/releases.html so
-- that the heading for e.g. "v1.8.2" has the anchor `#v1.8.2` and not
-- Pandoc's (ambiguous) default of `#v182`

local function Pandoc(doc)
    local msg = "file-specific filter used on unknown document"
    local t = assert(doc.meta.title, msg)
    assert(t[1] and t[1].text == "dte", msg)
    assert(t[3] and t[3].text == "Releases", msg)
    assert(not t[4], msg)
end

local function Header(header)
    local level = assert(header.level)
    local id = assert(header.attr.identifier)
    assert(level >= 1)
    assert(#id >= 1)

    if level == 2 and id:find("^v[1-9]") then
        local new_id = assert(header.content[1].text)
        assert(new_id:find("^v[1-9][0-9.]+$"))
        header.attr.identifier = new_id
    end

    return header
end

return {
    {
        Header = Header,
        Pandoc = Pandoc,
    }
}
