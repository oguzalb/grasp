from parse import Word, quotedString, delimitedList, Infix, Literal, IndentedBlock, Forward, Optional, Regex, Group, Postfix, LookAheadLiteral, PostfixWithoutLast, Action
from StringIO import StringIO

class Parser():
    def __init__(self):
        self.code = StringIO()
        self.contexts = [{}]
        self.label_counter = 0
        self.next_label = None
    def reset(self):
        self.code = StringIO()
        self.contexts = [{}]
        self.label_counter = 0
        self.next_label = None
    def add_instruction(self, code):
        if self.next_label:
            self.code.write(self.next_label + ":" + code + "\n")
            self.next_label = None
        else:
            self.code.write(code + "\n")
    def new_label(self):
        self.label_counter += 1
        return "l"+str(self.label_counter)
    def set_next_label(self, label):
        if self.next_label is not None:
            self.add_instruction("nop")
        self.next_label = label
    def __repr__(self):
        return "code:\n%s" % self.code.getvalue()
    def dumpcode(self):
        return self.code.getvalue() + "" if not self.next_label else self.next_label + ":nop"
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
access_op = Literal(".")
def accessor_action(parser, tokens):
    parser.add_instruction("str " + tokens[0][1])
    parser.add_instruction("getfield")
    return tokens
fieldname = Word()
accessor = PostfixWithoutLast(access_op + fieldname)
accessor.set_action(accessor_action)
last_accessor = Group(access_op + fieldname)
last_accessor.set_action(accessor_action)
access = atom + Optional(accessor) + Optional(last_accessor)
def access_action(parser, tokens):
    return tokens
access.set_action(access_action)
trailer = access + Optional(callparams)
divexpr = Infix(trailer, Literal('/'))
divexpr.set_action(infix_action("div"))
mulexpr = Infix(divexpr, Literal('*'))
mulexpr.set_action(infix_action("mul"))
subexpr = Infix(mulexpr, Literal('-'))
subexpr.set_action(infix_action("sub"))
addexpr = Infix(subexpr, Literal('+'))
addexpr.set_action(infix_action("add"))
equalsexpr = Infix(addexpr, Literal("=="))
equalsexpr.set_action(infix_action("equals"))
orexpr = Infix(equalsexpr, Literal('or'))
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
rightvalue = Group(andexpr)
setfield = access_op + fieldname
simpleassgmt = Word() + Literal("=") + Group(andexpr) + Literal("\n")
def simpleassgmt_action(parser, tokens):
    var_index = parser.get_var(tokens[0])
    if var_index != -1:
        parser.add_instruction("setlocal %s" % var_index)
    else:
        parser.add_instruction("setglobal %s" % tokens[0])
    return tokens
simpleassgmt.set_action(simpleassgmt_action)
 
assgmt = varname + Optional(accessor) + setfield + Literal("=") + Group(andexpr) + Literal("\n")
def assgmt_action(parser, tokens):
    parser.add_instruction("str " + tokens[2][1])
    parser.add_instruction("swp")
    parser.add_instruction("setfield")
    return tokens
assgmt.set_action(assgmt_action)
stmt = Forward()
ifexpr = Group(andexpr)
def ifexpr_action(parser, tokens):
    partend = parser.new_label()
    parser.add_instruction("jnt %s" % partend)
    return [partend]
ifexpr.set_action(ifexpr_action)

jmp_to_end = Action(pass_params=[3, 0])
def jmp_to_end_action(parser, tokens):
    parser.add_instruction("jmp %s" % tokens[0])
    parser.set_next_label(tokens[1])
    return tokens
jmp_to_end.set_action(jmp_to_end_action)
if_start = Literal("if")
def if_start_action(parser, tokens):
    ifend = parser.new_label()
    return [ifend]
if_start.set_action(if_start_action)
if_part = ifexpr + Literal("\n") + IndentedBlock(stmt) + jmp_to_end
if_part.pass_params = [0]
ifstmt = if_start + if_part
def ifstmt_action(parser, tokens):
    blockend = parser.new_label()
    parser.set_next_label(tokens[0])
    return tokens
ifstmt.set_action(ifstmt_action)
stmt << (funcdef | assgmt | simpleassgmt | primitivestmt | ifstmt)
defparams = Literal('(') + Optional(delimitedList(Word(), Literal(","))) + Literal(")")
def defparams_action(parser, tokens):
    for param in tokens[1]:
        parser.add_var(param)
    return tokens
defparams.set_action(defparams_action)
funcname = Word()
def funcname_action(parser, tokens):
    endlabel = parser.new_label()
    parser.add_instruction("jmp " + endlabel)
    startlabel = parser.new_label()
    parser.set_next_label(startlabel)
    parser.push_context()
    return [[tokens[0], startlabel, endlabel]]
funcname.set_action(funcname_action)
funcdef << funcname + defparams + Literal('->\n') + IndentedBlock(stmt)
def funcdef_action(parser, tokens):
    # TODO this return value issue should be fixed on parse lib
    parser.set_next_label(tokens[0][0][2])
    parser.add_instruction("function %s" % tokens[0][0][1])
    parser.add_instruction("setglobal %s" % tokens[0][0][0])
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
"""func1(a) ->
    return a.b.c
func1(1)
"""
)
print parser
parser.reset()
main.parseString(parser,
"""func1(a,b) ->
    return a+b
func1.func = func1
func1.func(1,2)
"""
)
print parser
parser.reset()
main.parseString(parser,
"""func1(a,b) ->
    if a == 1
        a = 2
    return a+b
func1.func = func1
func1.func(1,2)
"""
)

print parser
with open("test.graspo", "w") as f:
    f.write(parser.dumpcode())
    f.close()
