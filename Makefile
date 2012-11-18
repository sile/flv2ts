OPT=-g -Wall -Werror -Iinclude

# ALL=parse-flv flv2ts parse-ts
ALL=parse-ts

all: ${ALL}

parse-flv: src/bin/parse-flv.cc
	mkdir -p bin
	g++ ${OPT} -o bin/parse-flv src/bin/parse-flv.cc

parse-ts: src/bin/parse-ts.cc
	mkdir -p bin
	g++ ${OPT} -o bin/parse-ts src/bin/parse-ts.cc

flv2ts: src/bin/flv2ts.cc
	mkdir -p bin
	g++ ${OPT} -o bin/flv2ts src/bin/flv2ts.cc
