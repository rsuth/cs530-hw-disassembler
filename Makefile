CXX=g++
CXXFLAGS=-std=c++11 -g

dissem :
	$(CXX) $(CXXFLAGS) -o dissem main.cpp

clean :
	rm -rf dissem dissem.dSYM || true
