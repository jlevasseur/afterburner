
#include <iostream>

#include "AsmLexer.hpp"
#include "AsmParser.hpp"
#include <antlr/TokenBuffer.hpp>

int main( int, char** )
{
    ANTLR_USING_NAMESPACE(std);
    ANTLR_USING_NAMESPACE(antlr);
    ANTLR_USING_NAMESPACE(Asm);

    try {
	AsmLexer lexer(cin);
	TokenBuffer buffer(lexer);
	AsmParser parser(buffer);
	parser.asmFile();
    }
    catch( ANTLRException& e )
    {
	cerr << "exception: " << e.getMessage() << endl;
	return -1;
    }
    catch( exception& e )
    {
	cerr << "exception: " << e.what() << endl;
	return -1;
    }
    return 0;
}

