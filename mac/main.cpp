//
//  main.cpp
//  CompileArly
//
//  Created by Chris Marrin on 1/9/22.
//

#include "Compiler.h"
#include "Decompiler.h"
#include "Interpreter.h"
#include "NativeColor.h"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <getopt.h>
#include <cstdio>

// Simulator
//
// Subclass of Interpreter that outputs device info to consolee
class Simulator : public arly::Interpreter
{
public:
    Simulator(arly::NativeModule** mod, uint32_t modSize) : Interpreter(mod, modSize) { }
    virtual ~Simulator() { }
    
    virtual uint8_t rom(uint16_t i) const override
    {
        return (i < 1024) ? _rom[i] : 0;
    }
    
    virtual void log(const char* s) const override
    {
        std::cout << s;
    }

    void setROM(const std::vector<uint8_t>& buf)
    {
        size_t size = buf.size();
        if (size > 1024){
            size = 1024;
        }
        memcpy(_rom, &buf[0], size);
    }
    
private:
    uint8_t _rom[1024];
};

// compile [-o <output file>] [-x] <input file>
//
//      -o      output compiled binary root name (.arly is appended)
//      -s      output binary in 64 byte segments (named <root name>00.arly, etc.
//      -h      output in include file format. Output file is <root name>.h
//      -d      decompile and print result
//      -x      simulate resulting binary
//

// Include file format
//
// This file can be included in an Arduino sketch to upload to EEPROM. The file
// contains 'static const uint8_t PROGMEM EEPROM_Upload = {', followed by each
// byte of the binary data as a hex value. It also has a
// 'static constexpr uint16_t EEPROM_Upload_Size = ' with the number of bytes.

struct Test
{
    const char* _cmd;
    std::vector<uint8_t> _buf;
};

static std::vector<Test> Tests = {
    { "p", { 40, 224, 250, 7, 7 } },
    { "c", { 240, 224, 64 } },
    { "f", { 20, 224, 200, 0 } },
    { "m", { 40, 224, 250,  80, 224, 250,  120, 224, 250,  180, 224, 250, 1 } },
};

static void showError(arly::Compiler::Error error, arly::Token token, const std::string& str, uint32_t lineno, uint32_t charno)
{
    const char* err = "unknown";
    switch(error) {
        case arly::Compiler::Error::None: err = "internal error"; break;
        case arly::Compiler::Error::UnrecognizedLanguage: err = "unrecognized language"; break;
        case arly::Compiler::Error::ExpectedToken: err = "expected token"; break;
        case arly::Compiler::Error::ExpectedType: err = "expected type"; break;
        case arly::Compiler::Error::ExpectedValue: err = "expected value"; break;
        case arly::Compiler::Error::ExpectedString: err = "expected string"; break;
        case arly::Compiler::Error::ExpectedRef: err = "expected ref"; break;
        case arly::Compiler::Error::ExpectedOpcode: err = "expected opcode"; break;
        case arly::Compiler::Error::ExpectedEnd: err = "expected 'end'"; break;
        case arly::Compiler::Error::ExpectedIdentifier: err = "expected identifier"; break;
        case arly::Compiler::Error::ExpectedExpr: err = "expected expression"; break;
        case arly::Compiler::Error::ExpectedLHSExpr: err = "expected left-hand side expression"; break;
        case arly::Compiler::Error::ExpectedArgList: err = "expected arg list"; break;
        case arly::Compiler::Error::ExpectedFormalParams: err = "expected formal params"; break;
        case arly::Compiler::Error::ExpectedFunction: err = "expected function name"; break;
        case arly::Compiler::Error::ExpectedStructType: err = "expected Struct type"; break;
        case arly::Compiler::Error::AssignmentNotAllowedHere: err = "assignment not allowed here"; break;
        case arly::Compiler::Error::InvalidStructId: err = "invalid Struct identifier"; break;
        case arly::Compiler::Error::InvalidParamCount: err = "invalid param count"; break;
        case arly::Compiler::Error::UndefinedIdentifier: err = "undefined identifier"; break;
        case arly::Compiler::Error::ParamOutOfRange: err = "param must be 0..15"; break;
        case arly::Compiler::Error::JumpTooBig: err = "tried to jump too far"; break;
        case arly::Compiler::Error::IfTooBig: err = "too many instructions in if"; break;
        case arly::Compiler::Error::ElseTooBig: err = "too many instructions in else"; break;
        case arly::Compiler::Error::StringTooLong: err = "string too long"; break;
        case arly::Compiler::Error::TooManyConstants: err = "too many constants"; break;
        case arly::Compiler::Error::TooManyVars: err = "too many vars"; break;
        case arly::Compiler::Error::DefOutOfRange: err = "def out of range"; break;
        case arly::Compiler::Error::ExpectedDef: err = "expected def"; break;
        case arly::Compiler::Error::NoMoreTemps: err = "no more temp variables available"; break;
        case arly::Compiler::Error::TempNotAllocated: err = "temp not allocated"; break;
        case arly::Compiler::Error::InternalError: err = "internal error"; break;
        case arly::Compiler::Error::StackTooBig: err = "stack too big"; break;
        case arly::Compiler::Error::MismatchedType: err = "mismatched type"; break;
        case arly::Compiler::Error::WrongType: err = "wrong type"; break;
        case arly::Compiler::Error::WrongNumberOfArgs: err = "wrong number of args"; break;
        case arly::Compiler::Error::OnlyAllowedInLoop: err = "break/continue only allowed in loop"; break;
        case arly::Compiler::Error::DuplicateCmd: err = "duplicate command"; break;
    }
    
    if (token == arly::Token::EndOfFile) {
        err = "unexpected tokens after EOF";
    }
    
    std::cout << "Compile failed: " << err;
    if (!str.empty()) {
        std::cout << " ('" << str << "')";
    }
    std::cout << " on line " << lineno << ":" << charno << "\n";
}

