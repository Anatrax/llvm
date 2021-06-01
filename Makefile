# Figure out what the name of the llvm-config executable is (assuming a limited
# number of options for the common environments in which we'll be building this
# code for this course).
LLVM_CONFIG := $(shell which llvm-config-7.0-64)
ifndef LLVM_CONFIG
	LLVM_CONFIG := $(shell which llvm-config-9)
endif
ifndef LLVM_CONFIG
	LLVM_CONFIG := llvm-config
endif

all: compiler.cpp
	g++ -std=c++11 compiler.cpp $(shell $(LLVM_CONFIG) --cppflags --ldflags --libs --system-libs all) -o compiler

testAddRecursive: testAddRecursive.cpp addRecursive.o
	g++ testAddRecursive.cpp addRecursive.o -o testAddRecursive

addRecursive.o: addRecursive.ll
	llc -filetype=obj addRecursive.ll

clean:
	rm -f compiler testAddRecursive *.o
