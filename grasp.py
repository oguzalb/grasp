from parse import Word, quotedString, delimitedList, Infix, Literal

word = Word()
print word.parse("sadasd11_%%", 0)
print word.parseString("sadasd11_")
print quotedString.parse("\"asdasdasd\"", 0)
atom = word | quotedString
print atom.parseString("sadasd")
callparams = Literal('(').suppress() + delimitedList(atom, Literal(",").suppress()) + Literal(")").suppress()
def infix_action(name):
    def expr_action(tokens):
        if len(tokens) > 1:
            if not isinstance(tokens[0], list):
                print "push " + tokens[0]
            if not isinstance(tokens[2], list):
                print "push " + tokens[2]
            print name
        return tokens
    return expr_action
divexpr = Infix(atom, Literal('/'))
divexpr.set_action(infix_action("div"))
mulexpr = Infix(divexpr, Literal('*'))
mulexpr.set_action(infix_action("mul"))
subexpr = Infix(mulexpr, Literal('-'))
subexpr.set_action(infix_action("sub"))
print subexpr.parseString("a-b")
print subexpr.parseString("a-b-c")
addexpr = Infix(subexpr, Literal('+'))
addexpr.set_action(infix_action("add"))
print addexpr.parseString("a-b+c*d/e")
orexpr = Infix(addexpr, Literal('or'))
orexpr.set_action(infix_action("or"))
andexpr = Infix(orexpr, Literal('and'))
andexpr.set_action(infix_action('and'))
print andexpr.parseString("a-b+c*d/e or f and g")

funccall = Word() + callparams
def funccall_action(tokens):
    if len(tokens) > 1:
        print "\n".join(("push " + token for token in tokens[1:]))
    print "call %s %s" % (tokens[0], len(tokens) - 1)
    return []
funccall.set_action(funccall_action)
print callparams.parseString("(\"str1\", \"str3\")")
print funccall.parseString("func1(\"str1\", \"str3\")")
returnexpr = Literal('return') + andexpr
def return_action(tokens):
    print "return"
    return tokens
returnexpr.set_action(return_action)
print returnexpr.parseString('return a+b')
# return is first, expr can get return as identifier!!!
exprstmt = funccall | andexpr
def exprstmt_action(tokens):
    print "pop"
    return tokens
stmt = (returnexpr | exprstmt)
stmts = delimitedList(stmt, Literal("\n").suppress())
defparams = Literal('(').suppress() + delimitedList(Word(), Literal(",").suppress()) + Literal(")").suppress()
funcdef = Word() + defparams + Literal('->\n') + stmts
funcdef.parseString(
"""func1(a,b) ->
func2(c)
return a+b"""
)
