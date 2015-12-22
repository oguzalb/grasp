LIBS=
FLAGS=-std=c++0x -g
all:
	g++ vm.cpp -o vm.out ${LIBS} ${FLAGS} -Wall
test: all
	python grasp.py
	./vm.out
clean:
	rm vm.out
