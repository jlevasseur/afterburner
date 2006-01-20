
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
LDFLAGs += -L$(antlr_lib)
LIBS    += -lantlr

all: afterburner

afterburner: AsmLexer.o AsmParser.o AsmTreeParser.o

clean:
	-rm -f *.class
	-rm -f *TokenTypes.txt
	-rm -f *.o
	-rm -f afterburner

%Lexer.cpp %Lexer.hpp \
%Parser.cpp %Parser.hpp \
%TreeParser.cpp %TreeParser.hpp \
%ParserTokenTypes.hpp %ParserTokenTypes.txt: %.g
#	java antlr.Tool $<
	antlr $<

