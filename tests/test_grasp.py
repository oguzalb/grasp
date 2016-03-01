import unittest
from grasp import Parser
from grasp import main


class GraspTest(unittest.TestCase):
    def _test_code(self, code, output, i):
        parser = Parser()
        main.parse_string(parser, code)
        self.assertEquals(parser.dumpcode(), output, "test %s, %s: \"%s\" != \"%s\"" % (code, i, parser.dumpcode(), output))

    def test_varname(self):
        self._test_code('var\n', 'pushglobal var\npop\n', 0)

    def test_trailer(self):
        self._test_code("var.field1\n", "pushglobal var\nstr field1\ngetfield\npop\n", 0)

    def test_expression(self):
        # easier to see
        tests = (
            (
"a/b\n", 
"""pushglobal a
str __div__
getmethod
pushglobal b
call 2
pop
"""),
            (
"a-b-c\n",
"""pushglobal a
str __sub__
getmethod
pushglobal b
call 2
str __sub__
getmethod
pushglobal c
call 2
pop
"""),
            (
"a-b+c*d/e\n",
"""pushglobal a
str __sub__
getmethod
pushglobal b
call 2
str __add__
getmethod
pushglobal c
str __mul__
getmethod
pushglobal d
str __div__
getmethod
pushglobal e
call 2
call 2
call 2
pop
"""),
            (
"a-b+c*d/e or f and g\n",
"""pushglobal a
str __sub__
getmethod
pushglobal b
call 2
str __add__
getmethod
pushglobal c
str __mul__
getmethod
pushglobal d
str __div__
getmethod
pushglobal e
call 2
call 2
call 2
str __or__
getmethod
pushglobal f
call 2
str __and__
getmethod
pushglobal g
call 2
pop
"""),
        )
        for i, (code, output) in enumerate(tests):
            self._test_code(code, output, i)

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
        for i, (code, output) in enumerate(tests):
            self._test_code(code, output, i)
    def test_return(self):
        tests = (
            ("return a+b\n",
"""pushglobal a
str __add__
getmethod
pushglobal b
call 2
return
"""),
        )
        for i, (code, output) in enumerate(tests):
            self._test_code(code, output, i)
        # should be more

    def test_funcdef(self):
        tests = (    
            ("""
func1(a,b) ->
    func2(c)
    return a+b
""",
"""jmp 13
pushglobal func2
pushglobal c
call 1
pop
pushlocal 0
str __add__
getmethod
pushlocal 1
call 2
return
pushglobal None
return
function -12 0
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
"""jmp 9
pushlocal 0
str b
getfield
str c
getfield
return
pushglobal None
return
function -8 0
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
"""jmp 9
pushlocal 0
str __add__
getmethod
pushlocal 1
call 2
return
pushglobal None
return
function -8 0
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
"""jmp 25
pushlocal 0
str __equals__
getmethod
int 1
call 2
jnt 4
int 1
return
jmp 2
nop
pushlocal 0
str __add__
getmethod
pushglobal total
pushlocal 0
str __sub__
getmethod
int 1
call 2
call 1
call 2
return
pushglobal None
return
function -24 0
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
call 1
loop 7
setglobal a
pushglobal print
pushglobal a
call 1
pop
jmp -6
pop
"""     ), ("""
total2(a) ->
    tot = 0
    for i in range(a)
        tot = tot + i
    return tot
print(total2(5))
""", 
"""jmp 23
int 0
setlocal 1
pushglobal range
pushlocal 0
call 1
str iter
getmethod
call 1
loop 9
setlocal 2
pushlocal 1
str __add__
getmethod
pushlocal 2
call 2
setlocal 1
jmp -8
pop
pushlocal 1
return
pushglobal None
return
function -22 2
setglobal total2
pushglobal print
pushglobal total2
int 5
call 1
call 1
pop
"""     )
        )
        for i, (code, output) in enumerate(tests):
            self._test_code(code, output, i)
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
jmp 8
pushlocal 0
pushlocal 1
str name
swp
setfield
pushglobal None
return
dup
str __init__
function -9 0
setfield
jmp 9
pushglobal print
pushlocal 0
str name
getfield
call 1
pop
pushglobal None
return
dup
str hello
function -10 0
setfield
pop
pushglobal Person
str oguz
call 1
setglobal person
pushglobal person
str hello
getmethod
call 1
pop
""", 0)
