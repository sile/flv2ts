OPT=-g -Wall -Werror -Iinclude

all: parse-flv

parse-flv: src/bin/parse-flv.cc
	mkdir -p bin
	g++ ${OPT} -o bin/parse-flv src/bin/parse-flv.cc
