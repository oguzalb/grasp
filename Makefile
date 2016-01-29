LIBS=
FLAGS=-std=c++0x -g
all:
	g++ vm.cpp -o vm.out ${LIBS} ${FLAGS} -Wall -g
test: all
	flake8 parse.py grasp.py
	python -m unittest test_grasp test_parse
	python grasp.py
	./vm.out
clean:
	rm vm.out
