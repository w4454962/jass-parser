local tableSort    = table.sort
local stringRep    = string.rep
local tableConcat  = table.concat
local tostring     = tostring
local type         = type
local pairs        = pairs
local ipairs       = ipairs
local next         = next
local rawset       = rawset
local move         = table.move
local setmetatable = setmetatable
local mathType     = math.type
local mathCeil     = math.ceil
local getmetatable = getmetatable
local mathAbs      = math.abs
local ioOpen       = io.open

local function formatNumber(n)
    local str = ('%.10f'):format(n)
    str = str:gsub('%.?0*$', '')
    return str
end

local function isInteger(n)
    if mathType then
        return mathType(n) == 'integer'
    else
        return type(n) == 'number' and n % 1 == 0
    end
end



local TAB = setmetatable({}, { __index = function (self, n)
    if is_pack_code then
        return ''
    end
    self[n] = stringRep('    ', n)
    return self[n]
end})

local RESERVED = {
    ['and']      = true,
    ['break']    = true,
    ['do']       = true,
    ['else']     = true,
    ['elseif']   = true,
    ['end']      = true,
    ['false']    = true,
    ['for']      = true,
    ['function'] = true,
    ['goto']     = true,
    ['if']       = true,
    ['in']       = true,
    ['local']    = true,
    ['nil']      = true,
    ['not']      = true,
    ['or']       = true,
    ['repeat']   = true,
    ['return']   = true,
    ['then']     = true,
    ['true']     = true,
    ['until']    = true,
    ['while']    = true,
}


local function dump(tbl, option)
    if not option then
        option = { }

        if is_pack_code then
            option['noArrayKey'] = true
        end
    end
    if type(tbl) ~= 'table' then
        return ('%s'):format(tbl)
    end
    local lines = {}
    local mark = {}
    lines[#lines+1] = '{'
    local function unpack(tbl, tab)
        mark[tbl] = (mark[tbl] or 0) + 1
        local keys = {}
        local keymap = {}
        local integerFormat = '[%d]'
        local alignment = 0
        if #tbl >= 10 then
            local width = #tostring(#tbl)
            integerFormat = ('[%%0%dd]'):format(mathCeil(width))
        end
        for key in pairs(tbl) do
            if type(key) == 'string' then
                if not key:match('^[%a_][%w_]*$')
                or RESERVED[key]
                or option['longStringKey']
                then
                    keymap[key] = ('[%q]'):format(key)
                else
                    keymap[key] = ('%s'):format(key)
                end
            elseif isInteger(key) then
                keymap[key] = integerFormat:format(key)
            else
                keymap[key] = ('["<%s>"]'):format(tostring(key))
            end
            keys[#keys+1] = key
            if option['alignment'] then
                if #keymap[key] > alignment then
                    alignment = #keymap[key]
                end
            end
        end
        local mt = getmetatable(tbl)
        if not mt or not mt.__pairs then
            if option['sorter'] then
                option['sorter'](keys, keymap)
            else
                tableSort(keys, function (a, b)
                    return keymap[a] < keymap[b]
                end)
            end
        end
        for index, key in ipairs(keys) do
            local keyWord = keymap[key]
            if option['noArrayKey']
                and isInteger(key)
                and key <= #tbl
            then
                keyWord = ''
            else
                if #keyWord < alignment then
                    keyWord = keyWord .. (' '):rep(alignment - #keyWord) .. ' = '
                else
                    keyWord = keyWord .. ' = '
                end
            end
            local value = tbl[key]
            local tp = type(value)
            local s = ','
            if index == #keys then
                s = ''
            end

            if option['format'] and option['format'][key] then
                lines[#lines+1] = ('%s%s%s%s'):format(TAB[tab+1], keyWord, option['format'][key](value, unpack, tab+1), s)
            elseif tp == 'table' then
                if mark[value] and mark[value] > 0 then
                    lines[#lines+1] = ('%s%s%s%s'):format(TAB[tab+1], keyWord, option['loop'] or '"<Loop>"', s)
                else
                    lines[#lines+1] = ('%s%s{'):format(TAB[tab+1], keyWord)
                    unpack(value, tab+1)
                    lines[#lines+1] = ('%s}%s'):format(TAB[tab+1], s)
                end
            elseif tp == 'string' then
                lines[#lines+1] = ('%s%s%q%s'):format(TAB[tab+1], keyWord, value, s)
            elseif tp == 'number' then
                lines[#lines+1] = ('%s%s%s%s'):format(TAB[tab+1], keyWord, (option['number'] or formatNumber)(value), s)
            elseif tp == 'nil' then
            else
                lines[#lines+1] = ('%s%s%s%s'):format(TAB[tab+1], keyWord, tostring(value), s)
            end
        end
        mark[tbl] = mark[tbl] - 1
    end
    unpack(tbl, 0)
    lines[#lines+1] = '}'
    local newline = '\r\n'
    if is_pack_code then
        newline = nil
    end
    return tableConcat(lines, newline)
end


return dump
