//
//  main.cpp
//  CompileArly
//
//  Created by Chris Marrin on 1/9/22.
//

#include "Compiler.h"
#include "Decompiler.h"
#include "Interpreter.h"
#include <iostream>
#include <fstream>
#include <getopt.h>

// Simulator
//
// Subclass of Interpreter that outputs device info to consolee
class Simulator : public arly::Interpreter
{
public:
    virtual uint8_t rom(uint16_t i) const override
    {
        return (i < 1024) ? _rom[i] : 0;
    }
    
    virtual void setLight(uint8_t i, uint32_t rgb) override
    {
        printf("    setLight[%d] <== 0x%08x\n", i, rgb);
    }
    
    virtual uint8_t numPixels() const override
    {
        return 8;
    }

    void logAddr(uint16_t addr) const { std::cout << "[" << addr << "]"; }
    
    virtual void log(uint16_t addr, uint8_t r, int32_t v) const override
    {
        logAddr(addr);
        std::cout << ": r[" << uint32_t(r) << "] = " << v << std::endl;
    }
    virtual void logFloat(uint16_t addr, uint8_t r, float v) const override
    {
        logAddr(addr);
        std::cout << ": r[" << uint32_t(r) << "] = " << v << std::endl;
    }

    virtual void logColor(uint16_t addr, uint8_t r, const Color& c) const override
    {
        logAddr(addr);
        std::cout << ": c[" << uint32_t(r) << "] = (" << c.hue() << ", " << c.sat() << ", " << c.val() << ")" << std::endl;
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
//      -o      output compiled binary
//      -d      decompile and print result
//      -x      simulate resulting binary
//
int main(int argc, char * const argv[])
{
    std::cout << "Arly Compiler v0.1\n\n";
    
    int c;
    bool execute = false;
    bool decompile = false;
    std::string outputFile;
    
    while ((c = getopt(argc, argv, "dxo:")) != -1) {
        switch(c) {
            case 'd': decompile = true; break;
            case 'x': execute = true; break;
            case 'o': outputFile = optarg; break;
            default: break;
        }
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
    
    arly::Compiler compiler;
    std::fstream stream;
    stream.open(inputFile.c_str(), std::fstream::in);
    if (stream.fail()) {
        std::cout << "Can't open '" << inputFile << "'\n";
        return 0;
    }
    
    std::cout << "Compiling '" << inputFile << "'\n";
    
    std::vector<uint8_t> executable;
    
    compiler.compile(&stream, executable);
    if (compiler.error() == arly::Compiler::Error::None) {
        std::cout << "Compile succeeded!\n";
    } else {
        const char* err = "unknown";
        switch(compiler.error()) {
            case arly::Compiler::Error::None: err = "internal error"; break;
            case arly::Compiler::Error::ExpectedToken: err = "expected token"; break;
            case arly::Compiler::Error::ExpectedType: err = "expected type"; break;
            case arly::Compiler::Error::ExpectedValue: err = "expected value"; break;
            case arly::Compiler::Error::ExpectedInt: err = "expected int"; break;
            case arly::Compiler::Error::ExpectedOpcode: err = "expected opcode"; break;
            case arly::Compiler::Error::ExpectedEnd: err = "expected 'end'"; break;
            case arly::Compiler::Error::ExpectedIdentifier: err = "expected identifier"; break;
            case arly::Compiler::Error::ExpectedRegister: err = "expected register"; break;
            case arly::Compiler::Error::ExpectedCommandId: err = "expected command"; break;
            case arly::Compiler::Error::InvalidParamCount: err = "invalid param count"; break;
            case arly::Compiler::Error::UndefinedIdentifier: err = "undefined identifier"; break;
            case arly::Compiler::Error::ParamOutOfRange: err = "param must be 0..15"; break;
            case arly::Compiler::Error::ForEachTooBig: err = "too many instructions in foreach"; break;
            case arly::Compiler::Error::IfTooBig: err = "too many instructions in if"; break;
            case arly::Compiler::Error::ElseTooBig: err = "too many instructions in else"; break;
            case arly::Compiler::Error::TooManyConstants: err = "too many constants"; break;
            case arly::Compiler::Error::TooManyDefs: err = "too many defs"; break;
        }
        
        std::cout << "Compile failed: " << err << " on line " << compiler.lineno() << "\n";
        return 0;
    }
    
    // Write executable if needed
    if (outputFile.size()) {
        std::cout << "\nEmitting executable to '" << outputFile << "'\n";
        std::fstream outStream;
        outStream.open(outputFile.c_str(),
                    std::fstream::out | std::fstream::binary | std::fstream::trunc);
        if (outStream.fail()) {
            std::cout << "Can't open '" << outputFile << "'\n";
            return 0;
        } else {
            char* buf = reinterpret_cast<char*>(&(executable[0]));
            outStream.write(buf, executable.size());
            if (outStream.fail()) {
                std::cout << "Save failed\n";
                return 0;
            } else {
                outStream.close();
                std::cout << "Executable saved\n";
            }
        }
    }

    // decompile if needed
    if (decompile) {
        std::string out;
        arly::Decompiler decompiler(&executable, &out);
        bool success = decompiler.decompile();
        std::cout << "\nDecompiled executable:\n" << out << "\nEnd decompilation\n\n";
        if (!success) {
            const char* err = "unknown";
            switch(decompiler.error()) {
                case arly::Decompiler::Error::None: err = "internal error"; break;
                case arly::Decompiler::Error::InvalidSignature: err = "invalid signature"; break;
                case arly::Decompiler::Error::InvalidOp: err = "invalid op"; break;
            }
            std::cout << "Decompile failed: " << err << "\n\n";
            return 0;
        }
    }
    
    // Execute if needed
    if (execute) {
        Simulator sim;
        sim.setROM(executable);
        
        uint8_t buf[16];
        
        // Test 'c' command
        std::cout << "Simulating 'c' command...\n";
        
        bool success = true;

        buf[0] = 240;
        buf[1] = 224;
        buf[2] = 64;
        success = sim.init('c', buf, 3);
        if (success) {
            for (int i = 0; i < 10; ++i) {
                success = sim.loop() >= 0;
                if (!success) {
                    break;
                }
            }
            
            if (success) {
                std::cout << "Complete\n\n";
            
                // Test 'f' command
                std::cout << "Simulating 'f' command...\n";

                buf[0] = 20;
                buf[1] = 224;
                buf[2] = 200;
                buf[3] = 0;
                success = sim.init('f', buf, 4);
                if (success) {
                    for (int i = 0; i < 100; ++i) {
                        success = sim.loop() >= 0;
                        if (!success) {
                            break;
                        }
                        std::cout << "[" << i << "]\n";

                    }
                    if (success) {
                        std::cout << "Complete\n\n";
                    }
                }
            }
        }
        
        if (!success) {
            const char* err = "unknown";
            switch(sim.error()) {
                case arly::Interpreter::Error::None: err = "internal error"; break;
                case arly::Interpreter::Error::CmdNotFound: err = "command not found"; break;
                case arly::Interpreter::Error::NestedForEachNotAllowed: err = "nested foreach not allowed"; break;
                case arly::Interpreter::Error::UnexpectedOpInIf: err = "unexpected op in if (internal error)"; break;
                case arly::Interpreter::Error::InvalidOp: err = "invalid opcode"; break;
                case arly::Interpreter::Error::OnlyMemAddressesAllowed: err = "only Mem addresses allowed"; break;
            }
            std::cout << "Interpreter failed: " << err;
            
            int16_t errorAddr = sim.errorAddr();
            if (errorAddr >= 0) {
                std::cout << " at addr " << errorAddr;
            }
            
            std::cout << "\n\n";
            return 0;
        }
    }

    return 1;
}
