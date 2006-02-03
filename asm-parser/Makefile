##  This Makefile builds the afterburner assembler parser.
##
##  Build parameters of interest to the user:
##    O = The output directory for object files and binaries.
##    rebuild-antlr : Define this parameter to expose the make rules that
##        process antlr grammars.
##

O ?= .

antlr_cflags ?= $(shell antlr-config --cflags)
antlr_libs   ?= $(shell antlr-config --libs)

all: $(O)/afterburner

CXXFLAGS  += -Wall -O2 $(antlr_cflags) -I.
LIBS    += $(antlr_libs)

$(O)/afterburner: $(O)/afterburner.o \
                  $(O)/AsmLexer.o $(O)/AsmParser.o $(O)/AsmTreeParser.o
	g++ $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

$(O)/afterburner.o: AsmLexer.hpp AsmParser.hpp AsmTreeParser.hpp afterburner.cpp
$(O)/AsmParser.o: AsmParser.hpp AsmParser.cpp
$(O)/AsmLexer.o: AsmLexer.hpp AsmLexer.cpp
$(O)/AsmTreeParser.o: AsmTreeParser.hpp AsmTreeParser.cpp

$(O)/%.o: %.cpp
	g++ $(CXXFLAGS) -c -o $@ $<

clean:
	-rm -f $(O)/*.o $(O)/afterburner

######################################################################
##
##  Process the antlr grammars, but only if rebuild-antlr=1
##
######################################################################

ifdef rebuild-antlr

.PHONY: compile-grammar
compile-grammar: AsmParser.cpp

AsmTreeParser.cpp AsmTreeParser.hpp: Asm.g
AsmParser.cpp AsmParser.hpp: Asm.g
AsmLexer.hpp AsmLexer.cpp: Asm.g

%Lexer.hpp %Lexer.cpp \
%Parser.cpp %Parser.hpp \
%TreeParser.cpp %TreeParser.hpp \
%ParserTokenTypes.hpp %ParserTokenTypes.txt: %.g
	antlr $<

endif

clean-antlr:
	-rm -f *.class
	-rm -f *TokenTypes.txt *TokenTypes.hpp
	-rm -f *Parser.hpp *Parser.cpp *Lexer.hpp *Lexer.cpp

