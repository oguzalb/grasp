import unittest
from grasp import Parser
from grasp import main


class GraspTest(unittest.TestCase):
    def _test_code(self, code, output, i):
        parser = Parser()
        main.parse_string(parser, code)
        self.assertEquals(parser.dumpcode(), output, "test %s, %s: \"%s\" != \"%s\"" % (code, i, parser.dumpcode(), output))

    def test_varname(self):
        self._test_code('var\n', 'str var\npushglobal 1\npop\n', 0)

    def test_trailer(self):
        self._test_code("var.field1\n", "str var\nstr field1\npushglobal 1\npushconst 2\ngetfield\npop\n", 0)

    def test_expression(self):
        # easier to see
        tests = (
            (
"a/b\n", 
"""str a
str __div__
str b
pushglobal 1
pushconst 2
getmethod
pushglobal 3
call 2
pop
"""),
            (
"a-b-c\n",
"""str a
str __sub__
str b
str c
pushglobal 1
pushconst 2
getmethod
pushglobal 3
call 2
pushconst 2
getmethod
pushglobal 4
call 2
pop
"""),
            (
"a-b+c*d/e\n",
"""str a
str __sub__
str b
str __add__
str c
str __mul__
str d
str __div__
str e
pushglobal 1
pushconst 2
getmethod
pushglobal 3
call 2
pushconst 4
getmethod
pushglobal 5
pushconst 6
getmethod
pushglobal 7
pushconst 8
getmethod
pushglobal 9
call 2
call 2
call 2
pop
"""),
            (
"a-b+c*d/e or f and g\n",
"""str a
str __sub__
str b
str __add__
str c
str __mul__
str d
str __div__
str e
str __or__
str f
str __and__
str g
pushglobal 1
pushconst 2
getmethod
pushglobal 3
call 2
pushconst 4
getmethod
pushglobal 5
pushconst 6
getmethod
pushglobal 7
pushconst 8
getmethod
pushglobal 9
call 2
call 2
call 2
pushconst 10
getmethod
pushglobal 11
call 2
pushconst 12
getmethod
pushglobal 13
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
"""str func1
pushglobal 1
call 0
pop
"""),
            ("func1(1)\n",
"""str func1
int 1
pushglobal 1
pushconst 2
call 1
pop
"""),
            ("func1(\"str1\", \"str3\")\n",
"""str func1
str str1
str str3
pushglobal 1
pushconst 2
pushconst 3
call 2
pop
"""),
        )
        for i, (code, output) in enumerate(tests):
            self._test_code(code, output, i)
    def test_return(self):
        tests = (
            ("return a+b\n",
"""str a
str __add__
str b
pushglobal 1
pushconst 2
getmethod
pushglobal 3
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
"""str func2
str c
str __add__
str func1
jmp 31
pushglobal 1
pushglobal 2
call 1
pop
pushlocal 0
pushconst 3
getmethod
pushlocal 1
call 2
return
pushconst 0
return
function -28 0 2
setglobal 4
"""
            ), ("""
func1(c,d)
""", 
"""str func1
str c
str d
pushglobal 1
pushglobal 2
pushglobal 3
call 2
pop
"""
            ), ("""
func1(a) ->
    return a.b.c
func1(1)
""", 
"""str b
str c
str func1
int 1
jmp 19
pushlocal 0
pushconst 1
getfield
pushconst 2
getfield
return
pushconst 0
return
function -16 0 1
setglobal 3
pushglobal 3
pushconst 4
call 1
pop
"""
            ), ("""
func1(a,b) ->
    return a+b
func1.func = func1
func1.func(1,2)
""", 
"""str __add__
str func1
str func
int 1
int 2
jmp 21
pushlocal 0
pushconst 1
getmethod
pushlocal 1
call 2
return
pushconst 0
return
function -18 0 2
setglobal 2
pushglobal 2
pushglobal 2
pushconst 3
swp
setfield
pushglobal 2
pushconst 3
getmethod
pushconst 4
pushconst 5
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
"""str __equals__
int 1
str __add__
str total
str __sub__
int 5
jmp 61
pushlocal 0
pushconst 1
getmethod
pushconst 2
call 2
jnt 10
pushconst 2
return
jmp 4
nop
pushlocal 0
pushconst 3
getmethod
pushglobal 4
pushlocal 0
pushconst 5
getmethod
pushconst 2
call 2
call 1
call 2
return
pushconst 0
return
function -58 0 1
setglobal 4
pushglobal 4
pushconst 6
call 1
pop
"""
            ), ("""
for a in range(5)
    print(a)
""",
"""str range
int 5
str iter
str a
str print
pushglobal 1
pushconst 2
call 1
pushconst 3
getmethod
call 1
loop 19
setglobal 4
pushglobal 5
pushglobal 4
call 1
pop
jmp -16
nop
"""     ), ("""
total2(a) ->
    tot = 0
    for i in range(a)
        tot = tot + i
    return tot
print(total2(5))
""", 
"""int 0
str range
str iter
str __add__
str total2
str print
int 5
jmp 58
pushconst 1
setlocal 1
pushglobal 2
pushlocal 0
call 1
pushconst 3
getmethod
call 1
loop 25
setlocal 2
pushlocal 1
pushconst 4
getmethod
pushlocal 2
call 2
setlocal 1
jmp -22
pushlocal 1
return
pushconst 0
return
function -55 2 1
setglobal 5
pushglobal 6
pushglobal 5
pushconst 7
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
"""str Person
str name
str __init__
str print
str hello
str oguz
str person
class
dup
setglobal 1
jmp 18
pushlocal 0
pushlocal 1
pushconst 2
swp
setfield
pushconst 0
return
dup
pushconst 3
function -19 0 2
setfield
jmp 21
pushglobal 4
pushlocal 0
pushconst 2
getfield
call 1
pop
pushconst 0
return
dup
pushconst 5
function -22 0 1
setfield
pop
pushglobal 1
pushconst 6
call 1
setglobal 7
pushglobal 7
pushconst 5
getmethod
call 1
pop
""", 0)
