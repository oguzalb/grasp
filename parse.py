class ParseError(Exception):
    pass

# TODO not needed as it seems
def get_while(chars, text, i):
    w = []
    length = len(text)
    if i >= length or text[i] not in chars:
        raise ParseError(i)
    while i < length and text[i] in chars:
        w.append(text[i])
        i += 1
    return ''.join(w), i

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

class Token():
    def parseString(self, text):
        print "string: %s" % text
        tokens, i = self.parse(text, 0)
        if len(text) != i:
            raise ParseError("Not finished %s, rest: %s" % (i, text[i:]))
        return tokens
    def __or__(self, token):
        return Or(self, token)
    def __add__(self, token):
        return And(self, token)
    def process(self, results, i):
        if hasattr(self, 'suppressed'):
            return [], i
        elif hasattr(self, 'action'):
            return self.action(results), i
        return results, i
    def set_action(self, action):
        self.action = action
    def suppress(self):
        self.suppressed = True
        return self

class And(Token):
    def __init__(self, token, token2):
        self.patterns = [token, token2]
    def __add__(self, pattern):
        self.patterns.append(pattern)
        return self
    def parse(self, text, i):
        results = []
        for pattern in self.patterns:
            result, i = pattern.parse(text, i)
            results.extend(result)
        return self.process(results, i)

class Or(Token):
    def __init__(self, token, token2):
        self.alternatives = [token, token2]
    def __or__(self, alternative):
        self.alternatives.append(alternative)
        return self
    def parse(self, text, i):
        for alternative in self.alternatives:
            try:
                alt_result, i = alternative.parse(text, i)
                result = alt_result
                return self.process(result, i)
            except ParseError as e:
                pass
        raise ParseError("None of alternatives %s. rest: |%s|" % (i, text[i:]))


import re
class Regex(Token):
    def __init__(self, pattern):
        self.pattern = re.compile(pattern, 0)
    def parse(self, text, i):
        i = pass_space(text, i)
        match = self.pattern.match(text, i)
        if not match:
            raise ParseError("not matched %s, rest:|%s|" % (i, text[i:]))
        i = match.end()
        i = pass_space(text, i)
        result = [match.group()]
        return self.process(result, i)

class Forward(Token):
# TODO process should happen
    def parse(self, text, i):
        return self.token.parse(text, i)
    def __lshift__(self, token):
        self.token = token

class IndentedBlock(Token):
    indents = [-1]
    def __init__(self, stmt):
        self.stmt = stmt
    def parse(self, text, i):
        block = []
        stmt_start = pass_indent_space(text, i)
        first_indent = stmt_start - i
        indent = first_indent
        self.indents.append(first_indent)
        while self.indents[-1] == indent and indent > self.indents[-2] and i < len(text):
            i = stmt_start
            tokens, i = self.stmt.parse(text, i)
            block.append(tokens)
            stmt_start = pass_indent_space(text, i)
            indent = stmt_start - i
        self.indents.pop()
        return self.process(block, i)

class Literal(Token):
    def __init__(self, literal):
        self.literal = literal
    def parse(self, text, i):
        i = pass_space(text, i)
        if self.literal != text[i:i+len(self.literal)]:
            raise ParseError("not matched %s |%s| rest:|%s|" % (i, self.literal, text[i:]))
        i += len(self.literal)
        i = pass_space(text, i)
        result = [self.literal]
        return self.process(result, i)

class Infix(Token):
    def __init__(self, operand, operator):
        self.operator = operator
        self.operand = operand
    def parse(self, text, i):
        results = []
        result, i = self.operand.parse(text, i)
        if len(result) == 1:
            results.extend(result)
        else:
            results.append(result)
        while True:
            try:
                result, i = self.operator.parse(text, i)
                results.extend(result)
            except ParseError as e:
                return results[0] if len(results) == 1 else results, i
            result, i = self.operand.parse(text, i)
            if len(result) == 1:
                results.extend(result)
            else:
                results.append(result)
            results, i = self.process(results, i)
            results = [results]


class delimitedList(Token):
    def __init__(self, delimited, delimiter):
        self.delimiter = delimiter
        self.delimited = delimited
    def parse(self, text, i):
        results = []
        result, i = self.delimited.parse(text, i)
        results.extend(result)
        while True:
            try:
                result, i = self.delimiter.parse(text, i)
                results.extend(result)
            except ParseError as e:
                return self.process(results, i)
            result, i = self.delimited.parse(text, i)
            results.extend(result)

dblQuotedString = Regex(r'"(?:[^"\n\r\\]|(?:"")|(?:\\x[0-9a-fA-F]+)|(?:\\.))*"')
sglQuotedString = Regex(r"'(?:[^'\n\r\\]|(?:'')|(?:\\x[0-9a-fA-F]+)|(?:\\.))*'")
quotedString = Regex(r'''(?:"(?:[^"\n\r\\]|(?:"")|(?:\\x[0-9a-fA-F]+)|(?:\\.))*")|(?:'(?:[^'\n\r\\]|(?:'')|(?:\\x[0-9a-fA-F]+)|(?:\\.))*')''')

class Word(Regex):
    def __init__(self):
        Regex.__init__(self, "[a-zA-Z_]\w*")

class Group(Token):
    def __init__(self, token):
        self.token = token
    def parse(self, text, i):
        results, i = self.token.parse(text, i)
        return self.process([results], i)
