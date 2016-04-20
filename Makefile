LIBS=`mysql_config --cflags --libs`
FLAGS=-std=c++0x -g -Wall -v -O3 -DCALC_FLAG #-DDEBUG 
TYPES=int.cpp string.cpp object.cpp builtin_function.cpp class.cpp bool.cpp function.cpp list_iterator.cpp list.cpp exception.cpp module.cpp dict.cpp string_stream.cpp
MODULES=grmysql.cpp
SOURCES=$(TYPES:%.cpp=types/%.cpp) $(MODULES:%.cpp=modules/%.cpp)
OBJECTS=$(SOURCES:.cpp=.o) vm.o
.cpp.o:
	g++ -c $< ${LIBS} ${FLAGS} -o $@

all: ${OBJECTS}
	g++ stackmachine.cpp ${OBJECTS} -o stackmachine.out ${LIBS} ${FLAGS}

test: all
	flake8 parser/parse.py grasp.py
	python -m unittest discover tests
	./grasp example.grasp
	./stackmachine.out example.graspo
repl: all
	g++ repl.cpp ${OBJECTS} -o repl.out ${LIBS} ${FLAGS}
	
clean:
	rm -f stackmachine.out repl.out example.graspo
	rm -f types/*.o modules/*.o *.o *.graspo
	rm -Rf *.out.dSYM
