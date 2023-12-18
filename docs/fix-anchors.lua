-- This Pandoc filter modifies link anchors for section headings
-- in public/dterc.html and public/dte-syntax.html, so that they
-- remain stable over time

local allowed_docs = {
    ["dterc"] = true,
    ["dte-syntax"] = true,
}

local h2

local function Pandoc(doc)
    local title = assert(doc.meta.title[1].text)
    if not allowed_docs[title] or doc.meta.title[2] then
        local msg = "file-specific filter used on unknown document: '%s'"
        error(msg:format(title))
    end
end

local function Header(header)
    local level = assert(header.level)
    local id = assert(header.attr.identifier)
    assert(level >= 1)
    assert(#id >= 1)

    if level <= 2 then
        h2 = (level == 2) and id or nil
        return header
    end

    -- Uppercase all `<h3>` anchors in the "special variables" section,
    -- so that the one for e.g. `$FILETYPE` doesn't clash with the one
    -- for the `filetype` local option.
    if level == 3 and h2 == "special-variables" then
        header.attr.identifier = id:upper()
        return header
    end

    -- Remove trailing junk from other <h3> anchors (like command flags
    -- and arguments), so that e.g. "include--bq-file" becomes "include".
    if level == 3 and id:find("%-") then
        local new_id = assert(header.content[1].content[1].text)
        if #new_id < #id and new_id == id:sub(1, #new_id) then
            header.attr.identifier = new_id
            return header
        end
    end

    return header
end

return {
    {
        Header = Header,
        Pandoc = Pandoc,
    }
}
