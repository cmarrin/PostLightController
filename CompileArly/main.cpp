//
//  main.cpp
//  CompileArly
//
//  Created by Chris Marrin on 1/9/22.
//

#include "Compiler.h"
#include <iostream>
#include <fstream>
#include <getopt.h>

// compile [-o <output file>] [-x] <input file>
//
//      -o      output compiled binary
//      -x      simulate resulting binary
//
int main(int argc, char * const argv[])
{
    std::cout << "Arly Compiler v0.1\n";
    
    int c;
    bool execute = false;
    std::string outputFile;
    
    while ((c = getopt(argc, argv, "xo:")) != -1) {
        switch(c) {
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
        std::cout << "Can't open '" << outputFile << "'\n";
        return 0;
    }
    
    std::cout << "Compiling '" << inputFile << "'\n";
    
    compiler.compile(&stream);
    if (compiler.error() == arly::Compiler::Error::None) {
        std::cout << "Compile succeeded!\n";
    } else {
        const char* err = "unknown";
        switch(compiler.error()) {
            case arly::Compiler::Error::None: err = "internal error"; break;
            case arly::Compiler::Error::ExpectedToken: err = "expected token"; break;
            case arly::Compiler::Error::ExpectedType: err = "expected type"; break;
            case arly::Compiler::Error::ExpectedValue: err = "expected value"; break;
            case arly::Compiler::Error::ExpectedOpcode: err = "expected opcode"; break;
            case arly::Compiler::Error::ExpectedEnd: err = "expected 'end'"; break;
            case arly::Compiler::Error::ExpectedIdentifier: err = "expected id"; break;
            case arly::Compiler::Error::ExpectedCommandId: err = "expected command"; break;
            case arly::Compiler::Error::InvalidParamCount: err = "invalid param count"; break;

        }
        
        std::cout << "Compile failed: " << err << " on line " << compiler.lineno() << "\n";
    }

    return 0;
}