static constexpr int NumLoops = 10;

void setLight(uint8_t i, uint32_t rgb)
{
    printf("    setLight[%d] <== 0x%08x\n", i, rgb);
}

int main(int argc, char * const argv[])
{
    std::cout << "Arly Compiler v0.1\n\n";
    
    int c;
    bool execute = false;
    bool decompile = false;
    bool segmented = false;
    bool headerFile = false;
    std::string outputFile;
    
    while ((c = getopt(argc, argv, "dxsho:")) != -1) {
        switch(c) {
            case 'd': decompile = true; break;
            case 'x': execute = true; break;
            case 's': segmented = true; break;
            case 'h': headerFile = true; break;
            case 'o': outputFile = optarg; break;
            default: break;
        }
    }
    
    // If headerFile is true, segmented is ignored.
    if (headerFile) {
        segmented = false;
    }
    
    if (optind >= argc) {
        std::cout << "No input file given\n";
        return 0;
    }
    
    if (optind != argc - 1) {
        std::cout << "Too many input files given\n";
        return 0;
    }
    
    std::string inputFile = argv[optind];
    
    std::vector<std::pair<int32_t, std::string>> annotations;

    arly::Compiler compiler;
    std::fstream stream;
    stream.open(inputFile.c_str(), std::fstream::in);
    if (stream.fail()) {
        std::cout << "Can't open '" << inputFile << "'\n";
        return 0;
    }
    
    std::cout << "Compiling '" << inputFile << "'\n";
    
    std::vector<uint8_t> executable;
    
    std::cout << "\n\nTrying Arly...\n";
    
    randomSeed(uint32_t(clock()));

    arly::NativeColor nativeColor(setLight, 8);
    
    compiler.compile(&stream, arly::Compiler::Language::Arly, executable, { &nativeColor });
    if (compiler.error() != arly::Compiler::Error::None) {
        showError(compiler.error(), compiler.expectedToken(), compiler.expectedString(), compiler.lineno(), compiler.charno());
        
        std::cout << "\n\nTrying Clover...\n";
        
        stream.seekg(0);
        compiler.compile(&stream, arly::Compiler::Language::Clover, executable, { &nativeColor }, &annotations);

        if (compiler.error() != arly::Compiler::Error::None) {
            showError(compiler.error(), compiler.expectedToken(), compiler.expectedString(), compiler.lineno(), compiler.charno());
            return -1;
        }
    }

    std::cout << "Compile succeeded!\n";
    
    // Write executable if needed
    if (outputFile.size()) {
        // Delete any old copies
        std::string name = outputFile + ".h";
        remove(name.c_str());

        name = outputFile + ".arlx";
        remove(name.c_str());

        for (int i = 0; ; ++i) {
            name = outputFile + std::to_string(i) + ".arlx";
            if (remove(name.c_str()) != 0) {
                break;
            }
        }
        
        std::cout << "\nEmitting executable to '" << outputFile << "'\n";
        std::fstream outStream;
        
        // If segmented break it up into 64 byte chunks, prefix each file with start addr byte
        size_t sizeRemaining = executable.size();
        
        for (uint8_t i = 0; ; i++) {
            if (segmented) {
                name = outputFile + std::to_string(i) + ".arlx";
            } else if (headerFile) {
                name = outputFile + ".h";
            } else {
                name = outputFile + ".arlx";
            }
        
            std::ios_base::openmode mode = std::fstream::out;
            if (!headerFile) {
                mode|= std::fstream::binary;
            }
            
            outStream.open(name.c_str(), mode);
            if (outStream.fail()) {
                std::cout << "Can't open '" << outputFile << "'\n";
                return 0;
            } else {
                char* buf = reinterpret_cast<char*>(&(executable[i * 64]));
                size_t sizeToWrite = sizeRemaining;
                
                if (segmented && sizeRemaining > 64) {
                    sizeToWrite = 64;
                }
                
                if (segmented) {
                    // Write the 2 byte offset
                    uint16_t addr = uint16_t(i) * 64;
                    outStream.put(uint8_t(addr & 0xff));
                    outStream.put(uint8_t(addr >> 8));
                }
                
                if (!headerFile) {
                    // Write the buffer
                    outStream.write(buf, sizeToWrite);
                    if (outStream.fail()) {
                        std::cout << "Save failed\n";
                        return 0;
                    } else {
                        sizeRemaining -= sizeToWrite;
                        outStream.close();
                        std::cout << "    Saved " << name << "\n";
                        if (sizeRemaining == 0) {
                            break;
                        }
                    }
                } else {
                    outStream << "static constexpr uint16_t EEPROM_Upload_Size = " << sizeRemaining << "\n";
                    outStream << "static const uint8_t PROGMEM EEPROM_Upload = {\n";
                    
                    for (size_t i = 0; i < sizeRemaining; ++i) {
                        char hexbuf[5];
                        sprintf(hexbuf, "0x%02x", executable[i]);
                        outStream << hexbuf << ", ";
                        if (i % 8 == 7) {
                            outStream << std::endl;
                        }
                    }

                    outStream << "};\n";
                    
                    break;
                }
            }
        }
        std::cout << "Executables saved\n";
    }

    // decompile if needed
    if (decompile) {
        std::string out;
        arly::Decompiler decompiler(&executable, &out, annotations);
        bool success = decompiler.decompile();
        std::cout << "\nDecompiled executable:\n" << out << "\nEnd decompilation\n\n";
        if (!success) {
            const char* err = "unknown";
            switch(decompiler.error()) {
                case arly::Decompiler::Error::None: err = "internal error"; break;
                case arly::Decompiler::Error::InvalidSignature: err = "invalid signature"; break;
                case arly::Decompiler::Error::InvalidOp: err = "invalid op"; break;
                case arly::Decompiler::Error::PrematureEOF: err = "premature EOF"; break;
            }
            std::cout << "Decompile failed: " << err << "\n\n";
            return 0;
        }
    }
    
    // Execute if needed
    if (execute) {
        arly::NativeModule* mod = &nativeColor;
        Simulator sim(&mod, 1);
        
        sim.setROM(executable);
        
        for (const Test& test : Tests) {
            std::cout << "Simulating '" << test._cmd << "' command...\n";
        
            bool success = sim.init(test._cmd, &test._buf[0], test._buf.size());
            if (success) {
                for (int i = 0; i < NumLoops; ++i) {
                    int32_t delay = sim.loop();
                    if (delay < 0) {
                        success = false;
                        break;
                    }
                    std::cout << "[" << i << "]: delay = " << delay << "\n";
                }
            
                if (success) {
                    std::cout << "Complete\n\n";
                }
            }
            
            if (!success) {
                const char* err = "unknown";
                switch(sim.error()) {
                    case arly::Interpreter::Error::None: err = "internal error"; break;
                    case arly::Interpreter::Error::CmdNotFound: err = "command not found"; break;
                    case arly::Interpreter::Error::UnexpectedOpInIf: err = "unexpected op in if (internal error)"; break;
                    case arly::Interpreter::Error::InvalidOp: err = "invalid opcode"; break;
                    case arly::Interpreter::Error::InvalidNativeFunction: err = "invalid native function"; break;
                    case arly::Interpreter::Error::OnlyMemAddressesAllowed: err = "only Mem addresses allowed"; break;
                    case arly::Interpreter::Error::StackOverrun: err = "can't call, stack full"; break;
                    case arly::Interpreter::Error::StackUnderrun: err = "stack underrun"; break;
                    case arly::Interpreter::Error::StackOutOfRange: err = "stack access out of range"; break;
                    case arly::Interpreter::Error::AddressOutOfRange: err = "address out of range"; break;
                    case arly::Interpreter::Error::InvalidModuleOp: err = "invalid operation in module"; break;
                    case arly::Interpreter::Error::ExpectedSetFrame: err = "expected SetFrame as first function op"; break;
                    case arly::Interpreter::Error::NotEnoughArgs: err = "not enough args on stack"; break;
                    case arly::Interpreter::Error::WrongNumberOfArgs: err = "wrong number of args"; break;
                }
                std::cout << "Interpreter failed: " << err;
                
                int16_t errorAddr = sim.errorAddr();
                if (errorAddr >= 0) {
                    std::cout << " at addr " << errorAddr;
                }
                
                std::cout << "\n\n";
            }
        }
        
    }

    return 1;
}
