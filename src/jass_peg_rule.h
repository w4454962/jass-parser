#pragma once

static const char jass_peg_rule[] = R"(

Jass        <-  ((Nl / Chunk)*)  Sp  {} -> Jass
                       
                
Chunk       <-  (   Type
                /   Globals
                /   Native
                /   Function
                -- 错误收集
                /   Action %{ERROR_ACTION_IN_CHUNK}
                )
       


Function    <-  FDef Nl^MISS_NL
                (
                    FLocals
                    Actions
                )  -> FunctionBody
                FEnd -> FunctionEnd
FDef        <-  (
                    ({CONSTANT?} FUNCTION  Name TAKES^SYNTAX_ERROR) -> FunctionName
                    FTakes 
                    (FReturns) -> FunctionReturns
                )

FTakes      <-  (
                    NOTHING
                /   (FArg (COMMA FArg)*)
                /   Sp %{SYNTAX_ERROR}
                )
FArg        <-  (Name Name) -> FArg 

FReturns    <-  RETURNS^SYNTAX_ERROR Name
FLocals     <-  {Locals} / {} -> Nil
FEnd        <-  (ENDFUNCTION Ed^MISS_NL)?



Native      <-  ( 
                    ({CONSTANT?} NATIVE Name TAKES^SYNTAX_ERROR) -> NativeName
                     NTakes 
                    (NReturns) -> NativeReturns
                )
         

NTakes      <-  (
                    NOTHING
                /   (NArg (COMMA NArg)*)
                )

NArg        <-  (Name Name) -> NArg
NReturns    <-  RETURNS^SYNTAX_ERROR Name



Actions     <-  (Nl / Action)*

Action      <- (ACall / ASet / AReturn / AExit / ALogic / ALoop / AError)
         

-- Are you kidding me Blizzard?
ACall       <-  (DEBUG? CALL Name^SYNTAX_ERROR PL ACallArgs? PR^ERROR_MISS_PR)
        ->  ACall
ACallArgs   <-  Exp (COMMA Exp)*

ASet        <-  (SET Name^SYNTAX_ERROR (BL Exp^ERROR_MISS_EXP BR^ERROR_MISS_BR)? ASSIGN^SYNTAX_ERROR Exp^ERROR_MISS_EXP)
        ->  Set

AReturn     <-  RETURN (ARExp Ed / Sp %{SYNTAX_ERROR})
ARExp       <-  Ed  -> Return
        /   Exp -> ReturnExp Ed

AExit       <-  EXITWHEN Exp
        ->  Exit

ALogic      <-  (
                LIf
                LElseif*
                LElse?
            
            LEnd)
        ->  Logic
LIf         <-  (
                {} -> IfStart
                IF (Exp THEN)^ERROR_MISS_THEN Nl^MISS_NL
                    Actions
            )
        ->  If
LElseif     <-  (
                {} -> ElseifStart
                ELSEIF (Exp THEN)^ERROR_MISS_THEN Nl^MISS_NL
                    Actions
            )
        ->  Elseif
LElse       <-  (
                {} -> ElseStart
                ELSE Nl^MISS_NL
                    Actions
            )
        ->  Else
LEnd        <-  ({ENDIF} Ed^MISS_NL)?

ALoop       <-  (
                LOOP -> LoopStart Nl^MISS_NL
                    Actions
                ({ENDLOOP} Ed^MISS_NL)?
            )
        ->  Loop

AError      <-  LOCAL Ignore
            ->  localInFunction
            /   TYPE Ignore
            ->  typeInFunction


Globals     <-  GLOBALS -> GlobalsStart Nl^MISS_NL
                     ((Nl / Global)*)
                {(ENDGLOBALS Ed^MISS_NL)?} -> GlobalsEnd
Global      <-  !GLOBALS !FUNCTION !NATIVE
                ({CONSTANT?} Name {ARRAY?} Name (ASSIGN Exp)?)
            ->  Global

Local       <-  ((CONSTANT -> constantLocal)? LOCAL Name {ARRAY?} Name LocalExp?)
            ->  Local
            /   TYPE Ignore
            ->  typeInFunction

LocalExp    <-  ASSIGN Exp
Locals      <-  (Local? Nl)+


Type        <-  (TYPE TChild TExtends TParent)
            ->  Type
TChild      <-  Name   ^ERROR_VAR_TYPE
TExtends    <-  EXTENDS^ERROR_EXTENDS_TYPE
TParent     <-  Name   ^ERROR_EXTENDS_TYPE

Exp         <-  ECheckAnd
ECheckAnd   <-  (ECheckOr   (Sp ESAnd    ECheckOr  ^ERROR_MISS_EXP)*) -> Binary
ECheckOr    <-  (ECheckComp (Sp ESOr     ECheckComp^ERROR_MISS_EXP)*) -> Binary
ECheckComp  <-  (ECheckNot  (Sp ESComp   ECheckNot ^ERROR_MISS_EXP)*) -> Binary
ECheckNot   <-  (            Sp ESNot+   ECheckAdd ^ERROR_MISS_EXP  ) -> Unary / ECheckAdd
ECheckAdd   <-  (ECheckMul  (Sp ESAddSub ECheckMul ^ERROR_MISS_EXP)*) -> Binary
ECheckMul   <-  (EUnit      (Sp ESMulDiv EUnit     ^ERROR_MISS_EXP)*) -> Binary

