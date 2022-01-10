//
//  Compile.h
//  CompileArly
//
//  Created by Chris Marrin on 1/9/22.
//
// Arly compiler
//
// Compile Arly statements into a binary stream which can be loaded and executed
//
/* Arly Language

program         ::= { component }
component       ::= constant | table | effect
constant        ::= 'const' type <id> value ';'
table           ::= 'table' tableDef { tableDef }
tableDef        ::= type <id> values ';'
effect          ::= 'effect' <id> <integer> defs init loop
init            ::= 'init' '{' statements '}'
loop            ::= 'loop' '{' statements '}'

defs            ::= { def }
def             ::= type <id> ';'
statements      ::= { statement }
statement       ::= opStatement | forStatement | ifStatement
opStatement     ::= op { values } ';'
forStatement    ::= 'foreach' '{' statements '}'
ifStatement     ::= 'if' '{' statements '}' { 'else' '{' statements '}'

type            ::= 'float' | 'uint8' | 'uint32'
values          ::= value { value }
value           ::= <float> | <integer>

op              ::= <list of opcodes below>

Constants and Tables

The engine has space for constants. Constant memory is byte addressed and is
read-only (stored in EEPROM). A constant can be a byte (1), int32 (4),
float (4) or color (12). Values are packed tightly and unpacked in little endian
order. Tables are also byte addressed. The code must take care not to overrun
the length of the table.

Built-in Constants

There are built-in constants available to the program. Currently there is
'numPixels' which is the number of pixels in the LED array

Runtime

Runtime has a register each for color, float and int32. It also has byte storage
for values. Like constants, values are packed tightly. But storage can be read
and written. A def is used to identify a storage location. The id is a 0-127
value. Bytes 0-63 specify constants in EEPROM, starting at 0. Bytes 64-128 are
in storage in RAM, at id-64. So there are 64 bytes of storage available. An
internal I register is used to set the id to use. This keeps each instruction
to a single byte. The I register is set with an internal SetI instruction. This
register can only be relied upon for the instruction immediately following
SetI because it is used for other internal purposes.

Effects

An effect name is an id, but only the first character of that id is used as
the name of the command and that character must be 'A' - 'Z'. Using the same
first character for two effects is an error. The number given for the 'effect'
is the number of params expected, used for error checking. Passing too few
params is an error and the effect does not run. If index passed to PushColorParam
or PushInt8Param goes beyond end of param list, a 0 is pushed. Param index is 
4 bits allowing 16 bytes of params. This is enough for 4 colors and 4 bytes of
params.

ForEach

This op starts with 2 integers on the value stack. It pops TOS and uses it as the
count. The TOS now contains an index which is increments after execution of
all statement in the foreach, until the tos value equals count. If index is
>= count at the start of the loop, it is not executed at all. Before the first
loop the address of the first statement is pushed on the for stack. At the
end of each loop (unless it is terminating) this value is used to jump back to
the first statement for the next iteration. When the loop terminates, this
value is popped from the for stack.

If

When an If instructon is generated it is preceded by an internally generated 
SetI instruction which contains the number of bytes of statements in the if.
If the if test fails, the I value is used to skip to the end of the if.


Engine

There are 4 color registers which contain 3 float (h, s, v) and 4 registers of
4 bytes which can hold an integer or a float.

Opcodes:
    The SetId opcode sets a special id register to the lower 7 bits of the op.
    All the ops needing an id use this register
    
    Nomenclature:
    
        c - Color registers 0-1, each with 3 floats (h, s, v)
        v - Value registers 0-1, each with float
        p - Param byte array, maximum of 16 bytes
        
    SetI i                  - Set I register the passed value (0-127)
    
    LoadColorParam r, i     - c[r] = p[i]
    LoadParam r, i          - v[r] = p[i] (1 byte converted to float)
    
    LoadColor r, id         - c[r] = mem[id] (12 bytes, 3 floats)
    Load r, id              - v[r] = mem[id] (4 byte float)
    
    LoadColorBlack r        - c[r] = Color()
    LoadZero r              - v[r] = 0
    
    LoadX, id               - X = @id
    LoadY, id               - Y = @id
    LoadIX rd, i            - v[rd] = mem[X + i] (4 byte float)
    LoadIY rd, i            - v[rd] = mem[Y + i] (4 byte float)
    
    StoreColor id, r        - mem[id] = c[r] (12 bytes, 3 floats)
    Store id, r             - mem[id] = v[r] (4 byte float)
    
    StoreIX i, rs           - mem[X + i] = v[rs]
    StoreIY i, rs           - mem[Y + i] = v[rs]
    
    MoveColor rd, rs        - c[rd] = c[rs]
    Move rd, rs             - v[rd] = v[rs]
    
    SetAllLights            - Set all lights to color in c[0].
    Init id                 - Init mem at id with value in r[0] for r[1] values

    ForEach                 - v[2] is i, v[3] is count, iterate i until it hits count
    EndFor                  - End of for loop
    
    If                      - If v[0] is non-zero execute statements in first clause. 
                              If zero skip the statements. Number of bytes to skip is 
                              set in an internally generated SetI instruction before 
                              the if.
    Else                    - If the previously executed statement was a failed if
                              execute the following statements. Otherwise skip the
                              following statements up to the matching endif. The
                              number of instructions to skip is in the I register
                              from an internally generated SetI instruction.
    Endif                   - Signals the end of an if or else statement
    
    21 ops always use r0 and r1 leaving result in r0
    
    
    
    
    
    
    PushColorParam i        - Push 3 byte color param at i onto color stack
    PushInt8Param i         - Push byte param at i onto value stack (sign extend to 32 bits)
    PushInt32Param i        - Push 4 byte param at i onto value stack as int32
    PushFloatParam i        - Push 4 byte param at i onto value stack as float
    
    PushColor               - Push color at I onto color stack
    PushInt8                - Push int8 at I onto value stack as an int32
                              (sign extends to 32 bits)
    PushInt32               - Push int32 at I onto value stack
    PushFloat               - Push float at I onto value stack
    PushIntZero
    PushFloatZero
    PushColorBlack
    
    PushIndex               - Push I on value tos
    
    PushInt8Indexed         - Pop 2 values on tos, add them and push the int8 at that addr
    PushInt32Indexed        - Pop 2 values on tos, add them and push the int32 at that addr
    PushFloatIndexed        - Pop 2 values on tos, add them and push the float at that addr
    
    StoreColor              - Store top of color stack at I and pop
    StoreInt8               - Store top of value stack at I as a byte (truncate) and pop
    StoreInt32              - Store top of value stack at I as a 4 bytes and pop
    StoreFloat              - Store top of value stack at I as a 4 bytes and pop
    
    StoreInt8Indexed        - 
    
    InitInt8                - value tos is number of int8s to init. tos-1 is value to set.
                              I reg contains the index in storage where to begin
    InitInt32               - value tos is number of int32s to init. tos-1 is value to set.
                              I reg contains the index in storage where to begin
    InitFloat               - value tos is number of floats to init. tos-1 is value to set.
                              I reg contains the index in storage where to begin
    
    Dup                     - Duplicate tos in the value stack
    DupColor                - Duplicate tos in the color stack
    
    SetLight                - Set light at integer index on top of value stack to 
                              color on top of color stack. Do not pop color
    SetAllLights            - Set all lights to color on top of the color stack. Do not pop.
    PushVal                 - Push the val component on top of the color stack
                              onto the value stack as a float
    MoveVal                 - Move value stack tos (assumes float) to val component
                              of color tos, pop value stack
    ToFloat                 - Convert value stack TOS from int to float
                              
    AddInt                  - Add top two integers on value stack, pop both push result
    SubInt                  - Subtract top two integers on value stack, pop both push result
    MulInt                  - Multiply top two integers on value stack, pop both push result
    DivInt                  - Divide top two integers on value stack, pop both push result
    
    AddFloat                - Add top two floats on value stack, pop both, push result
    SubFloat                - Subtract top two floats on value stack, pop both, push result
    MulFloat                - Multiply top two floats on value stack, pop both, push result
    DivFloat                - Divide top two floats on value stack, pop both, push result
    
    foreach                 - value tos has count, value tos-1 has index, both integers. 
                              Pop count and iterate statements in braces, 
                              incrementing index on each iteration
    endfor                  - Signals end of for loop
    if                      - value tos has result. Pop result. If it is non-zero
                              execute statements in first clause. If zero skip the
                              statements. Number of bytes to skip is set in an
                              internally generated SetI instruction before the if.
    else                    - If the previously executed statement was a failed if
                              execute the following statements. Otherwise skip the
                              following statement up to the matching endif. The
                              number of instructions to skip is in the I register
                              from an internally generated SetI instruction.
    endif                   - Signals the end of an if or else statement
    

*/

