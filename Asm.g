
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
    namespaceStd = "std";
    namespaceAntlr = "antlr";
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
asmFile: (asmStatementList)?	// Empty files permitted.
	{ std::cerr << "finished parsing file\n"; }
    ;
asmStatementList: (asmCompleteStatement)+ ;

asmEnd: Newline | SEMI ;

asmCompleteStatement
    : (asmLabel) => asmLabel (asmInstr)? asmEnd	{ std::cerr << "label\n"; }
    | asmInstr asmEnd				{ std::cerr << "instr\n"; }
    | asmCommand asmEnd				{ std::cerr << "command\n"; }
    | asmEnd					{ std::cerr << "empty\n"; }
    ;

asmLabel
    : ID COLON
    | DOT ID COLON
    ;

asmCommand
    : ".macro" ID everythingElse
    | ".endm" 
    | DOT ID everythingElse
    ;

asmInstr
    : asmInnocuousInstr everythingElse
    | asmSensitive everythingElse
    ;

everythingElse: (param (COMMA param)* )* ;

param: String | ID | Number | Reg;


asmSensitive {int pad;}
    : pad = asmSensitiveInstr
    | pad = asmSensitiveRegInstr
    ;

asmInnocuousInstr
    : i:ID				{ std::cout << i->getText(); }
    ;

asmSensitiveRegInstr returns [int pad] {pad=0;}
    : ("pop"  | "popl"  | "popd")	{pad=5;}
    | ("push" | "pushl" | "pushd")	{pad=5;}
    ;

asmSensitiveInstr returns [int pad] {pad=0;}
    : ("popf"  | "popfl"  | "popfd")	{pad=21;}
    | ("pushf" | "pushfl" | "pushfd")	{pad=5;}
    | lgdt	{pad=9;}
    | sgdt	{pad=8;}
    | lidt	{pad=9;}
    | sidt	{pad=8;}
    | ljmp	{pad=9;}
    | (lds | les | lfs | lgs | lss)	{pad=16;}
    | clts	{pad=14;}
    | hlt	{pad=6;}
    | cli	{pad=7;}
    | sti	{pad=23;}
    | lldt	{pad=16;}
    | sldt	{pad=6;}
    | ltr	{pad=16;}
    | str	{pad=9;}
    | in	{pad=13;}
    | out	{pad=16;}
    | invlpg	{pad=6;}
    | iret	{pad=4;}
    | lret	{pad=4;}
    | cpuid	{pad=6;}
    | wrmsr	{pad=8;}
    | rdmsr	{pad=8;}
    | softint	{pad=11;}
    ;

lgdt	: "lgdt" | "lgdtl";
sgdt	: "sgdt" | "sgdtl";
lidt	: "lidt" | "lidtl";
sidt	: "sidt" | "sidtl";
ljmp	: "ljmp";
lds	: "lds";
les	: "les";
lfs	: "lfs";
lgs	: "lgs";
lss	: "lss";
clts	: "clts";
hlt	: "hlt";
cli	: "cli";
sti	: "sti";
lldt	: "lldt";
sldt	: "sldt" | "sldtl";
ltr	: "ltr";
str	: "str" | "strl";
in	: "inb" | "inw" | "inl";
out	: "outb" | "outw" | "outl";
invlpg	: "invlpg";
iret	: "iret" | "iretl" | "iretd";
lret	: "lret";
cpuid	: "cpuid";
wrmsr	: "wrmsr";
rdmsr	: "rdmsr";
softint	: "int";
push	: "push" | "pushl";
pop	: "pop"  | "popl";

{
    // Global stuff in the cpp file.
}

class AsmLexer extends Lexer;
options {
    k = 3;
    caseSensitive = true;
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

protected Letter : 'a'..'z' | 'A'..'Z' | '_';
protected Digit  : '0'..'9';

Int    : (Digit)+;
String : '"' ( ~('"') )* '"' ;

ID options {testLiterals=true;} : Letter (Letter | Digit)* ;

Reg: '%' ("eax" | "ebx" | "ecx" | "edx" | "esi" | "edi" | "esp");


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
