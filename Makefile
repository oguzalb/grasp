LIBS=
FLAGS=-std=c++0x
all:
	g++ vm.cpp -o vm.out ${LIBS} ${FLAGS}
test: all
	python grasp.py
	./vm.out
clean:
	rm vm.out
