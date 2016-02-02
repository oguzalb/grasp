LIBS=
FLAGS=-std=c++0x -g -Wall
TYPES=int.cpp string.cpp object.cpp builtin_function.cpp class.cpp bool.cpp function.cpp list_iterator.cpp list.cpp
SOURCES=$(TYPES:%.cpp=types/%.cpp)
OBJECTS=$(SOURCES:.cpp=.o)
.cpp.o:
	g++ -c $< ${LIBS} ${FLAGS} -o $@

all: ${OBJECTS}
	g++ vm.cpp ${OBJECTS} -o vm.out ${LIBS} ${FLAGS}

test: all
	flake8 parser/parse.py grasp.py
	python -m unittest discover tests
	./grasp example.grasp
	./vm.out example.graspo
clean:
	rm -f vm.out example.graspo
	rm -f types/*.o
	rm -Rf vm.out.dSYM
