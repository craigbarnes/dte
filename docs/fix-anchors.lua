-- This Pandoc filter modifies headings, so that the link anchors
-- for dterc commands don't include trailing junk like flags and
-- parameters. For example "include--bq-file" becomes "include".

local function get_canonical_id(header)
    return header.content[1].content[1].text
end

local function Header(header)
    local old_id = header.attr.identifier
    if old_id and header.level == 3 and old_id:find("%-") then
        local ok, id = pcall(get_canonical_id, header)
        if ok then
            if #old_id >= #id and id == old_id:sub(1, #id) then
                header.attr.identifier = id
            end
        end
    end
end

return {
    {Header = Header}
}