ESAnd       <-  AND -> 'and'
ESOr        <-  OR  -> 'or'
ESComp      <-  UE  -> '!='
            /   EQ  -> '=='
            /   LE  -> '<='
            /   LT  -> '<' 
            /   GE  -> '>='
            /   GT  -> '>'
            -- 收集错误
            /   Sp '=' -> '='
ESNot       <-  NOT -> 'not'
ESAddSub    <-  ADD -> '+'
            /   SUB -> '-'
ESMulDiv    <-  MUL -> '*'
            /   DIV -> '/'
            /   MOD -> '%%'

EUnit       <-  EParen / ECode / ECall / EValue / ENeg

EParen      <-  PL Exp PR^ERROR_MISS_PR

ECode       <-  (FUNCTION Name ({PL} ECallArgs? PR^ERROR_MISS_PR)?)
            ->  Code

ECall       <-  (Name PL ECallArgs? PR^ERROR_MISS_PR)
            ->  ECall
ECallArgs   <-  Exp (COMMA Exp)*

EValue      <-  Value / EVari / EVar

EVari       <-  (Name BL Exp BR)
            ->  Vari

EVar        <-  Name
            ->  Var

ENeg        <-  Sp SUB EUnit
            ->  Neg


Name        <-  Sp {[a-zA-Z] [a-zA-Z0-9_]*}


GT          <-  '>'
GE          <-  '>='
LT          <-  '<'
LE          <-  '<='
EQ          <-  '=='
UE          <-  '!='


ADD         <-  '+'
SUB         <-  '-'
MUL         <-  '*'
DIV         <-  '/'
MOD         <-  '%'

PL          <-  Sp '('
PR          <-  Sp ')'
BL          <-  Sp '['
BR          <-  Sp ']'

Value       <-  NULL / Boolean / String / Real / Integer
NULL        <-  Sp 'null' Cut
            ->  NULL

Boolean     <-  TRUE  -> TRUE
            /   FALSE -> FALSE

String      <-  Sp '"' {(Esc / %nl / [^"])*} -> String '"'

Real        <-  Sp ({'-'?} Sp {'.' [0-9]+^ERROR_REAL / [0-9]+ '.' [0-9]*})
            ->  Real

Integer     <-  Integer16 / Integer8 / Integer10 / Integer256
Integer8    <-  Sp ({'-'?} Sp {'0' [0-7]*})
            ->  Integer8
Integer10   <-  Sp ({'-'?} Sp {'0' / ([1-9] [0-9]*)})
            ->  Integer10
Integer16   <-  Sp ({'-'?} Sp ('$' / '0x' / '0X') {Char16})
            ->  Integer16
Char16      <-  [a-fA-F0-9]+^ERROR_INT16
Integer256  <-  Sp ({'-'?} Sp C256)
            ->  Integer256
C256        <-  "'" {C256_1} "'"
            /   "'" {C256_4 C256_4 C256_4 C256_4} "'"
            /   "'" %{ERROR_INT256_COUNT}
C256_1      <-  Esc / %nl / [^']
C256_4      <-  %nl / [^']


Comment     <-  '//' [^%nl]* -> Comment


Sp          <-  (Comment / %s)* ExChar?
ExChar      <-  &%ExChar %{EXCEPTION_CHAR}

Nl          <-  (Sp %nl)+
Ignore      <-  [^%nl]*

Cut         <-  ![a-zA-Z0-9_]
Ed          <-  Sp (&Nl / !.)
COMMA       <-  Sp ','
ASSIGN      <-  Sp '='
GLOBALS     <-  Sp 'globals' Cut
ENDGLOBALS  <-  Sp 'endglobals' Cut
CONSTANT    <-  Sp 'constant' Cut
NATIVE      <-  Sp 'native' Cut
ARRAY       <-  Sp 'array' Cut
AND         <-  Sp 'and' Cut
OR          <-  Sp 'or' Cut
NOT         <-  Sp 'not' Cut
TYPE        <-  Sp 'type' Cut
EXTENDS     <-  Sp 'extends' Cut
FUNCTION    <-  Sp 'function' Cut
ENDFUNCTION <-  Sp 'endfunction' Cut
NOTHING     <-  Sp 'nothing' Cut
TAKES       <-  Sp 'takes' Cut
RETURNS     <-  Sp ('returns' / 'return' -> returnAsReturns) Cut
CALL        <-  Sp ('call' / 'set' -> setAsCall) Cut
SET         <-  Sp ('set' / 'call' -> callAsSet) Cut
RETURN      <-  Sp 'return' Cut
IF          <-  Sp 'if' Cut
THEN        <-  Sp 'then' Cut
ENDIF       <-  Sp 'endif' Cut
ELSEIF      <-  Sp 'elseif' Cut
ELSE        <-  Sp 'else' Cut
LOOP        <-  Sp 'loop' Cut
ENDLOOP     <-  Sp 'endloop' Cut
EXITWHEN    <-  Sp 'exitwhen' Cut
LOCAL       <-  Sp 'local' Cut
TRUE        <-  Sp 'true' Cut
FALSE       <-  Sp 'false' Cut
DEBUG       <-  Sp 'debug' Cut

Esc         <-  '\' {EChar}
EChar       <-  'b' -> eb
            /   't' -> et
            /   'r' -> er
            /   'n' -> en
            /   'f' -> ef
            /   '"'
            /   '\'
            /   %{ERROR_ESC}

)";