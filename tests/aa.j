
constant native OrderId                     takes string  orderIdString     returns integer
constant native OrderId2String              takes integer orderId           returns string
constant native UnitId2String               takes integer unitId            returns string
constant native GetObjectName               takes integer objectId          returns string

globals 
   // integer test1 = OrderId("")
    string str2 = GetObjectName("ss")

    constant integer test2 = 1
    code c
endglobals 

constant native first takes nothing returns integer 
 function bb takes nothing returns integer
    return 0
endfunction 

constant function aa takes nothing returns integer
    call bb()
    return 0
endfunction 

    

function test5 takes handle test2, handle ab returns integer 
    local string test3 
    local integer num = aa()
    //set i = i[1] + 20

    //local string array buf

    //local string s =  "a" + test2()
    return 0
endfunction



    

//globals
//    integer a 
//    integer array b 
//
//endglobals
//
//
//function bbb takes handle h, handle xx returns nothing
//
//endfunction 


