import sys
import re
from parser.parse import (
    Word, quotedString, DelimitedList, Infix, Literal,
    IndentedBlock, Forward, Optional, Regex, Group,
    PostfixWithoutLast, Atom, Token, ParseError)
from StringIO import StringIO


class Parser():
    def __init__(self):
        self.code = StringIO()
        self.contexts = [{}]
        self.local_var_counts = []
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
        return "code:\n%s" % self.dumpcode()

    def convert_labels(self, code):
        labels = {}
        code_lines = code.split('\n')
        # TODO workaround
        assert code_lines[-1] == ''
        del code_lines[-1]
        new_code_lines = []
        for i, c in enumerate(code_lines):
            if re.search("^\w+:", c):
                label = c[:c.index(":")]
                labels[label] = i
                new_code_lines.append(c[c.index(":")+1:])
            else:
                new_code_lines.append(c)
        code_lines = new_code_lines
        label_indexes = {
            "jnt": 1,
            "jmp": 1,
            "loop": 1,
            "function": 1,
            "trap": 1,
            "onerr": 1,
            "pop_trap_jmp": 1
        }
        new_code_lines = []
        for i, line in enumerate(code_lines):
            instruction = line.split()[0]
            if instruction in label_indexes:
                index = label_indexes[instruction]
                line_code = line.split()
                line_code[index] = str(labels[line_code[index]] - i)
                new_code_lines.append(" ".join(line_code))
            else:
                new_code_lines.append(line)
        return '\n'.join(new_code_lines) + '\n' if len(
            new_code_lines) > 0 else ''

    def dumpcode(self):
        code = self.code.getvalue() + ("" if self.next_label is None
                                       else self.next_label + ":nop\n")
        return self.convert_labels(code)

    def push_context(self):
        self.contexts.append({})

    def pop_context(self):
        self.local_var_counts.append(
            sum(1 for v in self.contexts[-1].values() if v != -1))
        return self.contexts.pop()

    def pop_local_var_count(self):
        return self.local_var_counts.pop()

    def add_var(self, varname):
        context = self.contexts[-1]
        if len(self.contexts) > 1 and varname not in context:
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
    parser.add_instruction("str %s" % tokens[0][1:-1])
    return tokens
string.set_action(string_action)
number = Regex("\w+")


def number_action(parser, tokens):
    parser.add_instruction("int %s" % tokens[0])
    return tokens
number.set_action(number_action)
atom = varname | string | number
andexpr_container = Forward()
callparams = (
    Literal('(') +
    Optional(DelimitedList(andexpr_container, Literal(","))) +
    Literal(")"))
funccall = Group(callparams)
methodcall = Group(callparams)


def infix_process(name):
    def expr_process(children, parser):
        if len(children) < 3:
            return [c.process(c, parser) for c in children]
        results = [
            children[0].process(children[0], parser)
            if hasattr(children[0], "process") else None]
        parser.add_instruction("str __%s__" % name)
        parser.add_instruction("getmethod")
        results.append(
            children[2].process(children[2], parser)
            if hasattr(children[2], "process") else None)
        parser.add_instruction("call 2")
        return results
    return expr_process


def funccall_action(parser, tokens):
    param_count = tokens[0][1]
    parser.add_instruction("call %s" % (
        len(param_count) if param_count else 0))
    return tokens
funccall.set_action(funccall_action)


def methodcall_action(parser, tokens):
    param_count = tokens[0][1]
    parser.add_instruction("call %s" % (
        len(param_count) + 1 if param_count else 1))
    return tokens
methodcall.set_action(methodcall_action)
access_op = Literal(".")


def accessor_action(parser, tokens):
    parser.add_instruction("str " + tokens[0][1])
    parser.add_instruction("getfield")
    return tokens
fieldname = Word()
accessor = PostfixWithoutLast(access_op + fieldname)
accessor.set_action(accessor_action)
last_accessor = access_op + fieldname
last_accessor_call = access_op + fieldname + methodcall
trailerwithcall = last_accessor_call
trailerwithoutcall = Group(last_accessor)

getitem = (
    Literal('[') + andexpr_container + Literal(']')
)


def getitem_process(children, parser):
    parser.add_instruction("str __getitem__")
    parser.add_instruction("getmethod")
    results = [child.process(child, parser) for child in children[0]]
    parser.add_instruction("call 2")
    return results

getitem.process = getitem_process
functionmethodcall = (trailerwithcall | trailerwithoutcall | funccall)
# TODO should be fixed using postfix or sth
trailer = atom + Optional(accessor) + Optional(
    functionmethodcall | getitem
)


def trailerwithcall_process(children, parser):
    parser.add_instruction("str " + children[0][1][0])
    parser.add_instruction("getmethod")
    return [child.process(child, parser) for child in children[0]]

