CC = g++
INC_PATH = -I/Users/chris/boost_1_55_0 \
    -I/Users/chris/tbb41_20130516oss/include
LIB_PATH = -L/Users/chris/boost_1_55_0/stage/lib \
    -L/Users/chris/tbb41_20130516oss/build/macos_intel64_gcc_cc4.8.2_os10.9_release
LIBS = -lpthread -lboost_system -lboost_thread -lboost_timer -ltbb
CFLAGS = -std=c++0x -O2 -march=native -mtune=native $(INC_PATH)
#CFLAGS = -std=c++0x -O0 -ggdb $(INC_PATH)
LDFLAGS = $(LIB_PATH) $(LIBS)

all: emtree

emtree: src/EMTree.cpp
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

.PHONY: clean cleanest emtree

clean:
	rm -f *.o

cleanest: clean
	rm -f emtree
