from parse import Word, quotedString, delimitedList, Infix, Literal, IndentedBlock, Forward, Optional, Regex, Group
from StringIO import StringIO

class Parser():
    def __init__(self):
        self.code = StringIO()
        self.contexts = [{}]
    def reset(self):
        self.code = StringIO()
        self.contexts = [{}]
    def add_instruction(self, code):
        self.code.write(code + "\n")
    def __repr__(self):
        return "code:\n%s" % self.code.getvalue()
    def dumpcode(self):
        return self.code.getvalue()
    def push_context(self):
        self.contexts.append({})
    def pop_context(self):
        return self.contexts.pop()
    def add_var(self, varname):
        context = self.contexts[-1]
        if varname not in context:
            context[varname] = len(context)
    def get_var(self, varname):
        context = self.contexts[-1]
        if varname not in context:
            return -1
        return context[varname]
    def __str__(self):
        return self.__repr__()
word = Word()
varname = Word()
string = quotedString.copy()
def varname_action(parser, tokens):
    var_index = parser.get_var(tokens[0])
    if var_index != -1:
        parser.add_instruction("pushlocal %s" % var_index)
    else:
        parser.add_instruction("pushglobal %s" % tokens[0])
    return tokens
varname.set_action(varname_action)
def string_action(parser, tokens):
    parser.add_instruction("str %s" % tokens[0])
    return tokens
string.set_action(string_action)
number = Regex("\w+")
def number_action(parser, tokens):
    parser.add_instruction("int %s" % tokens[0])
    return tokens
number.set_action(number_action)
atom = varname | string | number
callparams = Literal('(') + Optional(delimitedList(atom, Literal(","))) + Literal(")")
def infix_action(name):
    def expr_action(parser, tokens):
        parser.add_instruction(name)
        return tokens
    return expr_action
def funccall_action(parser, tokens):
    parser.add_instruction("call %s" % (len(tokens[1]) if tokens[1] else 0))
    return tokens
callparams.set_action(funccall_action)
trailer = varname + Optional(callparams)
divexpr = Infix(trailer, Literal('/'))
divexpr.set_action(infix_action("div"))
mulexpr = Infix(divexpr, Literal('*'))
mulexpr.set_action(infix_action("mul"))
subexpr = Infix(mulexpr, Literal('-'))
subexpr.set_action(infix_action("sub"))
addexpr = Infix(subexpr, Literal('+'))
addexpr.set_action(infix_action("add"))
orexpr = Infix(addexpr, Literal('or'))
orexpr.set_action(infix_action("or"))
andexpr = Infix(orexpr, Literal('and'))
andexpr.set_action(infix_action('and'))

returnexpr = Literal('return') + andexpr
def return_action(parser, tokens):
    parser.add_instruction("return")
    return tokens
returnexpr.set_action(return_action)
# return is first, expr can get return as identifier!!!
exprstmt = Group(andexpr)
def exprstmt_action(parser, tokens):
    parser.add_instruction("pop")
    return tokens
funcdef = Forward()
primitivestmt = (returnexpr | exprstmt) + Literal("\n")
exprstmt.set_action(exprstmt_action)
stmt = funcdef | primitivestmt
defparams = Literal('(') + Optional(delimitedList(Word(), Literal(","))) + Literal(")")
def defparams_action(parser, tokens):
    for param in tokens[1]:
        parser.add_var(param)
    return tokens
defparams.set_action(defparams_action)
funcname = Word()
def funcname_action(parser, tokens):
    parser.add_instruction("function")
    parser.push_context()
    return tokens
funcname.set_action(funcname_action)
funcdef << funcname + defparams + Literal('->\n') + IndentedBlock(stmt)
def funcdef_action(parser, tokens):
    parser.add_instruction("endfunction")
    parser.add_instruction("setglobal %s" % tokens[0][0])
    parser.pop_context()
    return tokens
funcdef.set_action(funcdef_action)
main = IndentedBlock(stmt)
parser = Parser()
print varname.parseString(parser, "asdasd")
print parser
parser.reset()
print trailer.parseString(parser, "asdasd")
print parser
parser.reset()
divexpr.parseString(parser, "a/b")
print parser
parser.reset()
subexpr.parseString(parser, "a-b-c")
print parser
parser.reset()
addexpr.parseString(parser, "a-b+c*d/e")
print parser
parser.reset()
andexpr.parseString(parser, "a-b+c*d/e or f and g")
print parser
parser.reset()
callparams.parseString(parser, "(\"str1\", \"str3\")")
print parser
parser.reset()
andexpr.parseString(parser, "func1(\"str1\", \"str3\")")
print parser
parser.reset()
returnexpr.parseString(parser, 'return a+b')
print parser
parser.reset()
funcdef.parseString(parser,
"""func1(a,b) ->
    func2(c)
    return a+b
"""
)
print parser
parser.reset()
main.parseString(parser,
"""func1(a,b) ->
    func2()
    return a+b
func1(c,d)
"""
)
print parser
parser.reset()
main.parseString(parser,
"""func1(a,b) ->
    return a+b
func1(1,2)
"""
)
print parser
with open("test.graspo", "w") as f:
    f.write(parser.dumpcode())
    f.close()
