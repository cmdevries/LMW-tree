CC = g++
INC_PATH = -I/Users/chris/boost_1_55_0 \
    -I../../contrib/strtk
LIB_PATH = -L/Users/chris/boost_1_55_0/stage/lib
LIBS = -lpthread -lboost_iostreams -lboost_system -lboost_thread -lboost_timer
CFLAGS = -std=c++0x -O2 -march=native -mtune=native $(INC_PATH)
#CFLAGS = -std=c++0x -O0 -ggdb $(INC_PATH)
LDFLAGS = $(LIB_PATH) $(LIBS)

all: titles 

titles: TitleExtractor.cpp
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

.PHONY: clean cleanest titles 

clean:
	rm -f *.o

cleanest: clean
	rm -f emtree
