//
//  Compiler.h
//  CompileArly
//
//  Created by Chris Marrin on 1/9/22.
//
// Arly compiler
//
// Compile Arly statements into a binary stream which can be loaded and executed
//

#pragma once

#include "Scanner.h"
#include <cstdint>
#include <istream>
#include <vector>

namespace arly {

/* Arly Language

program         ::= constants tables effects
constants       ::= { constant <n> }
constant        ::= 'const' type <id> value
tables          ::= { table <n> }
table           ::= 'table' type <id> <n> tableEntries 'end'
tableEntries    ::= { values <n> }
effects         ::= { effect <n> }
effect          ::= 'effect' <id> <integer> <n> defs init loop 'end'
defs            ::= { def <n> }
def             ::= type <id> <integer>
init            ::= 'init' <n> statements 'end' <n>
loop            ::= 'loop' <n> statements 'end' <n>

statements      ::= { statement <n> }
statement       ::= opStatement | forStatement | ifStatement
opStatement     ::= op opParams
forStatement    ::= 'foreach' <id> <n> statements 'end'
ifStatement     ::= 'if' <n> statements { 'else' <n> statements } 'end'

type            ::= 'float' | 'int'
values          ::= { value }
value           ::= <float> | <integer>
opParams        ::= { value }
opParam         ::= <id> | <integer>

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
    Nomenclature:
    
        c - Color registers 0-3, each with 3 floats (h, s, v)
        v - Value registers 0-3, each with float
        p - Param byte array, maximum of 16 bytes
        
    Params:
        
        id          - In byte after opcode
        sz          - In byte after opcode
        r           - in lower 2 bits of opcode
        rs, rd, i   - Packed in byte (rs:2, rd:2, i:4) after opcode or after id
        
        
    LoadColorParam r i      - c[r] = p[i] (3 bytes, hsv converted to floats)
    LoadIntParam r i        - v[r] = p[i] (1 byte converted to int32)
    LoadFloatParam r i      - v[r] = p[i] (1 byte converted to float)
    
    LoadColor r id          - c[r] = mem[id] (12 bytes, 3 floats)
    Load r id               - v[r] = mem[id] (4 byte int or float)
    StoreColor id r         - mem[id] = c[r] (12 bytes, 3 floats)
    Store id r              - mem[id] = v[r] (4 byte int or float)

    LoadBlack r             - c[r] = Color()
    LoadZero r              - v[r] = 0
    LoadIntOne r            - v[r] = 1
    LoadFloatOne r          - v[r] = 1.0f
    LoadIntByteMax r        - v[r] = 255
    
    LoadColorX rd id rs     - c[rd] = mem[id + (rs * 3)]
    LoadX rd, id, rs, i     - v[rd] = mem[id + rs + i]
    StoreColorX id rd rs    - mem[id + (rd * 3)] = c[rs]
    StoreX id rd i rs       - mem[id + rd + i] = v[rs]
    
    MoveColor rd, rs        - c[rd] = c[rs]
    Move rd, rs             - v[rd] = v[rs]
    
    LoadVal rd rs           - v[rd] = c[rs].val
    StoreVal rd, rs         - c[rd].val = v[rs]
    MinInt                  - v[0] = min(v0, v1) (values are int32_t)
    MinFloat                - v[0] = min(v0, v1) (values are float)
    MaxInt                  - v[0] = max(v0, v1) (values are int32_t)
    MaxFloat                - v[0] = max(v0, v1) (values are float)
    
    Return r                - return v[r] (assumes v[r] is int32_t)
    ToFloat r               - v[r] = float(v[r])
    ToInt r                 - v[r] = int32_t(v[r])
    
    SetAllLights r          - Set all lights to color in c[r].

    SetLight rd rs          - light[rd] = c[rs]
    Init id                 - Init mem at id with value in r[0] for r[1] values
    Random                  - random(v[0], v[1]), using Effect::randomFloat

    If sz                   - If v[0] is non-zero execute statements in first clause. 
                              If zero skip the statements. Number of bytes to skip is 
                              in sz.
    Else sz                 - If the previously executed statement was a failed if
                              execute the following statements. Otherwise skip the
                              following statements up to the matching endif. The
                              number of instructions to skip is in sz.
    Endif                   - Signals the end of an if or else statement
    
    ForEach r sz            - v[r] is i, v[0] is count, iterate i until it hits count
                              sz is number of bytes in loop
    EndFor                  - End of for loop
    
    21 ops always use r0 and r1 leaving result in r0
    
    BOr                     - v[0] = v[0] | v[1] (assumes int32_t)
    BXOr                    - v[0] = v[0] ^ v[1] (assumes int32_t)
    BAnd                    - v[0] = v[0] & v[1] (assumes int32_t)
    BNot                    - v[0] = ~v[0] (assumes int32_t)
    
    Or                      - v[0] = v[0] || v[1] (assumes int32_t)
    And                     - v[0] = v[0] && v[1] (assumes int32_t)
    Not                     - v[0] = !v[0] (assumes int32_t)
    LTInt                   - v[0] = v[0] < v[1] (assumes int32_t, result is int32_t)
    LTFloat                 - v[0] = v[0] < v[1] (assumes float, result is int32_t)
    LEInt                   - v[0] = v[0] <= v[1] (assumes int32_t, result is int32_t)
    LEFloat                 - v[0] = v[0] <= v[1] (assumes float, result is int32_t)
    EQInt                   - v[0] = v[0] == v[1] (assumes int32_t, result is int32_t)
    EQFloat                 - v[0] = v[0] == v[1] (assumes float, result is int32_t)
    NEInt                   - v[0] = v[0] != v[1] (assumes int32_t, result is int32_t)
    NEFloat                 - v[0] = v[0] != v[1] (assumes float, result is int32_t)
    GEInt                   - v[0] = v[0] >= v[1] (assumes int32_t, result is int32_t)
    GEFloat                 - v[0] = v[0] >= v[1] (assumes float, result is int32_t)
    GTInt                   - v[0] = v[0] > v[1] (assumes int32_t, result is int32_t)
    GTFloat                 - v[0] = v[0] > v[1] (assumes float, result is int32_t)
    
    AddInt                  - v[0] = v[0] + v[1] (assumes int32_t, result is int32_t)
    AddFloat                - v[0] = v[0] + v[1] (assumes float, result is float)
    SubInt                  - v[0] = v[0] - v[1] (assumes int32_t, result is int32_t)
    SubFloat                - v[0] = v[0] - v[1] (assumes float, result is float)
    MulInt                  - v[0] = v[0] * v[1] (assumes int32_t, result is int32_t)
    MulFloat                - v[0] = v[0] * v[1] (assumes float, result is float)
    DivInt                  - v[0] = v[0] / v[1] (assumes int32_t, result is int32_t)
    DivFloat                - v[0] = v[0] / v[1] (assumes float, result is float)

    NegInt                  - v[0] = -v[0] (assumes int32_t, result is int32_t)
    NegFloat                - v[0] = -v[0] (assumes float, result is float)
    
    
    Executable format
    
    Format Id           - 4 bytes: 'arly'
    Constants size      - 1 byte: size in 4 byte units of Constants area
    Unused              - 3 bytes: for alignment
    Constants area      - n 4 byte entries: ends after size 4 byte units
    Effects entries     - Each entry has:
                            1 byte command ('a' to 'p')
                            1 byte number of param bytes
                            2 bytes start of init instructions, in bytes
                            2 bytes start of loop instructions, in bytes
                            
                          entries end when a byte of 0 is seen
                          
    Effects             - List of init and loop instructions for each effect
                          instruction sections are padded to end on a 4 byte boundary                            
*/

// Ops of 0x80 and above have an r param in the lower 2 bits.
enum class Op: uint8_t {
    LoadColorX      = 0x10,
    LoadX           = 0x11,
    StoreColorX     = 0x12,
    StoreX          = 0x13,

