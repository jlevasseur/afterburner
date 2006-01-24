/* tutorial: http://javadude.com/articles/antlrtut/ */

header "pre_include_hpp" {
    // Inserted before antlr generated includes in the header file.
    #include <iostream>
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
asmFile: asmBlocks ;

asmBlocks: (asmAnonymousBlock)? (asmBasicBlock)* ;

asmAnonymousBlock: (asmStatement)+
    { ## = #([ASTBasicBlock, "basic block"], ##); }
    ;

asmBasicBlock: asmLabel (asmStatement)+ 
    { ## = #([ASTBasicBlock, "basic block"], ##); }
    ;

asmEnd: Newline! | SEMI! ;

asmStatement
    : asmInstrPrefix asmEnd
    | asmInstr asmEnd
    | asmCommand asmEnd
    | asmEnd
    ;

asmLabel : Label | LocalLabel ;

asmMacroDefParams
    : (ID (COMMA! ID)*)? 
      { ## = #([ASTMacroDefParams, "macro parameters"], ##); }
    ;

asmMacroDef
    : ".macro"! ID asmMacroDefParams asmEnd!
               asmBlocks
      ".endm"!
      { ## = #([ASTMacroDef, "macro definition"], ##); }
    ;

asmCommand
    : asmMacroDef
    | Command commandParams
      { ## = #([ASTCommand, "command"], ##); }
    ;

asmInstr
    : asmInnocuousInstr instrParams	
      { ## = #([ASTInstruction, "instruction"], ##); }
    | asmSensitive
      { ## = #([ASTSensitive, "sensitive instruction"], ##); }
    ;

commandParams: (commandParam)? (COMMA! commandParam)* ;
commandParam: String | Option | instrParam;

instrParams: (instrParam)? (COMMA! instrParam)* ;
instrParam: regOffsetExpression;

regOffsetExpression
    : expression /*(LPAREN^ ((Int)? COMMA)? asmReg (COMMA (Int)?)? RPAREN)?*/
    /*| LPAREN^ asmReg RPAREN */
    | asmReg
    ;

primitive
    : ID | Int | DOT | Command 
    | LPAREN! expression RPAREN!
    ;

signExpression
    : (PLUS^|MINUS^)? primitive
    ;
makeConstantExpression
    : (DOLLAR^)? signExpression
    ;
multiplyingExpression
    : makeConstantExpression ((STAR^|DIV^|"mod"^) makeConstantExpression)*
    ;
addingExpression
    : multiplyingExpression ((PLUS^|MINUS^) multiplyingExpression)*
    ;

expression : addingExpression ;

asmSensitive {int pad;}
    : pad = asmSensitiveInstr instrParams
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
    : ("lock" | "rep")	{ ## = #([ASTInstructionPrefix, "prefix"], ##); }
    ;

asmSensitiveRegInstr returns [int pad] {pad=0;}
    : ("pop"  | "popl"  | "popd")  (asmSensitiveReg {pad=5;} | instrParam)
    | ("push" | "pushl" | "pushd") (asmSensitiveReg {pad=5;} | instrParam)
    | ("mov"  | "movl" )
        (
	  (instrParam COMMA asmSensitiveReg) => (instrParam COMMA! asmSensitiveReg {pad=12;})
        | (asmSensitiveReg COMMA! instrParam {pad=12;}) 
	| (instrParam COMMA! instrParam)
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
    | ASTMacroDefParams
    | ASTInstruction
    | ASTInstructionPrefix
    | ASTSensitive
    | ASTBasicBlock
    | ASTCommand
    | ASTRegOffset
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
	{ $setType(ANTLR_USE_NAMESPACE(antlr)Token::SKIP); }
    ;

Comment
    : ((CCommentBegin) => CComment | CPPComment)
	{ $setType(ANTLR_USE_NAMESPACE(antlr)Token::SKIP); }
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

{
    // Global stuff in the cpp file.
}

class AsmTreeParser extends TreeParser;
options {
    k = 2;
}
{
    int pad;

    // Additional methods and members.
    void crap( antlr::RefAST a )
        { std::cout << a->getText(); }
}

asmFile: asmBlocks;

asmBlocks: (asmBlock)* ;

asmBlock: #( ASTBasicBlock (asmLabel)?  (asmStatementLine)* );

asmLabel
    : l:Label 		{ std::cout << l->getText() << std::endl; }
    | ll:LocalLabel 	{ std::cout << ll->getText() << std::endl; }
    ;

asmStatementLine: asmStatement { std::cout << std::endl; } ;

asmStatement
    : asmInstrPrefix
    | asmInstr
    | asmCommand
    ;

asmCommand
    : asmMacroDef
    | #(ASTCommand c:Command	{ std::cout << '\t' << c->getText(); }
        (commandParams)?
       )
    ;

asmMacroDef
    : #( ASTMacroDef i:ID	{ std::cout << ".macro " << i->getText(); }
         asmMacroDefParams	{ std::cout << std::endl; }
	 asmBlocks		{ std::cout << ".endm" << std::endl; }
       )
    ;
asmMacroDefParams
    : #( ASTMacroDefParams (
            (i:ID		{ std::cout << ' ' << i->getText();   } )
            (ii:ID		{ std::cout << ", " << ii->getText(); } )*
        )? ) 
    ;

asmInstrPrefix
    : #( ASMInstructionPrefix i:ID	{std::cout << i->getText();}
       );

asmInstr {antlr::RefAST r;}
    : #( ASTInstruction i:ID	{std::cout << '\t' << i->getText();}
         (instrParams)? )
    | #( ASTSensitive r=asmSensitiveInstr  
                        {if( r ) std::cout << '\t' << r->getText();}
         (instrParams)? )
    ;

commandParams
    : { std::cout << '\t'; } commandParam
      ({ std::cout << ',' << ' '; } commandParam)*
    ;
commandParam
    : s:String		{ crap(s); }
    | o:Option 		{ crap(o); }
    | instrParam
    ;

instrParams
    : { std::cout << '\t'; } instrParam 
      ({ std::cout << ',' << ' '; } instrParam)*
    ;
instrParam: regOffsetExpression;

regOffsetExpression
    : expr
    | asmReg
    ;

primitive
    : i:ID 		{ crap(i); }
    | n:Int 		{ crap(n); }
    | d:DOT 		{ crap(d); }
    | c:Command		{ crap(c); }
    ;

expr
    : { std::cout << '('; }
    ( #(p:PLUS   ({ crap(p); } expr)+)
    | #(m:MINUS  ({ crap(m); } expr)+)
    | #(s:STAR   expr ({ crap(s); } expr)+)
    | #(d:DIV    expr ({ crap(d); } expr)+)
    | #(r:"mod"  expr ({ crap(r); } expr)+)
    | #(D:DOLLAR { crap(D); } expr)
    )
      {std::cout << ')'; }
    | primitive
    ;

asmLowReg returns [antlr::RefAST r] { r=NULL; }
    : {r=_t;} "%al"
    | {r=_t;} "%bl"
    | {r=_t;} "%cl"
    | {r=_t;} "%dl"
    ;
asmHighReg returns [antlr::RefAST r] { r=NULL; }
    : {r=_t;} "%ah"
    | {r=_t;} "%bh"
    | {r=_t;} "%ch"
    | {r=_t;} "%dh"
    ;
asmNormalReg returns [antlr::RefAST r] { r=NULL; }
    : {r=_t;} "%eax"
    | {r=_t;} "%ebx"
    | {r=_t;} "%ecx"
    | {r=_t;} "%edx"
    | {r=_t;} "%esi"
    | {r=_t;} "%edi"
    | {r=_t;} "%esp"
    | {r=_t;} "%ebp"
    ;

asmReg { antlr::RefAST n; }
    : n=asmNormalReg	{ if( n ) std::cout << n->getText(); }
    | n=asmLowReg	{ if( n ) std::cout << n->getText(); }
    | n=asmHighReg	{ if( n ) std::cout << n->getText(); }
    | n=asmSensitiveReg	{ if( n ) std::cout << n->getText(); }
    ;

asmSensitiveReg returns [antlr::RefAST r] { r=NULL; }
    : {r=_t;} "%cs"
    | {r=_t;} "%ds"
    | {r=_t;} "%es"
    | {r=_t;} "%fs"
    | {r=_t;} "%gs"
    | {r=_t;} "%cr0"
    | {r=_t;} "%cr2"
    | {r=_t;} "%cr3"
    | {r=_t;} "%cr4"
    | {r=_t;} "%db0"
    | {r=_t;} "%db1"
    | {r=_t;} "%db2"
    | {r=_t;} "%db3"
    | {r=_t;} "%db4"
    | {r=_t;} "%db5"
    | {r=_t;} "%db6"
    | {r=_t;} "%db7"
    ;

asmSensitiveInstr returns [antlr::RefAST r] { pad=8; r=NULL; }
    : ({r=_t;} "popf"  | {r=_t;} "popfl"  | {r=_t;} "popfd")	{pad=21;}
    | ({r=_t;} "pushf" | {r=_t;} "pushfl" | {r=_t;} "pushfd")	{pad=5;}
    | ({r=_t;} "lgdt" | {r=_t;} "lgdtl")			{pad=9;}
    | ({r=_t;} "sgdt" | {r=_t;} "sgdtl")
    | ({r=_t;} "lidt" | {r=_t;} "lidtl")			{pad=9;}
    | ({r=_t;} "sidt" | {r=_t;} "sidtl")
    |  {r=_t;} "ljmp"						{pad=9;}
    | ({r=_t;} "lds" | {r=_t;} "les" | {r=_t;} "lfs")		{pad=16;} 
    | ({r=_t;} "lgs" | {r=_t;} "lss")				{pad=16;}
    |  {r=_t;} "clts"						{pad=14;}
    |  {r=_t;} "hlt"						{pad=6;}
    |  {r=_t;} "cli"						{pad=7;}
    |  {r=_t;} "sti"						{pad=23;}
    |  {r=_t;} "lldt"						{pad=16;}
    | ({r=_t;} "sldt" | {r=_t;} "sldtl")			{pad=6;}
    |  {r=_t;} "ltr"						{pad=16;}
    | ({r=_t;} "str" | {r=_t;} "strl")				{pad=9;}
    | ({r=_t;} "inb"  | {r=_t;} "inw"  | {r=_t;} "inl")		{pad=13;}
    | ({r=_t;} "outb" | {r=_t;} "outw" | {r=_t;} "outl" )	{pad=16;}
    |  {r=_t;} "invlpg"						{pad=6;}
    | ({r=_t;} "iret" | {r=_t;} "iretl" | {r=_t;} "iretd")	{pad=4;}
    |  {r=_t;} "lret"						{pad=4;}
    |  {r=_t;} "cpuid"						{pad=6;}
    |  {r=_t;} "wrmsr"
    |  {r=_t;} "rdmsr"
    |  {r=_t;} "int"						{pad=11;}
    |  {r=_t;} "ud2"
    |  {r=_t;} "invd"
    |  {r=_t;} "wbinvd"
    | ({r=_t;} "smsw" | {r=_t;} "smswl")
    |  {r=_t;} "lmsw"
    |  {r=_t;} "arpl"
    |  {r=_t;} "lar"
    |  {r=_t;} "lsl"
    |  {r=_t;} "rsm"
    | ({r=_t;} "pop"  | {r=_t;} "popl"  | {r=_t;} "popd")	{pad=5;}
    | ({r=_t;} "push" | {r=_t;} "pushl" | {r=_t;} "pushd")	{pad=5;}
    | ({r=_t;} "mov"  | {r=_t;} "movl" )			{pad=12;}
    ;

