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
    | Command commandParams	{ ## = #([ASTCommand, "command"], ##); }
      // Set the AST type to Command, to simplify the tree walker.
    | f:".file" simpleParams	{ #f->setType(Command); 
    				  ## = #([ASTCommand, "command"], ##); }
    | l:".loc" simpleParams	{ #l->setType(Command);
    				  ## = #([ASTCommand, "command"], ##); }
    ;

asmInstr { bool sensitive=false; }
    : ( asmInnocuousInstr (instrParams)?  | sensitive = asmMaybeSensitive)
      {
        if( sensitive )
	    ## = #([ASTSensitive, "sensitive instruction"], ##);
        else
	    ## = #([ASTInstruction, "instruction"], ##);
      }
    ;

simpleParams: (simpleParam)* ;
simpleParam: String | Option | Int | Hex | Command;

commandParams
    // Some commands have optional parameters anywhere in the parameter
    // list, and use the commas to position the specified parameters.
    // Example: .p2align 4,,15
    : commandParam (COMMA! (commandParam | defaultParam) )* 
    | defaultParam (COMMA! (commandParam | defaultParam) )+
    | ! /* nothing */
    ;
defaultParam: /* nothing */ { ## = #[ASTDefaultParam, "default"]; } ;
commandParam: String | Option | instrParam;

instrParams: instrParam (COMMA! instrParam)* ;
instrParam : regExpression ;

regExpression: regDereferenceExpr ;

regDereferenceExpr
    : (s:STAR^			{#s->setType(ASTDereference);} )?
      regSegmentExpr
    ;
regSegmentExpr
    : (asmSegReg c:COLON^	{#c->setType(ASTSegment);} )?
      regDisplacementExpr
    ;

regDisplacementExpr
    // section:disp(base, index, scale)  
    // where section, base, and index are registers.
    : expression 
      (regOffsetBase
          {## = #([ASTRegisterDisplacement, "register displacement"], ##);}
      )?
    ;

regOffsetBase
    : (  LPAREN! defaultParam 
               COMMA!  (asmReg | defaultParam) 
	       COMMA!  (Int | defaultParam) RPAREN!
       | LPAREN! asmReg 
               (COMMA! (asmReg | defaultParam) 
	       (COMMA! (Int | defaultParam))? )? RPAREN!
       ) {## = #([ASTRegisterBaseIndexScale, "register base index scale"], ##);}
    ;

primitive
    : ID | Int | Hex | Command | asmReg
    | (LPAREN (asmReg|COMMA)) => regOffsetBase
    | LPAREN! expression RPAREN!
    ;

signExpression
    : (m:MINUS^ {#m->setType(ASTNegative);})? primitive
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

asmMaybeSensitive returns [bool sensitive] { sensitive=false; }
    : asmSensitiveInstr  { sensitive=true; }
    | sensitive=asmSensitiveRegInstr
    ;

asmLowReg8: "%al" | "%bl" | "%cl" | "%dl";
asmHighReg8: "%ah" | "%bh" | "%ch" | "%dh";
asmReg16: "%ax" | "%bx" | "%cx" | "%dx" | "%si" | "%di" | "%sp" | "%bp";
asmReg32: "%eax" | "%ebx" | "%ecx" | "%edx" | "%esi" | "%edi" | "%esp" | "%ebp";
asmReg
    : asmReg32
    | asmReg16
    | asmLowReg8
    | asmHighReg8
    ;
asmSegReg
    : "%cs" | "%ds" | "%es" | "%fs" | "%gs" ;
asmSensitiveReg
    : "%cr0" | "%cr2" | "%cr3" | "%cr4" 
    | "%db0" | "%db1" | "%db2" | "%db3" | "%db4" | "%db5" | "%db6" | "%db7"
    ;

asmInstrPrefix
    : ("lock" | "rep")	{ ## = #([ASTInstructionPrefix, "prefix"], ##); }
    ;

asmInnocuousInstr: ID;

asmSensitiveRegInstr returns [bool sensitive] {sensitive=false;}
    : ("pop"  | "popl"  | "popd")  
        (asmSensitiveReg {sensitive=true;} | instrParam)
    | ("push" | "pushl" | "pushd")
        (asmSensitiveReg {sensitive=true;} | instrParam)
    | ("mov"  | "movl"  | "movd")
        (
	  (instrParam COMMA asmSensitiveReg) => 
	    (instrParam COMMA! asmSensitiveReg)		{sensitive=true;}
        | (asmSensitiveReg COMMA! instrParam)		{sensitive=true;}
	| (instrParam COMMA! instrParam)
	)
    ;

asmSensitiveInstr 
    : "popf"  | "popfl"  | "popfd"
    | "pushf" | "pushfl" | "pushfd"
    | "lgdt"  | "lgdtl"  | "lgdtd"
    | "sgdt"  | "sgdtl"  | "sgdtd"
    | "lidt"  | "lidtl"  | "lidtd"
    | "sidt"  | "sidtl"  | "sidtd"
    | "ljmp"
    | "lds" | "les" | "lfs" | "lgs" | "lss"
    | "clts"
    | "hlt"
    | "cli"
    | "sti"
    | "lldt"
    | "sldt" | "sldtl" | "sldtd"
    | "ltr"
    | "str"  | "strl" | "strd"
    | "inb"  | "inw"  | "inl"
    | "outb" | "outw" | "outl" 
    | "invlpg"
    | "iret" | "iretl" | "iretd"
    | "lret"
    | "cpuid"
    | "wrmsr"
    | "rdmsr"
    | "int"
    | "ud2"
    | "invd"
    | "wbinvd"
    | "smsw" | "smswl" | "smswd"
    | "lmsw" | "lmswl" | "lmswd"
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
    | ASTDefaultParam
    | ASTRegisterDisplacement
    | ASTRegisterBaseIndexScale
    | ASTDereference
    | ASTSegment
    | ASTNegative
    ;


{
    // Global stuff in the cpp file.
}

class AsmLexer extends Lexer;
options {
    k = 5;
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
COLON   : ':' ;
protected DOT     : '.' ;
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
String : '"' ( StringEscape | ~('\\'|'"') )* '"' ;
protected StringEscape : '\\' . ;

// Note: For all literals that we wish to lookup in the hash table, there
// must be a Lexer rule that can match it, with the testLiterals option
// enabled.

// Note: We use left factoring for picking out labels amongst the 
// IDs and commands.
ID options {testLiterals=true;}
    : Name (COLON {$setType(Label);})? ;

Int : (Digit)+ (COLON {$setType(Label);})? ;
Hex : '0' 'x' (Digit | 'a'..'f' | 'A'..'F')+ (COLON {$setType(Label);})? ;

Command options {testLiterals=true;}
    : DOT (Letter | Digit | DOT)* (COLON {$setType(LocalLabel);})? ;

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
protected:
    unsigned pad;
    unsigned label_cnt;
    bool     in_macro;

public:
    void init()
    {
        // We need a class init method because C++ constructors are useless.
        pad = 0;
	in_macro = false;
	label_cnt = 0;
    }

protected:
    void crap( antlr::RefAST a )
        { std::cout << a->getText(); }
    void ch( char ch )
        { std::cout << ch; }

    void startSensitive( antlr::RefAST s )
    {
        // Add a label for the sensitive instruction block.
        if( this->in_macro )
	    std::cout << "9997:" << std::endl;
	else
            std::cout << ".L_sensitive_" << this->label_cnt << ":" << std::endl;
	// Print the sensitive instruction.
	std::cout << '\t' << s->getText();
    }

    void endSensitive( antlr::RefAST s )
    {
        unsigned begin = this->label_cnt;
        unsigned end   = this->label_cnt+1;
        this->label_cnt += 2;

        // Add a newline after the instruction's parameters.
        std::cout << std::endl;
        // Add the NOP padding.
        for( unsigned i = 0; i < pad; i++ )
            std::cout << '\t' << "nop" << std::endl;

        // Add the label following the NOP block.
        if( this->in_macro )
            std::cout << "9998:" << std::endl;
	else
            std::cout << ".L_sensitive_" << end << ":" << std::endl;

        // Record the instruction location.
        std::cout << "\t.pushsection\t.afterburn" << std::endl;
        std::cout << "\t.balign\t4" << std::endl;
        if( this->in_macro ) {
            std::cout << "\t.long\t9997b" << std::endl;
            std::cout << "\t.long\t9998b" << std::endl;
        } else {
            std::cout << "\t.long\t.L_sensitive_" << begin << std::endl;
            std::cout << "\t.long\t.L_sensitive_" << end << std::endl;
        }
        std::cout << "\t.popsection" << std::endl;
    }
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
	 { this->in_macro=true; } 
	 asmBlocks 		
	 { this->in_macro=false; }
	 			{ std::cout << ".endm" << std::endl; }
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
    : #( ASTInstruction i:. {std::cout << '\t' << i->getText();}
         (instrParams)? )
    | #( ASTSensitive r=asmSensitiveInstr  {if(r) startSensitive(r); }
         (instrParams)? { if(r) endSensitive(r); } )
    ;

commandParams
    : { std::cout << '\t'; } commandParam
      ({ std::cout << ',' << ' '; } commandParam)*
    ;
commandParam
    : s:String		{ crap(s); }
    | o:Option 		{ crap(o); }
    | ASTDefaultParam
    | instrParam
    ;

instrParams
    : { std::cout << '\t'; } instrParam 
      ({ std::cout << ',' << ' '; } instrParam)*
    ;
instrParam
    : regExpression
    ;

regExpression
    // section:disp(base, index, scale)  
    // where section, base, and index are registers.
    : expr
    ;

regOffsetBase
    : { std::cout << '('; }
      (ASTDefaultParam | asmReg) 
      ({ std::cout << ','; } (asmReg | ASTDefaultParam) 
       ({ std::cout << ','; } (i:Int {crap(i);} | ASTDefaultParam))?
      )? 
      { std::cout << ')'; }
    ;

primitive
    : i:ID 		{ crap(i); }
    | n:Int 		{ crap(n); }
    | h:Hex		{ crap(h); }
    | c:Command		{ crap(c); }
    | asmReg
    ;

subexpr
    : (primitive) => expr
    | { ch('('); }    expr    { ch(')'); }
    ;

expr { antlr::RefAST sr; }
    : #(p:PLUS  expr ({ crap(p); } expr)+)
    | #(m:MINUS expr ({ crap(m); } expr)+)
    | #(ASTNegative { ch('-'); } subexpr)
    | #(s:STAR   subexpr ({ crap(s); } subexpr)+)
    | #(d:DIV    subexpr ({ crap(d); } subexpr)+)
    | #(r:"mod"  subexpr ({ crap(r); } subexpr)+)
    | #(D:DOLLAR { crap(D); } subexpr)
    | primitive
    | #(ASTDereference { std::cout << '*'; } expr)
    | #(ASTSegment sr=asmSegReg { if(sr) std::cout << sr->getText() << ':'; }
        expr)
    | #(ASTRegisterDisplacement subexpr expr)
    | #(ASTRegisterBaseIndexScale regOffsetBase)
    ;

asmLowReg8 returns [antlr::RefAST r] { r=NULL; }
    : {r=_t;} "%al"
    | {r=_t;} "%bl"
    | {r=_t;} "%cl"
    | {r=_t;} "%dl"
    ;
asmHighReg8 returns [antlr::RefAST r] { r=NULL; }
    : {r=_t;} "%ah"
    | {r=_t;} "%bh"
    | {r=_t;} "%ch"
    | {r=_t;} "%dh"
    ;
asmReg16 returns [antlr::RefAST r] { r=NULL; }
    : {r=_t;} "%ax" 
    | {r=_t;} "%bx" 
    | {r=_t;} "%cx" 
    | {r=_t;} "%dx" 
    | {r=_t;} "%si" 
    | {r=_t;} "%di" 
    | {r=_t;} "%sp" 
    | {r=_t;} "%bp"
    ;
asmReg32 returns [antlr::RefAST r] { r=NULL; }
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
    : n=asmReg32	{ if( n ) std::cout << n->getText(); }
    | n=asmReg16	{ if( n ) std::cout << n->getText(); }
    | n=asmLowReg8	{ if( n ) std::cout << n->getText(); }
    | n=asmHighReg8	{ if( n ) std::cout << n->getText(); }
    | n=asmSensitiveReg	{ if( n ) std::cout << n->getText(); }
    | n=asmSegReg	{ if( n ) std::cout << n->getText(); }
    ;

asmSegReg returns [antlr::RefAST r] { r=NULL; }
    : {r=_t;} "%cs"
    | {r=_t;} "%ds"
    | {r=_t;} "%es"
    | {r=_t;} "%fs"
    | {r=_t;} "%gs"
    ;

asmSensitiveReg returns [antlr::RefAST r] { r=NULL; }
    : {r=_t;} "%cr0"
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
    | ({r=_t;} "lgdt" | {r=_t;} "lgdtl" | {r=_t;} "lgdtd")	{pad=9;}
    | ({r=_t;} "sgdt" | {r=_t;} "sgdtl" | {r=_t;} "sgdtd")
    | ({r=_t;} "lidt" | {r=_t;} "lidtl" | {r=_t;} "lidtd")	{pad=9;}
    | ({r=_t;} "sidt" | {r=_t;} "sidtl" | {r=_t;} "sidtd")
    |  {r=_t;} "ljmp"						{pad=9;}
    | ({r=_t;} "lds" | {r=_t;} "les" | {r=_t;} "lfs")		{pad=16;} 
    | ({r=_t;} "lgs" | {r=_t;} "lss")				{pad=16;}
    |  {r=_t;} "clts"						{pad=14;}
    |  {r=_t;} "hlt"						{pad=6;}
    |  {r=_t;} "cli"						{pad=7;}
    |  {r=_t;} "sti"						{pad=23;}
    |  {r=_t;} "lldt"						{pad=16;}
    | ({r=_t;} "sldt" | {r=_t;} "sldtl" | {r=_t;} "sldtd")	{pad=6;}
    |  {r=_t;} "ltr"						{pad=16;}
    | ({r=_t;} "str"  | {r=_t;} "strl" | {r=_t;} "strd")	{pad=9;}
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
    | ({r=_t;} "smsw" | {r=_t;} "smswl" | {r=_t;} "smswd")
    |  {r=_t;} "lmsw"
    |  {r=_t;} "arpl"
    |  {r=_t;} "lar"
    |  {r=_t;} "lsl"
    |  {r=_t;} "rsm"
    | ({r=_t;} "pop"  | {r=_t;} "popl"  | {r=_t;} "popd")	{pad=5;}
    | ({r=_t;} "push" | {r=_t;} "pushl" | {r=_t;} "pushd")	{pad=5;}
    | ({r=_t;} "mov"  | {r=_t;} "movl" )			{pad=12;}
    ;

