import unittest
from parse import Word, And, Infix, Literal, Optional
class Tests(unittest.TestCase):
    def test_and(self):
        actions = []
        def and_action(parser, tokens):
            actions.append(tokens)
            return tokens
        hede = Word() + Word()
        hede.set_action(and_action)
        hede.parseString({}, 'a b')
        self.assertEquals(actions, [['a', 'b']])
    def test_infix(self):
        actions = []
        def infix_action(parser, tokens):
            actions.append(tokens)
            return tokens
        var = Word()
        def var_action(parser, tokens):
            actions.extend(tokens)
            return tokens[0]
        var.set_action(var_action)
        hede = Infix(var, Literal("+"))
        hede.set_action(infix_action)
        hede.parseString({}, 'a + b')
        self.assertEquals(actions, ['a', 'b', ['a', '+', 'b']])
    def test_optional_and(self):
        varname = Word()
        actions = []
        def varname_action(parser, tokens):
            actions.append(tokens)
            return tokens
        varname.set_action(varname_action)
        trailer = varname + Optional(Word())
        def trailer_action(parser, tokens):
            actions.append(tokens)
            return tokens
        trailer.set_action(trailer_action)
        print trailer.parseString({}, "var")
        self.assertEquals(actions, [['var'], ['var', None]])
        actions = []
        print trailer.parseString({}, "var var2")
        print "sadasd" + str(actions)
if __name__ == "__main__":
    unittest.main()