    MoveColor       = 0x14,
    Move            = 0x15,

    LoadVal         = 0x16,
    StoreVal        = 0x17,
    MinInt          = 0x18,
    MinFloat        = 0x19,
    MaxInt          = 0x1a,
    MaxFloat        = 0x1b,

    SetLight        = 0x1c,

    Init            = 0x1d,
    Random          = 0x1e,

    If              = 0x1f,
    Else            = 0x20,
    EndIf           = 0x21, // To distinguish if from if-else

    Bor             = 0x30,
    Bxor            = 0x31,
    Band            = 0x32,
    Bnot            = 0x33,

    Or              = 0x34,
    And             = 0x35,
    Not             = 0x36,

    LTInt           = 0x37,
    LTFloat         = 0x38,
    LEInt           = 0x39,
    LEFloat         = 0x3a,
    EQInt           = 0x3b,
    EQFloat         = 0x3c,
    NEInt           = 0x3d,
    NEFloat         = 0x3e,
    GEInt           = 0x3f,
    GEFloat         = 0x40,
    GTInt           = 0x41,
    GTFloat         = 0x42,

    AddInt          = 0x43,
    AddFloat        = 0x44,
    SubInt          = 0x45,
    SubFloat        = 0x46,
    MulInt          = 0x47,
    MulFloat        = 0x48,
    DivInt          = 0x49,
    DivFloat        = 0x4a,

