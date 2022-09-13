#pragma once

#include "stdafx.h"

static const char peg_script[] = R"(

local re = require 'relabel'
local m = require 'lpeglabel'

local parser

timer_map = {}
count_map = {}

local defs = setmetatable({}, {__index = function (self, key)
    self[key] = function (...)
        if parser[key] then
            --print(key,select("#", ...), ...)

            local func = parser[key]
            
            local start = timer_start();
            
            local ret1 = func(...)
            
            local clock = timer_start()
            
            timer_map[key] = (timer_map[key] or 0) + (clock - start)
            count_map[key] = (count_map[key] or 0) + 1
            if ret1 then 
                return ret1
            end 
            --return parser[key](...)
        end
    end
    return self[key]
end})




local eof = re.compile '!. / %{SYNTAX_ERROR}'

return function (peg_script, jass, parser_)
    parser = parser_ or {}
    local defs = {}
    for k, v in pairs(parser) do 
        if defs[k] == nil then 
            defs[k] = v
        end 
    end 
   

    defs.nl = (m.P'\r\n' + m.S'\r\n') / (parser.nl or function () end)
    defs.Fail = function() return false end
    defs.True = m.Cc(true)
    defs.False = m.Cc(false)
    defs.s  = m.S' \t' + m.P'\xEF\xBB\xBF'
    defs.S  = - defs.s
    defs.eb = '\b'
    defs.et = '\t'
    defs.er = '\r'
    defs.en = '\n'
    defs.ef = '\f'
    defs.ExChar = m.R'\0\8' + m.R'\11\12' + m.R'\14\31' + m.R'\127\255'

    local compile = re.compile(peg_script, defs) * eof
    local r, e, pos = compile:match(jass)
    if not r then
        local errorpos = parser["errorpos"]
        if errorpos then 
            local row, col = re.rowcol(jass, pos)
            local str = re.line(jass, row)
            local err = errorpos(row, col, str, e)
            return nil, err
        end
    end
    return r
end 
 


)";
