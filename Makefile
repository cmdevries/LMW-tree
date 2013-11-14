CC = g++
CFLAGS = -I/Users/chris/boost_1_55_0 -std=c++0x -O2 -march=native -mtune=native
#CFLAGS = -I/Users/chris/boost_1_55_0 -std=c++0x -O0 -ggdb 
LDFLAGS = -L/Users/chris/boost_1_55_0/stage/lib -L/opt/local/lib -lpthread -lboost_system -lboost_thread -lboost_timer #-lrt

all: emtree

emtree: EMTree.o
	$(CC) -o $@ $^ $(LDFLAGS)

EMTree.o: src/EMTree.cpp
	$(CC) -c $(CFLAGS) $<

.PHONY: clean cleanest EMTree.o emtree

clean:
	rm -f *.o

cleanest: clean
	rm -f emtree
