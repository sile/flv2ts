OPT=-O3 -Wall -Werror -Iinclude

#ALL=parse-flv flv2ts parse-ts ts-extract flv-extract build-hls-index getts libgetts
ALL=getts build-hls-index flv2ts libgetts

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

libgetts: src/lib/getts.cc
	mkdir -p lib
	g++ -fPIC ${OPT} -c src/lib/getts.cc
	g++ -shared -Wl,-soname=${@}.so.1 -o lib/${@}.so.1.0.0 getts.o
	rm -f *.o