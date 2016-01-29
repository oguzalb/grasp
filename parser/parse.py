import re


class ParseError(Exception):
    def __init__(self, message, i):
        super(ParseError, self).__init__(message)
        self.i = i


def pass_indent_space(text, i):
    # TODO any space!
    length = len(text)
    while length > i and text[i] == ' ':
        i += 1
    return i


def pass_space(text, i):
    # TODO any space!
    length = len(text)
    while length > i and text[i] == ' ':
        if length > i - 1 and text[i-1] == '\n':
            break
        i += 1
    return i


def default_action(parser, value):
    return value


class Atom(list):
    def __init__(self, single=False, process=None):
        super(Atom, self).__init__()
        if process is not None:
            self.process = process
        self.action = default_action
        self.single = single

    def set_action(self, action):
        if action is not None:
            self.action = action

    def process(self, children, parser):
        results = [c.process(self, parser)
                   if not isinstance(c, str)
                   else c for c in self]
        result = self.action(parser, results)
        if (not self.single) or len(result) > 1:
            return result
        return result[0] if len(result) else None


class Token(object):
    def __init__(self, process=None):
        self.process = process
        self.action = None

    def parse_string(self, parser, text):
        print text
        tokens, i = self.parse(text, 0)
        if len(text) != i:
            raise ParseError("Not finished %s, rest: %s" % (i, text[i:]), i)
        tokens.process(tokens, parser)
        return tokens

    def __or__(self, token):
        return Or(self, token)

    def __add__(self, token):
        return And(self, token)

    def set_action(self, action):
        self.action = action


class Action(Token):
    def __init__(self, **kwargs):
        super(Action, self).__init__(**kwargs)

    def parse(self, text, i):
        results = Atom(process=self.process)
        results.set_action(self.action)
        return results, i


class And(Token):
    def __init__(self, token, token2):
        super(And, self).__init__()
        self.patterns = [token, token2]

    def __add__(self, pattern):
        self.patterns.append(pattern)
        return self

    def parse(self, text, i):
        results = Atom(process=self.process)

        def default_action(parser, tokens):
            return tokens
        results.set_action(self.action or default_action)
        for pattern in self.patterns:
            result, i = pattern.parse(text, i)
            results.append(result)
        return results, i


class Or(Token):
    def __init__(self, token, token2):
        super(Or, self).__init__()
        self.alternatives = [token, token2]

    def __or__(self, alternative):
        self.alternatives.append(alternative)
        return self

    def parse(self, text, i):
        longest = None
        for alternative in self.alternatives:
            try:
                alt_result, i = alternative.parse(text, i)
                result = Atom(single=True)
                result.append(alt_result)
                return result, i
            except ParseError as e:
                if longest is None or e.i > longest.i:
                    longest = e
        if longest is None:
            raise ParseError("None of alternatives %s. rest: |%s|" % (
                i, text[i:]), i)
        raise longest


class Regex(Token):
    def __init__(self, pattern):
        super(Regex, self).__init__()
        self.pattern = re.compile(pattern, 0)

    def parse(self, text, i):
        i = pass_space(text, i)
        match = self.pattern.match(text, i)
        if not match:
            raise ParseError("not matched %s, rest:|%s|" % (i, text[i:]), i)
        i = match.end()
        i = pass_space(text, i)
        result = Atom(single=True, process=self.process)
        result.append(match.group())
        result.set_action(self.action)
        return result, i

    def copy(self):
        return Regex(self.pattern)


class Forward(Token):
    def __init__(self):
        super(Forward, self).__init__()
        self.copies = []

    def parse(self, text, i):
        results = Atom(single=True, process=self.process)
        results.set_action(self.action)
        result, i = self.token.parse(text, i)
        results.append(result)
        return results, i

    def __lshift__(self, token):
        self.token = token


class IndentedBlock(Token):
    # not reentrant
    indents = [-1]

    def __init__(self, stmt):
        super(IndentedBlock, self).__init__()
        self.stmt = stmt

    def parse(self, text, i):
        block = Atom(process=self.process)
        block.set_action(self.action)
        stmt_start = pass_indent_space(text, i)
        first_indent = stmt_start - i
        indent = first_indent
        self.indents.append(first_indent)
        while (self.indents[-1] == indent and
               indent > self.indents[-2] and i < len(text)):
            i = stmt_start
            tokens, i = self.stmt.parse(text, i)
            block.append(tokens)
            stmt_start = pass_indent_space(text, i)
            indent = stmt_start - i
        self.indents.pop()
        return block, i


