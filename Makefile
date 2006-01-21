
antlr_root    ?= $(HOME)/apps
antlr_include ?= $(antlr_root)/include
antlr_lib     ?= $(antlr_root)/lib

#antlr_jar ?= /home/joshua/apps/lib/antlr.jar
#java_bin  ?= /tools/java/bin
#gcc_bin   ?= /tools/bin

#PATH := $(java_bin):$(gcc_bin):$(PATH)
#CLASSPATH := .:$(antlr_jar)
#export CLASSPATH PATH

CPPFLAGS  += -Wall -O2 -I$(antlr_include)
LDFLAGS += -L$(antlr_lib)
LIBS    += -lantlr

all: afterburner

afterburner: afterburner.o AsmLexer.o AsmParser.o
	g++ $(CPPFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

afterburner.o: AsmLexer.hpp AsmParser.hpp
AsmParser.o: AsmParser.hpp

AsmParser.cpp AsmParser.hpp: Asm.g
AsmLexer.hpp: Asm.g

clean:
	-rm -f *.class
	-rm -f *TokenTypes.txt *TokenTypes.hpp
	-rm -f *Parser.hpp *Parser.cpp *Lexer.hpp *Lexer.cpp
	-rm -f *.o
	-rm -f afterburner

%Lexer.hpp \
%Parser.cpp %Parser.hpp \
%TreeParser.cpp %TreeParser.hpp \
%ParserTokenTypes.hpp %ParserTokenTypes.txt: %.g
	antlr $<

