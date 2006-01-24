
#include <iostream>
#include <fstream>

#include "AsmLexer.hpp"
#include "AsmParser.hpp"
#include "AsmTreeParser.hpp"
#include <antlr/TokenBuffer.hpp>

int main( int argc, char *argv[] )
{
    ANTLR_USING_NAMESPACE(std);
    ANTLR_USING_NAMESPACE(antlr);
    ANTLR_USING_NAMESPACE(Asm);

    if( argc < 2 )
	exit( 0 );
    try {
        ifstream input( argv[1] );
	AsmLexer lexer(input);
	TokenBuffer buffer(lexer);
	AsmParser parser(buffer);

	ASTFactory ast_factory;
	parser.initializeASTFactory( ast_factory );
	parser.setASTFactory( &ast_factory );

	parser.asmFile();
	RefAST a = parser.getAST();

	AsmTreeParser tree_parser;
	tree_parser.initializeASTFactory( ast_factory );
	tree_parser.setASTFactory( &ast_factory );

	tree_parser.asmFile( a );

	cout << "List:" << endl;
	cout << a->toStringList() << endl;
	cout << "Tree:" << endl;
	cout << a->toStringTree() << endl;
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

