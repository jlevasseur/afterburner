
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
//    buildAST = true;
    k = 2;
}
{
    // Additional methods and members.
}

// Rules
asmFile: (asmStatements)? ;	// Empty files permitted.

asmStatements
    : (asmStatement)? (SEMI | Newline)+
    ;

asmStatement
    : asmLabel
    | asmInstrCall
    | asmCommand
    ;

asmLabel: ID COLON;

asmCommand
    : Command asmCommandParams
    | Cmacro asmCommandParams
    | Cendm asmCommandParams
    ;

asmInstrCall
    : asmInvoke macroParams
    | asmSensitive
    ;

asmInvoke: ID ;

asmParams: (asmParam (COMMA asmParam)* )* ;
macroParams: (macroParam (COMMA macroParam)* )* ;
asmCommandParams: (asmCommandParam (COMMA asmCommandParam)* )* ;

macroParam: String | asmParam;
asmCommandParam: ID | Number | String;

asmParam
    : ID
    | Number
    ;


asmSensitive {int pad;}
    : pad = asmSensitiveInstr asmParams { std::cout << pad; };

asmSensitiveRegInstr returns [int pad] {pad=0;}
    : Ipop	{pad=5;}
    | Ipush	{pad=5;}
    ;

asmSensitiveInstr returns [int pad] {pad=0;}
    : Ipopf	{pad=21;}
    | Ipushf	{pad=5;}
    | Ilgdt	{pad=9;}
    | Isgdt	{pad=8;}
    | Ilidt	{pad=9;}
    | Isidt	{pad=8;}
    | Iljmp	{pad=9;}
    | (Ilds | Iles | Ilfs | Ilgs | Ilss)	{pad=16;}
    | Iclts	{pad=14;}
    | Ihlt	{pad=6;}
    | Icli	{pad=7;}
    | Isti	{pad=23;}
    | Illdt	{pad=16;}
    | Isldt	{pad=6;}
    | Iltr	{pad=16;}
    | Istr	{pad=9;}
    | Iin	{pad=13;}
    | Iout	{pad=16;}
    | Iinvlpg	{pad=6;}
    | Iiret	{pad=4;}
    | Ilret	{pad=4;}
    | Icpuid	{pad=6;}
    | Iwrmsr	{pad=8;}
    | Irdmsr	{pad=8;}
    | Iint	{pad=11;}
    ;

{
    // Global stuff in the cpp file.
}

class AsmLexer extends Lexer;
options {
    k = 5;
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

Whitespace
    : (' ' | '\t' | '\014')
	{ _ttype = ANTLR_USE_NAMESPACE(antlr)Token::SKIP; }
    ;

Comment
    : ((CCommentBegin) => CComment | CPPComment)
	{ _ttype = ANTLR_USE_NAMESPACE(antlr)Token::SKIP; }
    ;
protected CCommentBegin : '/''*';
protected CCommentEnd   : '*''/' ;
protected CComment
    : CCommentBegin (options {greedy=false;}
                     : '\n' { newline(); } | ~('\n') )* 
      CCommentEnd ;
protected CPPComment: '/''/' ( ~('\n') )* ;	// Don't swallow the newline.

protected Letter : 'a'..'z' | '_';
protected Digit  : '0'..'9';

Int    : (Digit)+;
String : '"' ( ~('"') )* '"' ;

protected ID options {testLiterals=true;}
    : Letter (Letter | Digit)*
    ;

protected Command options {testLiterals=true;}
    : DOT ID
    ;

Reg: '%' ("eax" | "ebx" | "ecx" | "edx" | "esi" | "edi" | "esp");

Cmacro	: ".macro";
Cendm	: ".endm";

Ipushf	: "pushf" ('l' | 'd');
Ipopf	: "popf"  ('l' | 'd');
Ilgdt	: "lgdt" ('l')? ;
Isgdt	: "sgdt" ('l')? ; 
Ilidt	: "lidt" ('l')? ;
Isidt	: "sidt" ('l')? ;
Iljmp	: "ljmp";
Ilds	: "lds";
Iles	: "les";
Ilfs	: "lfs";
Ilgs	: "lgs";
Ilss	: "lss";
Iclts	: "clts";
Ihlt	: "hlt";
Icli	: "cli";
Isti	: "sti";
Illdt	: "lldt";
Isldt	: "sldt" ('l')? ;
Iltr	: "ltr";
Istr	: "str" ('l')? ;
Iin	: "in"  ('b' | 'w' | 'l');
Iout	: "out" ('b' | 'w' | 'l');
Iinvlpg	: "invlpg";
Iiret	: "iret" ('l' | 'd')? ;
Ilret	: "lret";
Icpuid	: "cpuid";
Iwrmsr	: "wrmsr";
Irdmsr	: "rdmsr";
Iint	: "int";
Ipush	: "push" ('l')? ;
Ipop	: "pop"  ('l')? ;


/*
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
*/