// All ops are 1 byte except those taking id param, which is in the following byte
// Id 
static constexpr uint8_t LoadColorParam     = 0x00; // lower 4 bits are param index
static constexpr uint8_t LoadParam          = 0x10; // lower 4 bits are param index

static constexpr uint8_t LoadColor          = 0x20;
static constexpr uint8_t Load               = 0x22;
static constexpr uint8_t LoadBlack          = 0x24;
static constexpr uint8_t LoadZero           = 0x26;
static constexpr uint8_t StoreColor         = 0x38;
static constexpr uint8_t Store              = 0x3A;

static constexpr uint8_t LoadX              = 0x3C;
static constexpr uint8_t LoadY              = 0x3E;
static constexpr uint8_t LoadIX             = 0x40;
static constexpr uint8_t LoadIY             = 0x42;
static constexpr uint8_t StoreIX            = 0x44;
static constexpr uint8_t StoreIY            = 0x46;

static constexpr uint8_t SetLights          = 0x47;
static constexpr uint8_t SetAllLights       = 0x48;
static constexpr uint8_t ForEach            = 0x49;
static constexpr uint8_t EndFor             = 0x4A;
static constexpr uint8_t If                 = 0x4B;
static constexpr uint8_t Else               = 0x4C;
static constexpr uint8_t Endif              = 0x4D;
static constexpr uint8_t Init               = 0x4E;

