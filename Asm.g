/* tutorial: http://javadude.com/articles/antlrtut/ */

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
    buildAST = true;
    k = 2;
}
{
    // Additional methods and members.
}

// Rules
asmFile: asmBlocks
    ;

asmBlocks: (asmAnonymousBlock)? (asmBasicBlock)* ;

asmAnonymousBlock: (asmStatement)+
    { ## = #([ASTBasicBlock], ##); }
    ;

asmBasicBlock: asmLabel (asmStatement)+ 
    { ## = #([ASTBasicBlock], ##); }
    ;

asmEnd: Newline | SEMI ;

asmStatement
    : asmInstrPrefix asmEnd
    | asmInstr asmEnd
    | asmCommand asmEnd
    | asmEnd
    ;

asmLabel : Label | LocalLabel ;

asmMacroDef
    : ".macro" ID (ID (COMMA ID)*)? asmEnd
               asmBlocks
      ".endm"
      { ## = #([ASTMacroDef], ##); }
    ;

asmCommand
    : asmMacroDef
    | Command everythingElse		{ ## = #([ASTCommand], ##); }
    ;

asmInstr
    : asmInnocuousInstr everythingElse	{ ## = #([ASTInstruction], ##); }
    | asmSensitive			{ ## = #([ASTSensitive], ##); }
    ;

everythingElse: (param | COMMA)* ;
param: String | ID | Int | asmReg | Option | Command | DOLLAR | PLUS | MINUS | STAR | DOT | LPAREN | RPAREN;

regOffsetExpression
    : expression (LPAREN ((Int)? COMMA)? asmReg (COMMA (Int)?)? RPAREN)*
    | LPAREN asmReg RPAREN
    | asmReg
    ;

primitive
    : ID | Int | DOT | Command
    ;

signExpression
    : ((PLUS|MINUS))* primitive
    ;
makeConstantExpression
    : (DOLLAR)? signExpression
    ;
multiplyingExpression
    : makeConstantExpression ((STAR|DIV|"mod") makeConstantExpression)*
    ;
addingExpression
    : multiplyingExpression ((PLUS|MINUS) multiplyingExpression)*
    ;

expression : addingExpression ;
instrParam : regOffsetExpression ;

asmSensitive {int pad;}
    : pad = asmSensitiveInstr everythingElse
    | pad = asmSensitiveRegInstr
    ;

asmInnocuousInstr
    : ID
    ;

asmLowReg: "%al" | "%bl" | "%cl" | "%dl";
asmHighReg: "%ah" | "%bh" | "%ch" | "%dh";
asmReg
    : "%eax" | "%ebx" | "%ecx" | "%edx" | "%esi" | "%edi" | "%esp" | "%ebp"
    | asmLowReg
    | asmHighReg
    ;
asmSensitiveReg
    : "%cs" | "%ds" | "%es" | "%fs" | "%gs" 
    | "%cr0" | "%cr2" | "%cr3" | "%cr4" 
    | "%db0" | "%db1" | "%db2" | "%db3" | "%db4" | "%db5" | "%db6" | "%db7"
    ;

asmInstrPrefix
    : ("lock" | "rep")		{ ## = #([ASTInstructionPrefix], ##); }
    ;

asmSensitiveRegInstr returns [int pad] {pad=0;}
    : ("pop"  | "popl"  | "popd")  (asmSensitiveReg {pad=5;} | instrParam)
    | ("push" | "pushl" | "pushd") (asmSensitiveReg {pad=5;} | instrParam)
    | ("mov"  | "movl" )
        (
	  (instrParam COMMA asmSensitiveReg) => (instrParam COMMA asmSensitiveReg {pad=12;})
        | (asmSensitiveReg COMMA instrParam {pad=12;}) 
	| (instrParam COMMA instrParam)
	)
    ;

asmSensitiveInstr returns [int pad] {pad=8;}
    : ("popf"  | "popfl"  | "popfd")	{pad=21;}
    | ("pushf" | "pushfl" | "pushfd")	{pad=5;}
    | ("lgdt" | "lgdtl")		{pad=9;}
    | ("sgdt" | "sgdtl")
    | ("lidt" | "lidtl")		{pad=9;}
    | ("sidt" | "sidtl")
    | "ljmp"				{pad=9;}
    | ("lds" | "les" | "lfs" | "lgs" | "lss")	{pad=16;}
    | "clts"				{pad=14;}
    | "hlt"				{pad=6;}
    | "cli"				{pad=7;}
    | "sti"				{pad=23;}
    | "lldt"				{pad=16;}
    | ("sldt" | "sldtl")		{pad=6;}
    | "ltr"				{pad=16;}
    | ("str" | "strl")			{pad=9;}
    | ("inb"  | "inw"  | "inl")		{pad=13;}
    | ("outb" | "outw" | "outl" )	{pad=16;}
    | "invlpg"				{pad=6;}
    | ("iret" | "iretl" | "iretd")	{pad=4;}
    | "lret"				{pad=4;}
    | "cpuid"				{pad=6;}
    | "wrmsr"
    | "rdmsr"
    | "int"				{pad=11;}
    | "ud2"
    | "invd"
    | "wbinvd"
    | ("smsw" | "smswl")
    | "lmsw"
    | "arpl"
    | "lar"
    | "lsl"
    | "rsm"
    ;

astDefs
    : ASTMacroDef
    | ASTInstruction
    | ASTInstructionPrefix
    | ASTSensitive
    | ASTBasicBlock
    | ASTCommand
    ;


{
    // Global stuff in the cpp file.
}

class AsmLexer extends Lexer;
options {
    k = 3;
    caseSensitive = true;
    testLiterals = false;
    // Specify the vocabulary, to support inversions in a scanner rule.
    charVocabulary = '\u0000'..'\u00FF';
}

{
    // Additional methods and members.
}

// Rules
COMMA	: ',' ;
SEMI	: ';' ;
DOT     : '.' ;
protected COLON   : ':' ;
protected PERCENT : '%' ;
protected AT      : '@' ;

LPAREN		: '(' ;
RPAREN		: ')' ;
LBRACKET	: '[' ;
RBRACKET	: ']' ;
LCURLY		: '{' ;
RCURLY		: '}' ;

PLUS		: '+' ;
MINUS		: '-' ;
DOLLAR		: '$' ;
STAR		: '*' ;
DIV		: '/' ;

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

protected Name   : Letter (Letter | Digit)* ;
String : '"' ( ~('"') )* '"' ;

// Note: For all literals that we wish to lookup in the hash table, there
// must be a Lexer rule that can match it, with the testLiterals option
// enabled.

// Note: We use left factoring for picking out labels amongst the 
// IDs and commands.
ID options {testLiterals=true;}
    : Name (COLON {$setType(Label);})? ;

Int
    : (Digit)+ (COLON {$setType(Label);})? ;

Command options {testLiterals=true;}
    : DOT Name (COLON {$setType(LocalLabel);})? ;

Reg options {testLiterals=true;}
    : PERCENT Name ;

Option options {testLiterals=true;}
    : AT Name ;

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
