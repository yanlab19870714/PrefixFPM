CCOMPILE=g++
CPPFLAGS=  -I system -Wno-deprecated -O2 -g -fopenmp

all: run

run: run.cpp
	$(CCOMPILE) -std=c++11 run.cpp $(CPPFLAGS)  -o run -lpthread

clean:
	-rm run
