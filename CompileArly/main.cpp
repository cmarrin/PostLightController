//
//  main.cpp
//  CompileArly
//
//  Created by Chris Marrin on 1/9/22.
//

#include "Compiler.h"
#include "Decompiler.h"
#include <iostream>
#include <fstream>
#include <getopt.h>

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

    return 0;
}
