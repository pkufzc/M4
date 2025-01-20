# CPFLAGS = -g -Wall -O2 -std=c++17 -lm -w


# test: main.cpp
# 		g++ main.cpp -o main $(CPFLAGS)

# clean:
# 		rm test

CXX = g++
CXXFLAGS = -g -Wall -O2 -std=c++17 -lm

all: tdigest mreq dd ddc

tdigest:
	rm -f tdigest
	$(CXX) $(CXXFLAGS) -D TEST_TD main.cpp -o tdigest

mreq:
	rm -f mreq
	$(CXX) $(CXXFLAGS) -D TEST_MREQ main.cpp -o mreq

dd:
	rm -f dd
	$(CXX) $(CXXFLAGS) -D TEST_DD main.cpp -o dd

ddc:
	rm -f ddc
	$(CXX) $(CXXFLAGS) -D TEST_DDC main.cpp -o ddc

clean:
	rm -f tdigest mreq dd ddc

.PHONY: all tdigest mreq dd ddc clean
