OPT=-O2 -Wall -Werror -Iinclude

all: bin/parse-flv

bin/parse-flv: src/bin/parse-flv.cc include/flv/parser.hh
	mkdir -p bin
	g++ ${OPT} -o bin/parse-flv src/bin/parse-flv.cc
