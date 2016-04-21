# constants int str 
opmap = {
    "str": 1,
####
    "pop": 10,
    "dup": 11,
    "return": 12,
    "swp": 13,
    "nop": 14,
    "class": 15,
    "setitem": 16,
    "getmethod": 17,
    "setfield": 18,
    "getfield": 19,
####
    "call": 20,
    "pushlocal": 21,
    "pushglobal": 22,
    "setlocal": 23,
    "setglobal": 24,
    "jmp": 25,
    "pop_trap_jmp": 26,
    "loop": 27,
    "trap": 28,
    "onerr": 29,
    "jnt": 30,
    "pushconst": 31,
    "build_list": 32,
    "build_dict": 33,
    "int": 34,
####
    "import": 50,
####
    "function": 61,
}
CONST = 1
HAVE_0 = 10
HAVE_1 = 20
HAVE_2 = 50
HAVE_3 = 60
def to_bytecode(code):
    bytecode = bytearray()
    lines = code.split("\n")[:-1]
    for line in lines:
        atoms = line.split(" ") if not line.startswith("str") else ["str", line[4:]]
        op = opmap[atoms[0]]
        bytecode.append(op)
        if line.startswith("str"):
            bytecode.extend(line[4:])
            bytecode.append(0)
        elif line.startswith("int"):
            int_val = int(line[4:])
            assert int_val < (1 << 32)
            if int_val < 0:
                int_val = (1 << 32) + int_arg
            bytecode.append(int_val & (255))
            bytecode.append((int_val & (255 << 8)) >> 8)
            bytecode.append((int_val & (255 << 16)) >> 16)
            bytecode.append((int_val & (255 << 24)) >> 24)
        else:
            for arg in atoms[1:]:
                int_val = int(arg)
                if int_val < 0:
                    int_val = (1<<16) + int_val
                assert int_val < (1 << 16)
                bytecode.append(int_val % 256)
                bytecode.append(int_val >> 8)
    return bytecode

def getint(val):
    if val > (1<<15):
        return val - (1<<16)
    return val

def get32int(val):
    if val > (1<<31):
        return val - (1<<32)
    return val

def to_code(bytecode):
    reverse_opmap = {v: k for k, v in opmap.iteritems()}
    code = ""
    i = 0
    while i < len(bytecode):
        opt = bytecode[i]
        i += 1
        args = []
        if opt == opmap["str"]:
            j = i
            while bytecode[j] != 0:
                j += 1
            args = [str(bytecode[i:j])]
            i = j + 1
        elif opt == opmap["int"]:
            args = get32int(
                bytecode[i] + (bytecode[i+1] << 8) +
                (bytecode[i] << 16) + (bytecode[i+1] << 24)
            )
            i += 4
        else:
            if opt >= HAVE_1:
                args.append(getint(bytecode[i] + (bytecode[i+1] << 8)))
                i+= 2
            if opt >= HAVE_2:
                args.append(getint(bytecode[i] + (bytecode[i+1] << 8)))
                i+= 2
            if opt >= HAVE_3:
                args.append(getint(bytecode[i] + (bytecode[i+1] << 8)))
                i+= 2
        code += " ".join([reverse_opmap[opt]] + [str(arg) for arg in args]) + "\n"
    return code
