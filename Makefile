all: compiler.cpp
	g++ -std=c++11 compiler.cpp  `llvm-config --cppflags --ldflags --libs --system-libs all` -o compiler

testAddRecursive: testAddRecursive.cpp addRecursive.o
	g++ testAddRecursive.cpp addRecursive.o -o testAddRecursive

addRecursive.o: addRecursive.ll
	llc -filetype=obj addRecursive.ll

clean:
	rm -f compiler testAddRecursive *.o
