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
    : ID | Int | Hex | Command | asmReg | Reg | RelativeLocation
    | (asmFpReg LPAREN) => asmFpReg LPAREN! Int RPAREN! 
          { ## = #([ASTRegisterIndex, "register index"], ##);}
    | asmFpReg
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
    : asmSensitiveInstr (instrParams)? { sensitive=true; }
    | sensitive=asmSensitiveRegInstr
    ;

asmReg
    : ( "%al" | "%bl" | "%cl" | "%dl"
      | "%ah" | "%bh" | "%ch" | "%dh"
      | "%ax" | "%bx" | "%cx" | "%dx" | "%si" | "%di" | "%sp" | "%bp"
      | "%eax" | "%ebx" | "%ecx" | "%edx" | "%esi" | "%edi" | "%esp" | "%ebp"
      )
        { ##->setType(ASTRegister); }
    ;
asmFpReg : "%st" { ##->setType(ASTRegister); };
asmSegReg
    : ("%cs" | "%ds" | "%es" | "%fs" | "%gs")
      { ##->setType(ASTRegister); }
    ;
asmSensitiveReg
    : ("%cr0" | "%cr2" | "%cr3" | "%cr4" 
      | "%db0" | "%db1" | "%db2" | "%db3" | "%db4" | "%db5" | "%db6" | "%db7"
      )
      { ##->setType(ASTRegister); }
    ;

asmInstrPrefix
    : ("lock" | "rep")	{ ## = #([ASTInstructionPrefix, "prefix"], ##); }
    ;

asmInnocuousInstr: ID;

asmSensitiveRegInstr returns [bool sensitive] {sensitive=false;}
    : ia32_pop  (asmSensitiveReg {sensitive=true;} | instrParam)
    | ia32_push (asmSensitiveReg {sensitive=true;} | instrParam)
    | ia32_mov
        (
	  (instrParam COMMA asmSensitiveReg) => 
	    (instrParam COMMA! asmSensitiveReg)		{sensitive=true;}
        | (asmSensitiveReg COMMA! instrParam)		{sensitive=true;}
	| (instrParam COMMA! instrParam)
	)
    ;

// We want instruction tokens, but via the string hash table, so 
// build the tokens manually.  Recall that tokens start with a capital letter.
ia32_popf : ("popf"  | "popfl"  | "popfd")	{ ##->setType(IA32_popf); } ;
ia32_pushf: ("pushf" | "pushfl" | "pushfd")	{ ##->setType(IA32_pushf); } ;
ia32_lgdt : ("lgdt"  | "lgdtl"  | "lgdtd")	{ ##->setType(IA32_lgdt); } ;
ia32_sgdt : ("sgdt"  | "sgdtl"  | "sgdtd")	{ ##->setType(IA32_sgdt); } ;
ia32_lidt : ("lidt"  | "lidtl"  | "lidtd")	{ ##->setType(IA32_lidt); } ;
ia32_sidt : ("sidt"  | "sidtl"  | "sidtd")	{ ##->setType(IA32_sidt); } ;
ia32_ljmp : ("ljmp")				{ ##->setType(IA32_ljmp); } ;
ia32_lds  : ("lds")				{ ##->setType(IA32_lds); } ;
ia32_les  : ("les")				{ ##->setType(IA32_les); } ;
ia32_lfs  : ("lfs")				{ ##->setType(IA32_lfs); } ;
ia32_lgs  : ("lgs")				{ ##->setType(IA32_lgs); } ;
ia32_lss  : ("lss")				{ ##->setType(IA32_lss); } ;
ia32_clts : ("clts")				{ ##->setType(IA32_clts); } ;
ia32_hlt  : ("hlt")				{ ##->setType(IA32_hlt); } ;
ia32_cli  : ("cli")				{ ##->setType(IA32_cli); } ;
ia32_sti  : ("sti")				{ ##->setType(IA32_sti); } ;
ia32_lldt : ("lldt" | "lldtl" | "lldtd")	{ ##->setType(IA32_lldt); } ;
ia32_sldt : ("sldt" | "sldtl" | "sldtd" )	{ ##->setType(IA32_sldt); } ;
ia32_ltr  : ("ltr"  | "ltrl" | "ltrd" )		{ ##->setType(IA32_ltr); } ;
ia32_str  : ("str"  | "strl" | "strd" )		{ ##->setType(IA32_str); } ;
ia32_in   : ("inb"  | "inw"  | "inl" )		{ ##->setType(IA32_in); } ;
ia32_out  : ("outb" | "outw" | "outl" )		{ ##->setType(IA32_out); } ;
ia32_invlpg : ("invlpg")			{ ##->setType(IA32_invlpg); } ;
ia32_iret : ("iret" | "iretl" | "iretd")	{ ##->setType(IA32_iret); } ;
ia32_lret : ("lret")				{ ##->setType(IA32_lret); } ;
ia32_cpuid : ("cpuid")				{ ##->setType(IA32_cpuid); } ;
ia32_wrmsr : ("wrmsr")				{ ##->setType(IA32_wrmsr); } ;
ia32_rdmsr : ("rdmsr")				{ ##->setType(IA32_rdmsr); } ;
ia32_int   : ("int")				{ ##->setType(IA32_int); } ;
ia32_ud2   : ("ud2")				{ ##->setType(IA32_ud2); } ;
ia32_invd  : ("invd")				{ ##->setType(IA32_invd); } ;
ia32_wbinvd : ("wbinvd")			{ ##->setType(IA32_wbinvd); } ;
ia32_smsw  : ("smsw" | "smswl" | "smswd")	{ ##->setType(IA32_smsw); } ;
ia32_lmsw  : ("lmsw" | "lmswl" | "lmswd")	{ ##->setType(IA32_lmsw); } ;
ia32_arpl  : ("arpl")				{ ##->setType(IA32_arpl); } ;
ia32_lar   : ("lar")				{ ##->setType(IA32_lar); } ;
ia32_lsl   : ("lsl")				{ ##->setType(IA32_lsl); } ;
ia32_rsm   : ("rsm")				{ ##->setType(IA32_rsm); } ;
ia32_pop   : ("pop"  | "popl"  | "popd")	{ ##->setType(IA32_pop); } ;
ia32_push  : ("push" | "pushl" | "pushd")	{ ##->setType(IA32_push); } ;
ia32_mov   : ("mov"  | "movl"  | "movd")	{ ##->setType(IA32_mov); } ;

asmSensitiveInstr 
    : ia32_popf | ia32_pushf | ia32_lgdt | ia32_sgdt | ia32_lidt | ia32_sidt
    | ia32_ljmp | ia32_lds | ia32_les | ia32_lfs | ia32_lgs | ia32_lss
    | ia32_clts | ia32_hlt | ia32_cli | ia32_sti | ia32_lldt | ia32_sldt
    | ia32_ltr  | ia32_str | ia32_in  | ia32_out
    | ia32_invlpg | ia32_iret | ia32_lret | ia32_cpuid 
    | ia32_wrmsr | ia32_rdmsr | ia32_int | ia32_ud2 | ia32_invd | ia32_wbinvd
    | ia32_smsw | ia32_lmsw | ia32_arpl | ia32_lar | ia32_lsl | ia32_rsm
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
    | ASTRegisterIndex
    | ASTDereference
    | ASTSegment
    | ASTRegister
    | ASTNegative
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
COLON   : ':' ;
protected DOT     : '.' ;
protected PERCENT : '%' ;
protected AT      : '@' ;
protected HASH	  : '#' ;

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

Preprocessor
    // #APP and #NO_APP surround inlined assembler.
    : h:HASH { (h->getColumn() == 1) }? (~('\n'))*
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

protected Name   : Letter (Letter | Digit | DOT)* ;
String : '"' ( StringEscape | ~('\\'|'"') )* '"' ;
protected StringEscape : '\\' . ;

// Note: For all literals that we wish to lookup in the hash table, there
// must be a Lexer rule that can match it, with the testLiterals option
// enabled.

// Note: We use left factoring for picking out labels amongst the 
// IDs and commands.
ID options {testLiterals=true;}
    : Name (COLON {$setType(Label);})? ;

Int : (Digit)+ (COLON {$setType(Label);})? 
      ('f'|'b' {$setType(RelativeLocation);})?
    ;
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
    | #( ASTSensitive r=asmSensitiveInstr  { startSensitive(r); }
         (instrParams)? { endSensitive(r); } )
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
      (ASTDefaultParam | r1:ASTRegister {crap(r1);}) 
      ({ std::cout << ','; } (ASTDefaultParam | r2:ASTRegister {crap(r2);}) 
       ({ std::cout << ','; } (i:Int {crap(i);} | ASTDefaultParam))?
      )? 
      { std::cout << ')'; }
    ;

primitive
    : i:ID 		{ crap(i); }
    | n:Int 		{ crap(n); }
    | h:Hex		{ crap(h); }
    | c:Command		{ crap(c); }
    | r:ASTRegister	{ crap(r); }
    | rl:RelativeLocation { crap(rl); }
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
    | #(m2:"mod"  subexpr ({ crap(m2); } subexpr)+)
    | #(D:DOLLAR { crap(D); } subexpr)
    | primitive
    | #(ASTDereference { std::cout << '*'; } expr)
    | #(ASTSegment r:ASTRegister { std::cout << r->getText() << ':'; } expr)
    | #(ASTRegisterDisplacement subexpr expr)
    | #(ASTRegisterBaseIndexScale regOffsetBase)
    | #(ASTRegisterIndex ri:ASTRegister in:Int 
             { std::cout << ri->getText() << '(' << in->getText() << ')'; } )
    ;

asmSensitiveInstr returns [antlr::RefAST r] { pad=8; r=NULL; }
    : popf:IA32_popf	{r=popf;  pad=21;}
    | pushf:IA32_pushf	{r=pushf; pad=5;}
    | lgdt:IA32_lgdt	{r=lgdt;  pad=9;}
    | sgdt:IA32_sgdt	{r=sgdt; }
    | lidt:IA32_lidt	{r=lidt;  pad=9;}
    | sidt:IA32_sidt	{r=sidt; }
    | ljmp:IA32_ljmp	{r=ljmp;  pad=9;}
    | lds:IA32_lds	{r=lds;   pad=16;}
    | les:IA32_les	{r=les;   pad=16;}
    | lfs:IA32_lfs	{r=lfs;   pad=16;}
    | lgs:IA32_lgs	{r=lgs;   pad=16;}
    | lss:IA32_lss	{r=lss;   pad=16;}
    | clts:IA32_clts	{r=clts;  pad=14;}
    | hlt:IA32_hlt	{r=hlt;   pad=6;}
    | cli:IA32_cli	{r=cli;   pad=7;}
    | sti:IA32_sti	{r=sti;   pad=23;}
    | lldt:IA32_lldt	{r=lldt;  pad=16;}
    | sldt:IA32_sldt	{r=sldt;  pad=6;}
    | ltr:IA32_ltr	{r=ltr;   pad=16;}
    | str:IA32_str	{r=str;   pad=9;}
    | in:IA32_in	{r=in;    pad=13;}
    | out:IA32_out	{r=out;   pad=16;}
    | invlpg:IA32_invlpg {r=invlpg; pad=6;}
    | iret:IA32_iret	{r=iret;  pad=4;}
    | lret:IA32_lret	{r=lret;  pad=4;}
    | cpuid:IA32_cpuid	{r=cpuid; pad=6;}
    | wrmsr:IA32_wrmsr	{r=wrmsr;}
    | rdmsr:IA32_rdmsr	{r=rdmsr;}
    | ia32_int:IA32_int	{r=ia32_int;}
    | ud2:IA32_ud2	{r=ud2;}
    | invd:IA32_invd	{r=invd;}
    | wbinvd:IA32_wbinvd {r=wbinvd;}
    | smsw:IA32_smsw	{r=smsw;}
    | lmsw:IA32_lmsw	{r=lmsw;}
    | arpl:IA32_arpl	{r=arpl;}
    | lar:IA32_lar	{r=lar;}
    | lsl:IA32_lsl	{r=lsl;}
    | rsm:IA32_rsm	{r=rsm;}
    | pop:IA32_pop	{r=pop; pad=5;}
    | push:IA32_push	{r=push; pad=5;}
    | mov:IA32_mov	{r=mov; pad=12;}
    ;

