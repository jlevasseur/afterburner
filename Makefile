
antlr_root    ?= /home/joshua/apps
antlr_include ?= $(antlr_root)/include
antlr_lib     ?= $(antlr_root)/lib

O ?= .

all: $(O)/afterburner

CPPFLAGS  += -Wall -O2 -I$(antlr_include)
LDFLAGS += -L$(antlr_lib)
LIBS    += -lantlr

$(O)/afterburner: $(O)/afterburner.o \
                  $(O)/AsmLexer.o $(O)/AsmParser.o $(O)/AsmTreeParser.o
	g++ $(CPPFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

$(O)/afterburner.o: AsmLexer.hpp AsmParser.hpp AsmTreeParser.hpp afterburner.cpp
$(O)/AsmParser.o: AsmParser.hpp AsmParser.cpp
$(O)/AsmLexer.o: AsmLexer.hpp AsmLexer.cpp
$(O)/AsmTreeParser.o: AsmTreeParser.hpp AsmTreeParser.cpp

$(O)/%.s: %.cpp
	g++ $(CPPFLAGS) -S $< -o $@
$(O)/%.afterburnt.s: %.s $(O)/afterburner
	$(O)/afterburner $< > $@
$(O)/%.o.s: $(O)/%.o
	objdump -d $< > $@

.PHONY: test
test: $(O)/afterburner $(O)/AsmParser.s $(O)/AsmParser.afterburnt.s \
      $(O)/AsmParser.o $(O)/AsmParser.o.s \
      $(O)/AsmParser.afterburnt.o $(O)/AsmParser.afterburnt.o.s
	

clean:
	-rm -f $(O)/*.o $(O)/afterburner $(O)/*.s


ifdef rebuild_antlr

AsmTreeParser.cpp AsmTreeParser.hpp: Asm.g
AsmParser.cpp AsmParser.hpp: Asm.g
AsmLexer.hpp AsmLexer.cpp: Asm.g

%Lexer.hpp %Lexer.cpp \
%Parser.cpp %Parser.hpp \
%TreeParser.cpp %TreeParser.hpp \
%ParserTokenTypes.hpp %ParserTokenTypes.txt: %.g
	$(antlr_root)/bin/antlr $<

endif

clean_antlr:
	-rm -f *.class
	-rm -f *TokenTypes.txt *TokenTypes.hpp
	-rm -f *Parser.hpp *Parser.cpp *Lexer.hpp *Lexer.cpp