static constexpr uint8_t Bor                = 0x50;
static constexpr uint8_t Bxor               = 0x51;
static constexpr uint8_t Band               = 0x52;
static constexpr uint8_t Bnot               = 0x53;
static constexpr uint8_t Or                 = 0x54;
static constexpr uint8_t And                = 0x55;
static constexpr uint8_t Not                = 0x56;
static constexpr uint8_t Neg                = 0x57;
static constexpr uint8_t LT                 = 0x58;
static constexpr uint8_t LE                 = 0x59;
static constexpr uint8_t EQ                 = 0x5A;
static constexpr uint8_t NE                 = 0x5B;
static constexpr uint8_t GE                 = 0x5C;
static constexpr uint8_t GT                 = 0x5D;
static constexpr uint8_t Add                = 0x5E;
static constexpr uint8_t Sub                = 0x5F;
static constexpr uint8_t Mul                = 0x60;
static constexpr uint8_t Div                = 0x61;
static constexpr uint8_t Mod                = 0x62;
static constexpr uint8_t Inc                = 0x63;
static constexpr uint8_t Dec                = 0x64;

static constexpr uint8_t MoveColor          = 0x70;
static constexpr uint8_t Move               = 0x78;

static constexpr uint8_t SetI               = 0x80  // lower 7 bits are id




static constexpr uint8_t PushInt8       = 0x41;
static constexpr uint8_t PushInt32      = 0x42;
static constexpr uint8_t PushFloat      = 0x43;
static constexpr uint8_t StoreColor     = 0x44;
static constexpr uint8_t StoreInt8      = 0x45;
static constexpr uint8_t StoreInt32     = 0x46;
static constexpr uint8_t StoreFloat     = 0x47;



#pragma once

