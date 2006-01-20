
header "pre_include_hpp" {
    // Inserted before antlr generated includes in the header file.
}
header "post_include_hpp" {
    // Inserted after antlr generated includes in the header file
    // outside any generated namespace specifications
}
header "pre_include_cpp" {
    // Inserted before the antlr generated includes in the cpp file
}
header "post_include_cpp" {
    // Inserted after the antlr generated includes in the cpp file
}

options {
    language = "Cpp";
    namespace = "Asm";
//    namespaceStd = "std";
//    namespaceAntlr = "antlr";
    genHashLines = true;
}

{
    // Global stuff in the cpp file.
}

class AsmParser extends Parser;
options {
    buildAST = true;
    k = 2;
}
{
    // Additional methods and members.
}

// Rules
asmFile: (asmStatements)? ;	// Empty files permitted.

asmStatements
    : asmStatement (SEMI | Newline)
    ;

asmStatement
    : asmLabel
    | asmInstrCall
    | asmCommand
    ;

asmLabel: ID COLON;

asmCommand: DOT ID asmCommandParams;

asmInstrCall
    : asmMacro macroParams
    | asmInstr asmParams
    | asmSensitiveInstr asmParams
    ;

asmParams: (asmParam (COMMA asmParam)* )*
    ;

macroParams: (macroParam (COMMA macroParam)* )*
    ;

asmCommandParams: (asmCommandParam (COMMA asmCommandParam)* )*
    ;

asmParam
    : ID
    | Number
    ;

macroParam: String | asmParam;

asmCommandParam: ID | Number | String;

asmMacro: ID ;

asmInstr: Iadd | Isub
    ;

asmSensitiveInstr: Ipopf | Ipushf
    ;

{
    // Global stuff in the cpp file.
}

class AsmLexer extends Lexer;
options {
    k = 3;
    caseSensitive = false;
    testLiterals = false;
}
{
    // Additional methods and members.
}

// Rules
COMMA	: ',' ;
SEMI	: ';' ;
COLON	: ':' ;
DOT	: '.' ;

LPAREN		: '(' ;
RPAREN		: ')' ;
LBRACKET	: '[' ;
RBRACKET	: ']' ;
LCURLY		: '{' ;
RCURLY		: '}' ;

Newline
    : '\r' '\n'	{ newline(); } // DOS
    | '\n'	{ newline(); } // Sane systems
    ;

protected String: '"' ( ~('"') )* '"' ;

Comment
    : ((CCommentBegin) => CComment | CPPComment)
	{ _ttype = ANTLR_USE_NAMESPACE(antlr)Token::SKIP; }
    ;
protected CCommentBegin : '/''*';
protected CCommentEnd   : '*''/' ;
protected CComment
    : CCommentBegin (options {greedy=false;} :.)* CCommentEnd ;
protected CPPComment: '/''/' ( ~('\n') )* ;	// Don't swallow the newline.

protected Letter : 'a'..'z' | '_';
protected Digit  : '0'..'9';
protected Int    : (Digit)+;
protected ID options {testLiterals=true;}
    : Letter (Letter | Digit)*
    ;

Reg: '%' ("eax" | "ebx" | "ecx" | "edx" | "esi" | "edi" | "esp");

Iadd	: "add" ('l')?;
Isub	: "sub" ('l')?;

Ipopf	: "popf";
Ipushf	: "pushf";

{
    // Global stuff in the cpp file.
}

class AsmTreeParser extends TreeParser;
options {
    k = 2;
}
{
    // Additional methods and members.
}

asmFile: ;

