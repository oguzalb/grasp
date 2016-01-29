Parser written with python which complies with API of pyparsing, will contain a compiler and stack machine for imaginary language grasp

=== STILL TRYING STUFF, AWFULL CODE ===


The grasp grammar is something like python with arrow function definition

now it can parse
class Person
    __init__(self, name) ->
        self.name = name
    hello(self) ->
        print(self.name)
person = Person("oguz")
person.hello()

Project consists of three parts
=== parse.py ===
This is something like the pyparsing written from scratch

=== grasp.py ===
This is the compiler written using parse.py, which generates stack machine codes. As an example the class definition will be translated into:

class
dup
setglobal Person
jmp l1
l2:pushlocal 0
pushlocal 1
str name
swp
setfield
pushglobal none
return
l1:dup
str __init__
function l2
setfield
jmp l3
l4:pushglobal print
pushlocal 0
str name
getfield
call 1
pop
pushglobal none
return
l3:dup
str hello
function l4
setfield
pop
pushglobal Person
str oguz
call 1
setglobal person
pushglobal person
str hello
getmethod
swp
call 1
pop

=== vm.cpp, vm.h ===
This part is the vm code, interprets the bytecode. Bytecode is not binary for now, that will be done in finalization of the project