class Literal(Token):
    def __init__(self, literal):
        super(Literal, self).__init__()
        self.literal = literal

    def parse(self, text, i):
        i = pass_space(text, i)
        if self.literal != text[i:i+len(self.literal)]:
            raise ParseError("not matched %s |%s| rest:|%s|" % (
                i, self.literal, text[i:]), i)
        i += len(self.literal)
        i = pass_space(text, i)
        result = Atom(single=True, process=self.process)
        result.set_action(self.action)
        result.extend(self.literal)
        return result, i


class LookAheadLiteral(Literal):
    def __init__(self, literal):
        super(Literal, self).__init__(literal)
        self.literal = literal

    def parse(self, text, i):
        result, _ = Literal.parse(self, text, i)
        return result, i


class Infix(Token):
    def __init__(self, operand, operator):
        super(Infix, self).__init__()
        self.operator = operator
        self.operand = operand

    def parse(self, text, i):
        results = Atom()
        result, i = self.operand.parse(text, i)
        results.append(result)
        while True:
            try:
                result, i = self.operator.parse(text, i)
                results.set_action(self.action)
                results.append(result)
            except ParseError:
                return results[0], i
            result, i = self.operand.parse(text, i)
            results.append(result)
            atom = Atom()
            atom.append(results)
            results = atom

    def copy(self):
        i = Infix(self.operand, self.operator)
        return i


class Postfix(Token):
    def __init__(self, operator):
        super(Postfix, self).__init__()
        self.operator = operator

    def parse(self, text, i):
        results = Atom(process=self.process)
        results.set_action(self.action)
        result, i = self.operator.parse(text, i)
        results.append(result)
        while True:
            atom = Atom(process=self.process)
            atom.set_action(self.action)
            atom.append(results)
            results = atom
            try:
                result, i = self.operator.parse(text, i)
                results.append(result)
            except ParseError:
                # parse error may be no alternative or sth,
                #  should be reconsidered
                return results[0], i


class PostfixWithoutLast(Token):
    def __init__(self, operator):
        super(PostfixWithoutLast, self).__init__()
        self.operator = operator

    def parse(self, text, i):
        results = Atom(process=self.process)
        results.set_action(self.action)
        indexes = [i]
        result, i = self.operator.parse(text, i)
        indexes.append(i)
        results.append(result)
        while True:
            atom = Atom(process=self.process)
            atom.set_action(self.action)
            atom.append(results)
            results = atom
            try:
                result, i = self.operator.parse(text, i)
                indexes.append(i)
                results.append(result)
            except ParseError:
                # parse error may be no alternative or sth,
                # should be reconsidered
                if len(indexes) > 2:
                    return results[0][0], indexes[-2]
                else:
                    return Atom(single=True), indexes[0]


class Optional(Token):
    def __init__(self, token):
        super(Optional, self).__init__()
        self.token = token

    def parse(self, text, i):
        try:
            result, i = self.token.parse(text, i)
            results = Atom(single=True, process=self.process)
            results.set_action(self.action)
            results.append(result)
            return results, i
        except ParseError:
            return Atom(single=True), i


class DelimitedList(Token):
    def __init__(self, delimited, delimiter):
        super(DelimitedList, self).__init__()
        self.delimiter = delimiter
        self.delimited = delimited

    def parse(self, text, i):
        results = Atom()
        result, i = self.delimited.parse(text, i)
        results.extend(result)
        while True:
            try:
                result, i = self.delimiter.parse(text, i)
            except ParseError:
                return results, i
            result, i = self.delimited.parse(text, i)
            results.extend(result)

dblQuotedString = Regex(
    r'"(?:[^"\n\r\\]|(?:"")|(?:\\x[0-9a-fA-F]+)|(?:\\.))*"')
sglQuotedString = Regex(
    r"'(?:[^'\n\r\\]|(?:'')|(?:\\x[0-9a-fA-F]+)|(?:\\.))*'")
quotedString = Regex(r'''(?:"(?:[^"\n\r\\]|(?:"")|
(?:\\x[0-9a-fA-F]+)|(?:\\.))*")|(?:'(?:[^'\n\r\\]|
(?:'')|(?:\\x[0-9a-fA-F]+)|(?:\\.))*')''')


class Word(Regex):
    def __init__(self):
        super(Word, self).__init__("[a-zA-Z_]\w*")


class Group(Token):
    def __init__(self, token):
        super(Group, self).__init__()
        self.token = token

    def parse(self, text, i):
        results = Atom(single=True, process=self.process)
        results.set_action(self.action)
        result, i = self.token.parse(text, i)
        results.append(result)
        return results, i