    NegInt          = 0x4b,
    NegFloat        = 0x4c,

    LoadColorParam  = 0x80,
    LoadIntParam    = 0x84,
    LoadFloatParam  = 0x88,

    LoadColor       = 0x8c,
    Load            = 0x90,
    StoreColor      = 0x94,
    Store           = 0x98,

    LoadBlack       = 0x9c,
    LoadZero        = 0xa0,
    LoadIntOne      = 0xa4,
    LoadFloatOne    = 0xa8,
    LoadByteMax     = 0xac,

    Return          = 0xb0,
    ToFloat         = 0xb4,
    ToInt           = 0xb8,
    SetAllLights    = 0xbc,

    ForEach         = 0xd0,
};

// Up to 4 params
//
// First param is R, Id or Rd
// Second params is I, Id, R or Rs
// Third param is Rs or I
// Fourth param is Rs or I
//
// Param entry is 4 byte array of enums
enum class P : uint8_t { R, I, Id, Rs, Rd, Sz };

enum class OpParams : uint8_t {
    None,       // No params
    R,          // b[1:0] = 'r0'-'r3'
    C,          // b[1:0] = 'c0'-'c3'
    R_I,        // b[1:0] = 'r0'-'r3', b+1 = <int>
    C_I,        // b[1:0] = 'c0'-'c3', b+1 = <int>
    R_Id,       // b[1:0] = 'r0'-'r3', b+1 = <id>
    C_Id,       // b[1:0] = 'c0'-'c3', b+1 = <id>
    Id_R,       // b+1 = <id>, b[1:0] = 'r0'-'r3'
    Id_C,       // b+1 = <id>, b[1:0] = 'c0'-'c3'
    Rd_Id_Rs,   // b+2[7:6] = 'r0'-'r3', b+1 = <id>, b+2[5:4] = 'r0'-'r3'
    Cd_Id_Rs,   // b+2[7:6] = 'c0'-'c3', b+1 = <id>, b+2[5:4] = 'r0'-'r3'
    Rd_Id_Rs_I, // b+2[7:6] = 'r0'-'r3' b+1 = <id>, b+2[5:4] = 'r0'-'r3', b+2[3:0] = <int>
    Id_Rd_Rs,   // b+1 = <id>, b+2[7:6] = 'r0'-'r3', b+2[5:4] = 'r0'-'r3'
    Id_Rd_Cs,   // b+1 = <id>, b+2[7:6] = 'r0'-'r3', b+2[5:4] = 'c0'-'c3'
    Id_Rd_I_Rs, // b+1 = <id>, b+2[7:6] = 'r0'-'r3', b+2[3:0] = <int>, b+2[5:4] = 'r0'-'r3'
    Rd_Rs,      // b+1[7:6] = 'r0'-'r3', b+1[5:4] = 'r0'-'r3'
    Cd_Rs,      // b+1[7:6] = 'c0'-'c3', b+1[5:4] = 'r0'-'r3'
    Rd_Cs,      // b+1[7:6] = 'r0'-'r3', b+1[5:4] = 'c0'-'c3'
    Cd_Cs,      // b+1[7:6] = 'c0'-'c3', b+1[5:4] = 'c0'-'c3'
    Id,         // b+1 = <id>
};

struct OpData
{
    OpData() { }
    OpData(std::string str, Op op, OpParams par) : _str(str), _op(op), _par(par) { }
    std::string _str;
    Op _op = Op::LoadColorX;
    OpParams _par = OpParams::None;
};

enum class Reserved {
    None,
    Const,
    Table,
    Effect,
    End,
    Init,
    Loop,
    ForEach,
    If,
    Else,
    Float,
    Int,
    R0, R1, R2, R3,
    C0, C1, C2, C3,
};

class Compiler
{
public:
    enum class Error {
        None,
        ExpectedToken,
        ExpectedType,
        ExpectedValue,
        ExpectedInt,
        ExpectedOpcode,
        ExpectedEnd,
        ExpectedIdentifier,
        ExpectedCommandId,
        ExpectedRegister,
        InvalidParamCount,
        UndefinedIdentifier,
        ParamOutOfRange,
        ForEachTooBig,
        IfTooBig,
        ElseTooBig,
    };
    
    Compiler() { }
    
    bool compile(std::istream*, std::vector<uint8_t>& executable);

    Error error() const { return _error; }
    Token expectedToken() const { return _expectedToken; }
    const std::string& expectedString() const { return _expectedString; }
    uint32_t lineno() const { return _lineno; }

private:
    Error _error = Error::None;
    Token _expectedToken = Token::None;
    std::string _expectedString;
    uint32_t _lineno;
};

}
