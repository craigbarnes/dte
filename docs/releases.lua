-- This Pandoc filter modifies link anchors in public/releases.html so
-- that the heading for e.g. "v1.8.2" has the anchor `#v1.8.2` and not
-- Pandoc's (ambiguous) default of `#v182`

local function Pandoc(doc)
    local t1 = assert(doc.meta.title[1].text)
    local t3 = assert(doc.meta.title[3].text)
    if t1 ~= "dte" or t3 ~= "Releases" or doc.meta.title[4] then
        local msg = "file-specific filter used on unknown document"
        error(msg:format(title))
    end
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
