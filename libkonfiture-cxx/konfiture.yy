//
// konfiture.yy
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

%skeleton "lalr1.cc"
%require  "2.5"
%debug
%defines
%define namespace "konfiture"
%define parser_class_name "Konfiture_Parser"

%code requires{
    #include <string>
    #include <list>
    
    // forward declaration of konfiture classes
    namespace konfiture {
        class Konfiture_Lexer;
        class Konfiture_Driver;
        class KFAttribute;
        class KFNode;
    }
}
%locations

%lex-param      { konfiture::Konfiture_Lexer  &lexer  }
%parse-param    { konfiture::Konfiture_Lexer  &lexer  }

%lex-param      { konfiture::Konfiture_Driver &driver }
%parse-param    { konfiture::Konfiture_Driver &driver }

%code top {
    #include "konfiture_lexer.hh"
    #include "konfiture_attribute.hh"
    #include "konfiture_node.hh"
    #include "konfiture_driver.hh"

    using konfiture::KFNode;
    using konfiture::KFAttribute;
    
    static int yylex(konfiture::Konfiture_Parser::semantic_type *yylval,
                     konfiture::Konfiture_Parser::location_type *location,
                     konfiture::Konfiture_Lexer &lexer,
                     konfiture::Konfiture_Driver &driver);
}

%union {
    std::string *sval;
    konfiture::KFAttribute *attrval;
    konfiture::KFNode *nodeval;
    std::list<konfiture::KFAttribute*> *attrlist;
    std::list<konfiture::KFNode*> *nodelist;
}

%token<sval>    IDENT
%token<sval>    STRING
%token          NEWLINE
%token          END     0   "end of file"

%type<attrval> attribute
%type<nodeval> member

%type<sval> rvalue
%type<attrlist> attributes
%type<nodelist> members

%destructor { if ($$) delete $$; $$ = nullptr; } IDENT STRING
%destructor { if ($$) delete $$; $$ = nullptr; } attribute
%destructor { if ($$) delete $$; $$ = nullptr; } member
%destructor { if ($$) { for (auto &i: *$$) delete i; }; $$->clear(); delete $$; $$ = nullptr; } attributes
%destructor { if ($$) { for (auto &i: *$$) delete i; }; $$->clear(); delete $$; $$ = nullptr; } members
%start tree
%%

tree
    :           /* nothing */   { driver.parsed = new konfiture::KFNode(""); }
    | newline   /* newline */   { driver.parsed = new konfiture::KFNode(""); }
    | members                   { driver.parsed = new konfiture::KFNode(""); (driver.parsed)->nodes = *$1; }
    | newline members           { driver.parsed = new konfiture::KFNode(""); (driver.parsed)->nodes = *$2; }
    ;

members
    : member                    { $$ = new std::list<KFNode*>(1, $1); }
    | member newline            { $$ = new std::list<KFNode*>(1, $1); }
    | member newline members    { $$ = $3; $$->push_front($1); }
    ;

newline
    : NEWLINE
    | newline NEWLINE
    ;

member
    : IDENT                                     { $$ = new konfiture::KFNode(*$1);                                   }
    | IDENT rvalue                              { $$ = new konfiture::KFNode(*$1, *$2);                              }
    | IDENT '=' rvalue                          { $$ = new konfiture::KFNode(*$1, *$3);                              }
    | IDENT attributes                          { $$ = new konfiture::KFNode(*$1); $$->attrs = *$2;                  }
    | IDENT '{' newline members '}'             { $$ = new konfiture::KFNode(*$1); $$->nodes = *$4;                  }
    | IDENT rvalue '{' newline members '}'      { $$ = new konfiture::KFNode(*$1, *$2); $$->nodes = *$5;             }
    | IDENT attributes '{' newline members '}'  { $$ = new konfiture::KFNode(*$1); $$->attrs = *$2; $$->nodes = *$5; }
    ;

rvalue
    : IDENT  { $$ = $1; }
    | STRING { $$ = $1; }
    ;

attributes
    : attribute             { $$ = new std::list<KFAttribute*>(1, $1); }
    | attribute attributes  { $$ = $2; $$->push_front($1); }
    ;

attribute
    : IDENT '=' rvalue      { $$ = new KFAttribute(*$1, *$3); delete $1; delete $3; }
    ;


%%

void konfiture::Konfiture_Parser::error(const konfiture::Konfiture_Parser::location_type& location,
                                  const std::string& err_message) {
    std::cerr << "Error(" << location << "): " << err_message << std::endl;
}

static int yylex(konfiture::Konfiture_Parser::semantic_type *yylval,
                 konfiture::Konfiture_Parser::location_type *location,
                 konfiture::Konfiture_Lexer &lexer,
                 konfiture::Konfiture_Driver &driver) {
    return lexer.yylex(yylval, location);
}

