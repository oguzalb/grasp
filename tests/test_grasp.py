import unittest
from grasp import Parser
from grasp import main


class GraspTest(unittest.TestCase):
    def _test_code(self, code, output):
        parser = Parser()
        main.parse_string(parser, code)
        self.assertEquals(parser.dumpcode(), output)

    def test_varname(self):
        self._test_code('var\n', 'pushglobal var\npop\n')

    def test_trailer(self):
        self._test_code("var.field1\n", "pushglobal var\nstr field1\ngetfield\npop\n")

    def test_expression(self):
        # easier to see
        tests = (
            (
"a/b\n", 
"""pushglobal a
pushglobal b
div
pop
"""),
            (
"a-b-c\n",
"""pushglobal a
pushglobal b
sub
pushglobal c
sub
pop
"""),
            (
"a-b+c*d/e\n",
"""pushglobal a
pushglobal b
sub
pushglobal c
pushglobal d
pushglobal e
div
mul
add
pop
"""),
            (
"a-b+c*d/e or f and g\n",
"""pushglobal a
pushglobal b
sub
pushglobal c
pushglobal d
pushglobal e
div
mul
add
pushglobal f
or
pushglobal g
and
pop
"""),
        )
        for code, output in tests:
            self._test_code(code, output)

    def test_funccall(self):
        tests = (
            (
"func1()\n",
"""pushglobal func1
call 0
pop
"""),
            ("func1(1)\n",
"""pushglobal func1
int 1
call 1
pop
"""),
            ("func1(\"str1\", \"str3\")\n",
"""pushglobal func1
str str1
str str3
call 2
pop
"""),
        )
        for code, output in tests:
            self._test_code(code, output)
    def test_return(self):
        tests = (
            ("return a+b\n",
"""pushglobal a
pushglobal b
add
return
"""),
        )
        for code, output in tests:
            self._test_code(code, output)
        # should be more

    def test_funcdef(self):
        tests = (    
            ("""
func1(a,b) ->
    func2(c)
    return a+b
""",
"""jmp l1
l2:pushglobal func2
pushglobal c
call 1
pop
pushlocal 0
pushlocal 1
add
return
pushglobal none
return
l1:function l2
setglobal func1
"""
            ), ("""
func1(c,d)
""", 
"""pushglobal func1
pushglobal c
pushglobal d
call 2
pop
"""
            ), ("""
func1(a) ->
    return a.b.c
func1(1)
""", 
"""jmp l1
l2:pushlocal 0
str b
getfield
str c
getfield
return
pushglobal none
return
l1:function l2
setglobal func1
pushglobal func1
int 1
call 1
pop
"""
            ), ("""
func1(a,b) ->
    return a+b
func1.func = func1
func1.func(1,2)
""", 
"""jmp l1
l2:pushlocal 0
pushlocal 1
add
return
pushglobal none
return
l1:function l2
setglobal func1
pushglobal func1
pushglobal func1
str func
swp
setfield
pushglobal func1
str func
getmethod
int 1
int 2
swp
call 3
pop
"""
            ), ("""
total(a) ->
    if a == 1
        return 1
    else
        return a + total(a - 1)
total(5)
""", 
"""jmp l1
l2:pushlocal 0
int 1
equals
jnt l4
int 1
return
jmp l3
l4:nop
l3:pushlocal 0
pushglobal total
pushlocal 0
int 1
sub
call 1
add
return
pushglobal none
return
l1:function l2
setglobal total
pushglobal total
int 5
call 1
pop
"""
            ), ("""
for a in range(5)
    print(a)
""",
"""pushglobal range
int 5
call 1
str iter
getmethod
swp
call 1
l1:loop l2
setglobal a
pushglobal print
pushglobal a
call 1
pop
jmp l1
l2:pop
"""     ), ("""
total2(a) ->
    tot = 0
    for i in range(a)
        tot = tot + i
    return tot
print(total2(5))
""", 
"""jmp l1
l2:int 0
setglobal tot
pushglobal range
pushlocal 0
call 1
str iter
getmethod
swp
call 1
l3:loop l4
setglobal i
pushglobal tot
pushglobal i
add
setglobal tot
jmp l3
l4:pop
pushglobal tot
return
pushglobal none
return
l1:function l2
setglobal total2
pushglobal print
pushglobal total2
int 5
call 1
call 1
pop
"""     )
        )
        for code, output in tests:
            self._test_code(code, output)
    def test_class(self):
        self._test_code(
"""
class Person
    __init__(self, name) ->
        self.name = name
    hello(self) ->
        print(self.name)
person = Person("oguz")
person.hello()
""", 
"""class
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
""")