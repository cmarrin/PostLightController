//
//  Opcodes.h
//  CompileArly
//
//  Created by Chris Marrin on 1/9/22.
//
// Arly opcodes
//

#pragma once

#include <stdint.h>

namespace arly {

/*

Arly Machine Code

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

Runtime has 4 Color registers for Color, and 4 scalar registers for int32/float.
The opcode being executed determines whether the scalar registers have an
int or float. There are operations for both, but 2 operand opcodes require
both values to be the same type.

The constants are is a read-only array of int32 or floats in ROM. In the opcodes
the id for a constant is 0x00 to 0x7f. But only as much ROM memory is used as needed
by the number of constants and tables defined. Theoretically you can have
128 values, which would take up 512 bytes. This is half the available ROM
space, so a program will generally no use that much.

There is also a read/write memory area which contains int32 or float values.
You can place colors in this memory, in which case it is stored as 3 
consecutive float (hsv). These values are defined by the 'defs' statements.
Like the constants, only enough ram space as is needed by the defs is
allocated. The size of a def is in 4 byte units. So if it is to hold a
color for instance, it would need 3 locations. In the opcodes the id for a 
var is 0x80 to 0xff to distinguish them from constants.

Values

There are four types of values:

    Defs            These are compile time definitions which can be used in
                    LoadIntConst ops with a value from 0 to 255 and anywhere an 'i'
                    value is used with a value of 0-15.
                    
    Constants       From the standpoint of the runtime, constants introduced with
                    'const' and 'table' are the same thing. They appear first
                    in ROM and are accessed with an <id> from 0 to 127.
                    
    Global vars     These are introduced with the 'var' keyword and are stored in
                    RAM. The executable has the size needed for this memory and it
                    is allocated at runtime. They are globally available to all
                    functions. They are accessed with an <id> of 0x80 to 0xbf.

    Local vars      These are also introduced by the 'var' keyword, but at the
                    top of functions. They are kept on the stack, so they are only 
                    available while the function is active. The system can also 
                    allocate temp variables, which are also placed on the stack 
                    above the declared vars. They are accessed with an <id> of
                    0xc0 to 0xff.

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

Function calls

There is a stack which holds passed parameters. And each function can have locals
on the stack. When a function is running the bp points to the first param.
locals are on top of params. Then the return pc and the previous bp. On return
save the previous bp, then pop the return pc into _pc, then set _sp to bp
and finally set bp to the saved previous bp. This cleans up the stack without
the caller needing to do any work.

Each function must have a SetFrame as the first instruction. The caller ensures
this and throws if it's not true. SetFrame establishes the number of params
and locals. On function entry the stack contains the passed params and return
pc. The number of passed params must match the number of formal params. This is
ensured at compile time. SetFrame will pop the return pc and save it. Then it
will add the number of locals to sp. then it will push the saved return pc and
the current bp. Finally it will set bp to sp - params - locals - 2. That 
makes it point at the first passed param.

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
        
        
    Push id                 - stack[sp++] = global[id - 0x80] or const[id] or stack[bp + id - 0xc0]
    Pop id                  - global[id - 0x80] or stack[bp + id = 0xc0] = stack[--sp]
    
    PushZero                - stack[sp++] = 0
    PushIntConst const      - stack[sp++] = const (constant value 0-255)

    // The opcodes deal with variable references. LoadRef simply places
    // the address of the passed id in the passed register. It can later
    // be used by LoadDeref and StoreDeref. LoadRefX is passed a source
    // register and numeric value. The value is the number of words per
    // entry in the array. The register value is multiplied by the passed
    // value and the result is added to the address of the passed id which
    // is stored in the destination register.
    //
    // LoadDeref is passed the register with the address previous set above
    // along with a numeric value to access the given element of the entry.
    // The source register address is added to the passed value, then the
    // memory at that address is loaded into the destination register.
    // StoreDeref does the same but the value in the source register is
    // stored at the address.
    
    PushRef id              - stack[sp++] = id
    PushRefX id i           - t = stack[--sp], stack[sp++] = id + t * i
    PushDeref i             - t = stack[--sp], stack[sp++] = mem[t + i]
    PopDeref i              - t = stack[--sp], mem[t + i] = stack[--sp]
    
    Dup                     - stack[sp++] = stack[sp]
    Drop                    - --sp
    Swap                    - t = stack[sp]; stack[sp] = stack[sp-1]; stack[sp-1[ = t;

    MinInt                  - stack[sp++] = min(stack[--sp], stack[--sp]) (values are int32_t)
    MinFloat                - stack[sp++] = min(stack[--sp], stack[--sp]) (values are float)
    MaxInt                  - stack[sp++] = max(stack[--sp], stack[--sp]) (values are int32_t)
    MaxFloat                - stack[sp++] = max(stack[--sp], stack[--sp]) (values are float)
    
    Exit                    - exit interpreter and return value in stack[--sp] (assumes stack[--sp] is int32_t)
    ToFloat                 - stack[sp] = float(stack[sp])
    ToInt                   - stack[sp] = int32_t(stack[sp])
    
    Init id                 - Init mem at id with value in stack[--sp] for stack[--sp] values
    RandomInt               - stack[sp++] = random(stack[--sp], stack[--sp]), integer params, integer return
    RandomFloat             - stack[sp++] = random(stack[--sp], stack[--sp]), float params, float return

    If sz                   - If stack[--sp] is non-zero execute statements in first clause. 
                              If zero skip the statements. Number of bytes to skip is 
                              in sz.
    Else sz                 - If the previously executed statement was a failed if
                              execute the following statements. Otherwise skip the
                              following statements up to the matching endif. The
                              number of instructions to skip is in sz.
    Endif                   - Signals the end of an if or else statement
    
    ForEach id sz           - id is i, stack[--sp] is count, iterate i until it hits count
                              sz is number of bytes in loop
    EndForEach              - End of for loop
    
    Call target             - Call function [target], params on stack
    CallNative Const        - Call native function [enum NativeFunction] (0..255)
    Return                  - Return from function
    SetFrame p l            - Set the local frame with number of formal
                              params (p) and locals (l). This must be
                              the first instruction of every function
    
    21 ops always use r0 and r1 leaving result in r0
    
    Or                     - v[0] = v[0] | v[1] (assumes int32_t)
    XOr                    - v[0] = v[0] ^ v[1] (assumes int32_t)
    And                    - v[0] = v[0] & v[1] (assumes int32_t)
    Not                    - v[0] = ~v[0] (assumes int32_t)
    
    LOr                     - v[0] = v[0] || v[1] (assumes int32_t)
    LAnd                    - v[0] = v[0] && v[1] (assumes int32_t)
    LNot                    - v[0] = !v[0] (assumes int32_t)
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

    Log i                   - Print value of TOS-i as int
    LogFloat i              - Print value of TOS-i as float
    LogColor i              - Print value of c[i] as color
    
    Executable format
    
    Format Id           - 4 bytes: 'arly'
    Constants size      - 1 byte: size in 4 byte units of Constants area
    Global size         - 1 byte: size in 4 byte units needed for global vars
    Stack size          - 1 byte: size in 4 byte units needed for the stack
    Unused              - 1 bytes: for alignment
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

static constexpr uint8_t ConstStart = 0x00;
static constexpr uint8_t GlobalStart = 0x80;
static constexpr uint8_t LocalStart = 0xc0;
static constexpr uint8_t ConstSize = GlobalStart - ConstStart;
static constexpr uint8_t GlobalSize = LocalStart - GlobalStart;
static constexpr uint8_t LocalSize = 0xff - LocalStart + 1;

enum class Op: uint8_t {

    None            = 0x0f,

// 15 unused opcodes

    Push            = 0x10,
    Pop             = 0x11,

    PushZero        = 0x12,
    PushIntConst    = 0x13,
    
    PushRef         = 0x14,
    PushRefX        = 0x15,
    PushDeref       = 0x16,
    PopDeref        = 0x17,
    
    Dup             = 0x18,
    Drop            = 0x19,
    Swap            = 0x1a,
    
    MinInt          = 0x30,
    MinFloat        = 0x31,
    MaxInt          = 0x32,
    MaxFloat        = 0x33,
    
    Exit            = 0x34,
    ToFloat         = 0x35,
    ToInt           = 0x36,

    Init            = 0x37,
    RandomInt       = 0x38,
    RandomFloat     = 0x39,

    If              = 0x3a,
    Else            = 0x3b,
    EndIf           = 0x3c, // At the end of if
    ForEach         = 0x3d,
    EndForEach      = 0x3e, // At the end of foreach

// Call needs 2 extra bits for its
// target addr, use lower 2 bits of
// op, so skip 4 here.
    Call            = 0x3f,
    
    CallNative      = 0x40,
    Return          = 0x41,
    SetFrame        = 0x42,
    
// 5 unused opcodes
    
    Or              = 0x50,
    Xor             = 0x51,
    And             = 0x52,
    Not             = 0x53,

    LOr             = 0x54,
    LAnd            = 0x55,
    LNot            = 0x56,

    LTInt           = 0x57,
    LTFloat         = 0x58,
    LEInt           = 0x59,
    LEFloat         = 0x5a,
    EQInt           = 0x5b,
    EQFloat         = 0x5c,
    NEInt           = 0x5d,
    NEFloat         = 0x5e,
    GEInt           = 0x5f,
    GEFloat         = 0x60,
    GTInt           = 0x61,
    GTFloat         = 0x62,

    AddInt          = 0x63,
    AddFloat        = 0x64,
    SubInt          = 0x65,
    SubFloat        = 0x66,
    MulInt          = 0x67,
    MulFloat        = 0x68,
    DivInt          = 0x69,
    DivFloat        = 0x6a,

    NegInt          = 0x6b,
    NegFloat        = 0x6c,
    
    Log             = 0xe0, // Print r as int32_t with addr - For debugging
    LogFloat        = 0xe1, // Print r as float with addr - For debugging
    LogColor        = 0xe2, // Print c with addr - For debugging
    
    End             = 0xff,
    
// 6 unused opcodes

};

enum class OpParams : uint8_t {
    None,       // No params
    Id,         // b+1 = <id>
    Id_I,       // b+1 = <id>, b+2[3:0] = <int>
    I,          // b+1[3:0] = <int>
    Const,      // b+1 = 0-255
    Target,     // b+1 = call target bits 7:2, b[2:0] = call target bits 1:0};
    P_L,        // b+1[7:4] = num params, b+1[3:0] = num locals
    Sz,         // b+1 = <int>
    Id_Sz,      // b+1 = <id>, b+2 = <int>
};

}
