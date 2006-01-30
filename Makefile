
O ?= .

antlr_cflags ?= $(shell antlr-config --cflags)
antlr_libs   ?= $(shell antlr-config --libs)

all: $(O)/afterburner

CPPFLAGS  += -Wall -O2 $(antlr_cflags)
LIBS    += $(antlr_libs)

$(O)/afterburner: $(O)/afterburner.o \
                  $(O)/AsmLexer.o $(O)/AsmParser.o $(O)/AsmTreeParser.o
	g++ $(CPPFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

$(O)/afterburner.o: AsmLexer.hpp AsmParser.hpp AsmTreeParser.hpp afterburner.cpp
$(O)/AsmParser.o: AsmParser.hpp AsmParser.cpp
$(O)/AsmLexer.o: AsmLexer.hpp AsmLexer.cpp
$(O)/AsmTreeParser.o: AsmTreeParser.hpp AsmTreeParser.cpp

clean:
	-rm -f $(O)/*.o $(O)/afterburner $(O)/*.s

######################################################################
##
##  Test the afterburner
##
######################################################################

$(O)/%.s: %.cpp
	g++ $(CPPFLAGS) -S $< -o $@
$(O)/%.afterburnt.s: $(O)/%.s $(O)/afterburner
	$(O)/afterburner $< > $@
$(O)/%.o.s: $(O)/%.o
	objdump -d $< > $@

## Compare a normal binary to an afterburnt binary 
.PHONY: test
test: $(O)/afterburner $(O)/AsmParser.s $(O)/AsmParser.afterburnt.s \
      $(O)/AsmParser.o $(O)/AsmParser.afterburnt.o
	diff $(O)/AsmParser.o $(O)/AsmParser.afterburnt.o

######################################################################
##
##  Process the antlr grammars, but only if rebuild_antlr=1
##
######################################################################

ifdef rebuild_antlr

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

clean_antlr:
	-rm -f *.class
	-rm -f *TokenTypes.txt *TokenTypes.hpp
	-rm -f *Parser.hpp *Parser.cpp *Lexer.hpp *Lexer.cpp

