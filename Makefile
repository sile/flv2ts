OPT=-g -Wall -Werror -Iinclude

# ALL=parse-flv flv2ts parse-ts ts-extract-audio
ALL=ts-extract-audio

all: ${ALL}

parse-flv: src/bin/parse-flv.cc
	mkdir -p bin
	g++ ${OPT} -o bin/${@} src/bin/${@}.cc

parse-ts: src/bin/parse-ts.cc
	mkdir -p bin
	g++ ${OPT} -o bin/${@} src/bin/${@}.cc

flv2ts: src/bin/flv2ts.cc
	mkdir -p bin
	g++ ${OPT} -o bin/${@} src/bin/${@}.cc

ts-extract-audio: src/bin/ts-extract-audio.cc
	mkdir -p bin
	g++ ${OPT} -o bin/${@} src/bin/${@}.cc
