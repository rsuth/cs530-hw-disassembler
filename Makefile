CXX=g++
CXXFLAGS=-std=c++11 -g

dissem : main.o ListRow.o
	$(CXX) $(CXXFLAGS) -o dissem main.o ListRow.o

main.o: main.cpp
	$(CXX) $(CXXFLAGS) -c main.cpp

ListRow.o: ListRow.cpp
	$(CXX) $(CXXFLAGS) -c ListRow.cpp

clean :
	rm -rf dissem dissem.dSYM *.o || true
