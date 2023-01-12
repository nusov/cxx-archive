//
// konfiture_lexer.hh
//
// Author:
//   Alexander Nusov 
//
// Copyright (C) 2014 Alexander Nusov
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

#ifndef __KONFITURE_LEXER_HH__
#define __KONFITURE_LEXER_HH__

#ifndef yyFlexLexerOnce
#include <FlexLexer.h>
#endif

#include "konfiture.tab.hh"

#undef  YY_DECL
#define YY_DECL int konfiture::Konfiture_Lexer::yylex()

namespace konfiture {
    class Konfiture_Lexer : public yyFlexLexer {
    public:
        Konfiture_Lexer(std::istream *in) : yyFlexLexer(in), yylval(nullptr){};
        
        int yylex(konfiture::Konfiture_Parser::semantic_type *lval,
                  konfiture::Konfiture_Parser::location_type *location) {
            yylval = lval;
            yylocation = location;
            return yylex();
        }

    private:
        int yylex();
        konfiture::Konfiture_Parser::semantic_type *yylval;
        konfiture::Konfiture_Parser::location_type *yylocation;
    };
}

#endif

