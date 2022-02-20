
type unit extends handle


function func takes nothing returns integer 
    return 0
endfunction 


function test5 takes handle test2, handle ab returns integer 
    local boolean b = true

    if true then
        return ab
        return 0
    else     
        return function func
        return ab
    endif 

    return "aaa"

endfunction

function foo takes handle h returns unit
	return h
	return null
endfunction

   