trailerwithcall.process = trailerwithcall_process
trailerwithoutcall.set_action(accessor_action)
divexpr = Infix(trailer, Literal('/'))
divexpr.process = infix_process("div")
mulexpr = Infix(divexpr, Literal('*'))
mulexpr.process = infix_process("mul")
modexpr = Infix(mulexpr, Literal('%'))
modexpr.process = infix_process("mod")
subexpr = Infix(modexpr, Literal('-'))
subexpr.process = infix_process("sub")
addexpr = Infix(subexpr, Literal('+'))
addexpr.process = infix_process("add")
equalsexpr = Infix(addexpr, Literal("=="))
equalsexpr.process = infix_process("equals")
orexpr = Infix(equalsexpr, Literal('or'))
orexpr.process = infix_process("or")
andexpr = Infix(orexpr, Literal('and'))
andexpr_container << andexpr
andexpr.process = infix_process('and')

# TODO return is not expr!!
returnexpr = Literal('return') + andexpr


def return_action(parser, tokens):
    parser.add_instruction("return")
    return tokens
returnexpr.set_action(return_action)

importstmt = Literal('from') + Word() + Literal('import') + Word()


def import_action(parser, tokens):
    parser.add_instruction("import %s %s" % (tokens[1], tokens[3]))
    return tokens
importstmt.set_action(import_action)

raisestmt = Literal('raise') + andexpr


def raisestmt_action(parser, tokens):
    parser.add_instruction("raise")
    return tokens
raisestmt.set_action(raisestmt_action)

exprstmt = Group(andexpr)


def exprstmt_action(parser, tokens):
    parser.add_instruction("pop")
    return tokens
funcdef = Forward()
# return is first, expr can get return as identifier!!!
primitivestmt = (
    returnexpr | importstmt |
    raisestmt | exprstmt) + Literal("\n")
exprstmt.set_action(exprstmt_action)
rightvalue = Group(andexpr)
setfield = access_op + fieldname


def lval_action(parser, tokens):
    # TODO fix here!!!
    if len(parser.contexts) > 0:
        parser.add_var(tokens[0])
    return tokens
lval = Word()
lval.set_action(lval_action)
simpleassgmt = lval + Literal("=") + Group(andexpr) + Literal("\n")


def simpleassgmt_action(parser, tokens):
    var_index = parser.get_var(tokens[0])
    if var_index != -1:
        parser.add_instruction("setlocal %s" % var_index)
    else:
        parser.add_instruction("setglobal %s" % tokens[0])
    return tokens
simpleassgmt.set_action(simpleassgmt_action)

fieldassgmt = (
    varname + accessor + setfield +
    Literal("=") + Group(andexpr) + Literal("\n"))


def fieldassgmt_action(parser, tokens):
    parser.add_instruction("str " + tokens[2][1])
    parser.add_instruction("swp")
    parser.add_instruction("setfield")
    return tokens
fieldassgmt.set_action(fieldassgmt_action)
stmt = Forward()


class IfAtom(Atom):
    def process(self, children, parser):
        ifend = parser.new_label()
        for ifpart in self[0]:
            ifpart.process(ifpart, parser, ifend)
        parser.set_next_label(ifend)
        if len(self) > 1:
            self[1].process(self[0], parser)
        return self


class IfPart(Atom):
    def process(self, children, parser, ifend):
        partend = parser.new_label()
        self[0].process(self[0], parser)
        parser.add_instruction("jnt %s" % partend)
        self[1].process(self[0], parser)
        parser.add_instruction("jmp %s" % ifend)
        parser.set_next_label(partend)
        return self


class IfStmt(Token):
    def parse(self, text, i):
        # not a nice solution, will have a look later
        if_indent = IndentedBlock.indents[-1]

        def parse_ifpart(tag, text, i):
            # space? solved but is it nice?
            try:
                _, i = Regex("(%s) " % tag).parse(text, i)
            except ParseError:
                return None, i
            ifpart = IfPart()
            ifexpr, i = Group(andexpr).parse(text, i)
            ifpart.append(ifexpr)
            _, i = Literal("\n").parse(text, i)
            block, i = IndentedBlock(stmt).parse(text, i)
            ifpart.append(block)
            return ifpart, i

        def parse_else(text, i):
            indent, new_i = Regex("\s*").parse(text, i)
            if new_i - i != if_indent:
                return None, i
            try:
                _, i = Regex("else\n").parse(text, new_i)
            except ParseError:
                return None, i
            try:
                elsepart, i = IndentedBlock(stmt).parse(text, i)
            except Exception as e:
                raise Exception(e)
            return elsepart, i
        results = IfAtom()
        ifpart, i = parse_ifpart("if", text, i)
        if ifpart is None:
            raise ParseError("not if", i)
        ifparts = [ifpart]
        while True:
            indent, new_i = Regex("\s*").parse(text, i)
            if new_i - i != if_indent:
                break
            ifpart, new_i = parse_ifpart("elif", text, new_i)
            if ifpart is None:
                break
            i = new_i
            ifparts.append(ifpart)
        results.append(ifparts)
        elsepart, i = parse_else(text, i)
        if elsepart is not None:
            results.append(elsepart)
        return results, i

