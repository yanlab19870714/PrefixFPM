CCOMPILE=g++
CPPFLAGS=  -I system -Wno-deprecated -O2 -g -fopenmp

all: run

run: run_sleuth.cpp
	$(CCOMPILE) -std=c++11 run_sleuth.cpp $(CPPFLAGS)  -o run -lpthread

clean:
	-rm run
