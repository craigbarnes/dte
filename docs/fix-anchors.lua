-- This Pandoc filter modifies headings, so that the link anchors
-- for dterc commands don't include trailing junk like flags and
-- parameters. For example "include--bq-file" becomes "include".

local function get_canonical_id(header)
    return header.content[1].content[1].text
end

local section

local function Header(header)
    local level = assert(header.level)
    local id = header.attr.identifier

    if level == 2 then
        section = assert(id)
        return header
    end

    if level == 3 and section == "special-variables" then
        header.attr.identifier = id:upper()
        return header
    end

    if level == 3 and id:find("%-") then
        local ok, new_id = pcall(get_canonical_id, header)
        if ok and #new_id < #id and new_id == id:sub(1, #new_id) then
            header.attr.identifier = new_id
        end
    end

    return header
end

return {
    {Header = Header}
}
