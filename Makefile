LIBS=`pkg-config --cflags glib-2.0` `pkg-config --libs glib-2.0`
FLAGS=-std=c++11
all:
	g++ vm.cpp -o vm.out ${LIBS} ${FLAGS}
test: all
	python grasp.py
	./vm.out
clean:
	rm vm.out
