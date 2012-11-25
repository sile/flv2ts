OPT=-g -Wall -Werror -Iinclude

#ALL=parse-flv flv2ts parse-ts ts-extract flv-extract build-hls-index getts
ALL=getts build-hls-index

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

ts-extract: src/bin/ts-extract.cc
	mkdir -p bin
	g++ ${OPT} -o bin/${@} src/bin/${@}.cc

flv-extract: src/bin/flv-extract.cc
	mkdir -p bin
	g++ ${OPT} -o bin/${@} src/bin/${@}.cc

build-hls-index: src/bin/build-hls-index.cc
	mkdir -p bin
	g++ ${OPT} -o bin/${@} src/bin/${@}.cc

getts: src/bin/getts.cc
	mkdir -p bin
	g++ ${OPT} -o bin/${@} src/bin/${@}.cc