ifstmt = IfStmt()
forstmt = (Literal("for") + Word() + Literal('in') +
           Group(andexpr) + Literal('\n') + IndentedBlock(stmt))


def forstmt_process(children, parser):
    startlabel = parser.new_label()
    children[0][3].process(children[0][3], parser)
    parser.add_instruction("str iter")
    parser.add_instruction("getmethod")
    parser.add_instruction("call 1")
    endlabel = parser.new_label()
    parser.set_next_label(startlabel)
    parser.add_instruction("loop %s" % endlabel)
    forvar = children[0][1]
    parser.add_var(forvar[0])
    simpleassgmt_action(parser, forvar)
    children[0][5].process(children[0][5], parser)
    parser.add_instruction("jmp %s" % startlabel)
    parser.set_next_label(endlabel)
    parser.add_instruction("pop")
    return []
forstmt.process = forstmt_process


def funcdef_action(parser, tokens):
    parser.add_instruction("pushglobal None")
    parser.add_instruction("return")
    return tokens
funcdef.set_action(funcdef_action)

namedfuncdef = Group(funcdef)


def namedfuncdef_action(parser, tokens):
    parser.pop_context()
    # TODO this return value issue should be fixed on parse lib
    parser.set_next_label(tokens[0][0][2])
    parser.add_instruction(
        "function %s %s" % (
            tokens[0][0][1],
            parser.pop_local_var_count() - len(tokens[0][1][1])))
    parser.add_instruction("setglobal %s" % tokens[0][0][0])
    return tokens
namedfuncdef.set_action(namedfuncdef_action)

classmethoddef = Group(funcdef)


def classmethoddef_action(parser, tokens):
    parser.pop_context()
    parser.set_next_label(tokens[0][0][2])
    parser.add_instruction("dup")
    parser.add_instruction("str %s" % tokens[0][0][0])
    parser.add_instruction(
        "function %s %s" % (
            tokens[0][0][1],
            parser.pop_local_var_count() - len(tokens[0][1][1])))
    parser.add_instruction("setfield")
    return tokens
classmethoddef.set_action(classmethoddef_action)

classstmt = (Literal('class') + Word() +
             Literal('\n') + IndentedBlock(classmethoddef))


def classstmt_process(children, parser):
    parser.add_instruction("class")
    parser.add_instruction("dup")
    # TODO
    parser.add_instruction("setglobal %s" % children[0][1][0])
    children[0][3].process(children[0][3], parser)
    parser.add_instruction("pop")
    return []

classstmt.process = classstmt_process

onerrstmt = (
    Literal("try") + Literal("\n") + IndentedBlock(stmt) +
    Literal("catch") + Word() + Literal('\n') +
    IndentedBlock(stmt))


def onerrstmt_process(children, parser):
    trap_end_label = parser.new_label()
    parser.add_instruction("trap %s" % trap_end_label)
    children[0][2].process(children[0][2], parser)
    onerr_end_label = parser.new_label()
    # this will be the ultimate_end_onerr_label
    parser.add_instruction("pop_trap_jmp %s" % onerr_end_label)
    parser.add_instruction("onerr %s" % onerr_end_label)
    parser.set_next_label(trap_end_label)
    children[0][6].process(children[0][6], parser)
    parser.set_next_label(onerr_end_label)
    # TODO multiple catch when catch type added
    # TODO multiple catch means each end needs to jump to ultimate end
    return []

onerrstmt.process = onerrstmt_process

stmt << (namedfuncdef | simpleassgmt | fieldassgmt |
         onerrstmt | primitivestmt |
         ifstmt | forstmt | classstmt | Regex('\s+'))


defparams = (Literal('(') +
             Optional(DelimitedList(Word(), Literal(","))) + Literal(")"))


def defparams_action(parser, tokens):
    if tokens[1] is not None:
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

funcdef << (
    funcname + defparams + Literal('->\n') +
    IndentedBlock(stmt))
main = IndentedBlock(stmt)


def printerr(msg):
    sys.stderr.write(msg)


if __name__ == "__main__":
    import argparse
    argparser = argparse.ArgumentParser(description='Compile grasp code')
    argparser.add_argument(
        'file', nargs='?', help='File to compile')
    args = argparser.parse_args()
    filename = args.file
    if filename is None:
        code = "".join(sys.stdin)
    elif filename.endswith(".grasp"):
        with open(filename, 'r') as sourcef:
            code = sourcef.read()
    else:
        printerr('Source file should end with .grasp\n')
        exit(1)
    parser = Parser()
    main.parse_string(parser, code)
    outputfilename = filename + 'o' if filename else "repl.graspo"
    print parser
    with open(outputfilename, 'w') as outputf:
        outputf.write(parser.dumpcode())
