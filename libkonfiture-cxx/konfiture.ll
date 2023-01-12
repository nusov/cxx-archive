%{
//
// konfiture.ll
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

#include <string>
#include "konfiture_lexer.hh"
#include "konfiture_utils.hh"

typedef konfiture::Konfiture_Parser::token_type token_type;
typedef konfiture::Konfiture_Parser::token      token;
%}

%option debug
%option nodefault
%option noyywrap
%option stdinit
%option nounistd
%option c++
%option yyclass="Konfiture_Lexer"

%{
#undef YY_INTERACTIVE
#define YY_USER_ACTION yylocation->columns(YYLeng());
%}

%x IN_COMMENT
%%

<INITIAL>{
"/*"    BEGIN(IN_COMMENT);
}
<IN_COMMENT>{
"*/"        BEGIN(INITIAL);
[^*\n]+     // eat comment in chunks
"*"         // eat the lone star
\n          { yylocation->lines(YYLeng());
              yylocation->step(); }
}

\/\/.*      // eat comment

[\-\:\._a-zA-Z0-9]* { yylval->sval = new std::string(YYText());
                      return token::IDENT; }
\"[^\n]*\"          { yylval->sval = new std::string(konfiture::kf_unescape(std::string(YYText()).substr(1, YYLeng()-2))); 
                      return token::STRING; }
\'[^\n]*\'          { yylval->sval = new std::string(std::string(YYText()).substr(1, YYLeng()-2));
                      return token::STRING; }

[ \t\r]+            { yylocation->step(); }

\n  { yylocation->lines(YYLeng());
      yylocation->step();
      return token::NEWLINE; }
.   { return static_cast<token_type>(*yytext); }

%%

