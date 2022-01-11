/*-------------------------------------------------------------------------
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2021, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include <cstdint>
#include <istream>

namespace arly {

static constexpr uint8_t MAX_ID_LENGTH = 32;

//////////////////////////////////////////////////////////////////////////////
//
//  Class: Scanner
//
//  
//
//////////////////////////////////////////////////////////////////////////////

// Tokens for special chars are the same as their ASCII code. Other tokens 
// have values above 0x7f. Script parsers rely on these facts. Don't change 
// them unless you change those other places.
enum class Token : uint8_t {
    None        = 0x83,
    Unknown     = 0x84,
    Comment     = 0x85,
    Whitespace  = 0x86,
    Float       = 0x87,
    Identifier  = 0x88,
    NewLine     = 0x89,
    Integer     = 0x8a,
    Special     = 0x8c,
    Error       = 0x8d,
    EndOfFile   = 0x8e,
};

static inline bool isdigit(uint8_t c)		{ return c >= '0' && c <= '9'; }
static inline bool isLCHex(uint8_t c)       { return c >= 'a' && c <= 'f'; }
static inline bool isUCHex(uint8_t c)       { return c >= 'A' && c <= 'F'; }
static inline bool isHex(uint8_t c)         { return isUCHex(c) || isLCHex(c); }
static inline bool isxdigit(uint8_t c)		{ return isHex(c) || isdigit(c); }
static inline bool isUpper(uint8_t c)		{ return (c >= 'A' && c <= 'Z'); }
static inline bool isLower(uint8_t c)		{ return (c >= 'a' && c <= 'z'); }
static inline bool isLetter(uint8_t c)		{ return isUpper(c) || isLower(c); }
static inline bool isIdFirst(uint8_t c)		{ return isLetter(c) || c == '$' || c == '_'; }
static inline bool isIdOther(uint8_t c)		{ return isdigit(c) || isIdFirst(c); }
static inline bool isspace(uint8_t c)       { return c == ' ' || c == '\r' || c == '\f' || c == '\t' || c == '\v'; }
static inline bool isnewline(uint8_t c)     { return c == '\n'; }
static inline uint8_t tolower(uint8_t c)    { return isUpper(c) ? (c - 'A' + 'a') : c; }
static inline uint8_t toupper(uint8_t c)    { return isLower(c) ? (c - 'a' + 'A') : c; }

static constexpr uint8_t C_EOF = static_cast<uint8_t>(Token::EndOfFile);

class Scanner  {
public:
    typedef struct {
        float   	    number;
        uint32_t        integer;
        const char*     str;
    } TokenType;

  	Scanner(std::istream* stream = nullptr)
  	 : _lastChar(0xff)
  	 , _stream(stream)
     , _lineno(1)
  	{
    }
  	
  	~Scanner()
  	{
    }
    
    void setStream(std::istream* stream) { _stream = stream; }
  
    uint32_t lineno() const { return _lineno; }
  	
    Token getToken()
    {
        if (_currentToken == Token::None) {
            _currentToken = getToken(_currentTokenValue);
        }
        return _currentToken;
    }
    
    const Scanner::TokenType& getTokenValue()
    {
        if (_currentToken == Token::None) {
            _currentToken = getToken(_currentTokenValue);
        }
        return _currentTokenValue;
    }
    
    const std::string getTokenString()
    {
        const TokenType tokenType = getTokenValue();
        return (_currentToken == Token::Identifier) ? tokenType.str : "";
    }
    
    void retireToken() { _currentToken = Token::None; }

private:
  	Token getToken(TokenType& token);

    uint8_t get() const;
    
	void putback(uint8_t c) const
	{
  		assert(_lastChar == C_EOF && c != C_EOF);
  		_lastChar = c;
	}

  	Token scanIdentifier();
  	Token scanNumber(TokenType& tokenValue);
  	Token scanComment();
  	int32_t scanDigits(int32_t& number, bool hex);
  	bool scanFloat(int32_t& mantissa, int32_t& exp);
    
  	mutable uint8_t _lastChar;
  	std::string _tokenString;
  	std::istream* _stream;
    mutable uint32_t _lineno;

    Token _currentToken = Token::None;
    Scanner::TokenType _currentTokenValue;
    
    bool _lastCharIsNewLine = false;
};

}
