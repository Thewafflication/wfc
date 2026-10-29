"""Differential fuzzer: random VB programs, interpreter versus native code.

usage: python tools/fuzz_native_vs_interpreter.py <wfc.exe> <count> [seed]

A program that differs is kept as fuzz_fail_<seed>_<n>.bas next to the current
directory. Do not rebuild wfc.exe while it runs.
"""
import os
import random
import subprocess
import sys
import tempfile

WFC = sys.argv[1]
COUNT = int(sys.argv[2])
SEED = int(sys.argv[3]) if len(sys.argv) > 3 else 1
rnd = random.Random(SEED)

LONGS = ["a", "b", "c"]
INTS = ["i1", "i2"]
DOUBLES = ["d1", "d2"]
STRINGS = ["s1", "s2"]
VARIANTS = ["v1", "v2"]


def lit_long():
    return str(rnd.choice([0, 1, 2, 3, 7, 10, -1, -5, 100, 255, 1000, 32767,
                           32768, 65535, 100000, 2147483647, -2147483647]))


def lit_double():
    return rnd.choice(["0.5", "1.5", "-2.25", "3.14159", "100.125", "1E10",
                       "0.1", "2.5", "-0.75", "7.0"])


def lit_string():
    return '"' + rnd.choice(["", "a", "abc", "Hello", "  pad ", "12", "3.5",
                             "-7", "&H1F", "xyz123", "A,B", "Zebra"]) + '"'


def num(depth=0):
    """A numeric expression."""
    r = rnd.random()
    if depth > 2 or r < 0.3:
        return rnd.choice([lit_long, lit_double, lambda: rnd.choice(LONGS),
                           lambda: rnd.choice(INTS),
                           lambda: rnd.choice(DOUBLES),
                           lambda: rnd.choice(VARIANTS),
                           lambda: "arr(%d)" % rnd.randint(0, 5),
                           lambda: "Twice(%s)" % rnd.choice(LONGS),
                           lambda: "Scale(%s, %s)" % (rnd.choice(LONGS), rnd.choice(DOUBLES))])()
    op = rnd.choice(["+", "-", "*", "/", "\\", "Mod", "^", "fn", "fn",
                     "cmp", "logic", "neg"])
    if op in "+-*/\\^" or op == "Mod":
        return "(%s %s %s)" % (num(depth + 1), op, num(depth + 1))
    if op == "neg":
        return "(-%s)" % num(depth + 1)
    if op == "cmp":
        return "(%s %s %s)" % (num(depth + 1),
                               rnd.choice(["<", ">", "=", "<>", "<=", ">="]),
                               num(depth + 1))
    if op == "logic":
        return "(%s %s %s)" % (rnd.choice(["Not", ""]) and num(depth + 1) or num(depth + 1),
                               rnd.choice(["And", "Or", "Xor"]), num(depth + 1))
    if rnd.random() < 0.15:
        k = rnd.choice(["like", "wd", "dd", "ubound", "instrrev", "cmp"])
        if k == "like":
            return "(%s Like %s)" % (strexp(depth + 1), rnd.choice(['"a*"', '"?b?"', '"*[0-9]"', '"[A-Z]*"', '"H*o"', '"#"']))
        if k == "wd":
            return "Weekday(DateSerial(%d, %d, %d))" % (rnd.choice([1999, 2000, 2024]), rnd.randint(1, 12), rnd.randint(1, 28))
        if k == "dd":
            return 'DateDiff("%s", DateSerial(2000, 1, %d), DateSerial(2024, %d, %d))' % (rnd.choice(["d", "m", "yyyy", "ww"]), rnd.randint(1, 28), rnd.randint(1, 12), rnd.randint(1, 28))
        if k == "ubound":
            return "UBound(Split(%s, %s))" % (strexp(depth + 1), rnd.choice(['","', '" "']))
        if k == "instrrev":
            return "InStrRev(%s, %s)" % (strexp(depth + 1), strexp(depth + 1))
        return "StrComp(%s, %s)" % (strexp(depth + 1), strexp(depth + 1))
    f = rnd.choice(["Abs", "Int", "Fix", "Sgn", "CLng", "CInt", "CDbl", "Len",
                    "Asc", "Val", "InStr", "Sqr", "Round", "CSng"])
    if f == "Len":
        return "Len(%s)" % strexp(depth + 1)
    if f == "Asc":
        return 'Asc(%s & "x")' % strexp(depth + 1)
    if f == "Val":
        return "Val(%s)" % strexp(depth + 1)
    if f == "InStr":
        return "InStr(%s, %s)" % (strexp(depth + 1), strexp(depth + 1))
    if f == "Sqr":
        return "Sqr(Abs(%s))" % num(depth + 1)
    if f == "Round":
        return "Round(%s)" % num(depth + 1)
    return "%s(%s)" % (f, num(depth + 1))


def strexp(depth=0):
    r = rnd.random()
    if depth > 2 or r < 0.3:
        return rnd.choice([lit_string, lambda: rnd.choice(STRINGS),
                           lambda: "Pad(%s)" % rnd.choice(LONGS)])()
    op = rnd.choice(["&", "&", "fn", "fn", "fn"])
    if op == "&":
        return "(%s & %s)" % (strexp(depth + 1), strexp(depth + 1))
    extra = rnd.random()
    if extra < 0.25:
        k = rnd.choice(["rep", "rev", "join", "date", "dateadd", "fmtd", "strc", "lset"])
        if k == "rep":
            return "Replace(%s, %s, %s)" % (strexp(depth + 1), rnd.choice(['"a"', '"l"', '"abc"', '""', '" "']), rnd.choice(['"X"', '""', '"yy"']))
        if k == "rev":
            return "StrReverse(%s)" % strexp(depth + 1)
        if k == "join":
            return 'Join(Split(%s, %s), %s)' % (strexp(depth + 1), rnd.choice(['","', '" "', '"a"']), rnd.choice(['"-"', '"+"', '""']))
        if k == "date":
            return 'Format$(DateSerial(%d, %d, %d), "%s")' % (rnd.choice([1999, 2000, 2024, 1900]), rnd.randint(1, 14), rnd.randint(0, 33), rnd.choice(["yyyy-mm-dd", "dd/mm/yy", "dddd", "mmmm d, yyyy", "m/d"]))
        if k == "dateadd":
            return 'Format$(DateAdd("%s", %d, DateSerial(2024, 2, 28)), "yyyy-mm-dd")' % (rnd.choice(["d", "m", "yyyy", "ww", "q"]), rnd.randint(-40, 400))
        if k == "fmtd":
            return 'Format$(%s, "%s")' % (rnd.choice(["1234.5678", "-0.5", "0", "1E6", "255"]), rnd.choice(["0.00", "#,##0.0", "000000", "0%", "General Number", "Currency", "Scientific", "Yes/No", "On/Off", "True/False"]))
        if k == "strc":
            return "StrConv(%s, %s)" % (strexp(depth + 1), rnd.choice(["vbUpperCase", "vbLowerCase", "vbProperCase"]))
        return 'Left$(%s & Space$(5), 4)' % strexp(depth + 1)
    f = rnd.choice(["Left$", "Right$", "Mid$", "UCase$", "LCase$", "Trim$",
                    "Str$", "CStr", "Chr$", "Space$", "String$", "Hex$", "Oct$",
                    "LTrim$", "RTrim$", "Format$"])
    if f in ("Left$", "Right$"):
        return "%s(%s, %s)" % (f, strexp(depth + 1), rnd.choice(["0", "1", "2", "3", "10", "a"]))
    if f == "Mid$":
        return "Mid$(%s, %s, %s)" % (strexp(depth + 1), rnd.choice(["1", "2", "3", "0", "10"]), rnd.choice(["1", "2", "5"]))
    if f in ("Str$", "CStr", "Hex$", "Oct$"):
        return "%s(%s)" % (f, num(depth + 1))
    if f == "Chr$":
        return "Chr$(%s)" % rnd.choice(["65", "97", "48", "32", "126", "200"])
    if f == "Space$":
        return "Space$(%s)" % rnd.choice(["0", "1", "3"])
    if f == "String$":
        return "String$(%s, %s)" % (rnd.choice(["0", "2", "4"]), rnd.choice(['"ab"', "65"]))
    if f == "Format$":
        return 'Format$(%s, "%s")' % (num(depth + 1), rnd.choice(["0.00", "000", "#,##0", "0.0E+00", "Fixed", "Percent"]))
    return "%s(%s)" % (f, strexp(depth + 1))


def statement(indent, depth=0):
    pad = "    " * indent
    r = rnd.random()
    if depth > 1 or r < 0.5:
        k = rnd.random()
        if k < 0.35:
            return [pad + "%s = %s" % (rnd.choice(LONGS), num())]
        if k < 0.5:
            return [pad + "%s = %s" % (rnd.choice(INTS), num())]
        if k < 0.65:
            return [pad + "%s = %s" % (rnd.choice(DOUBLES), num())]
        if k < 0.78:
            return [pad + "%s = %s" % (rnd.choice(STRINGS), strexp())]
        if k < 0.84:
            return [pad + "%s = %s" % (rnd.choice(VARIANTS), rnd.choice([num, strexp, lit_long, lit_string, lit_double])())]
        if k < 0.87:
            return [pad + "arr(%d) = %s" % (rnd.randint(0, 5), num())]
        if k < 0.89:
            return [pad + "Bump %s, %s" % (rnd.choice(LONGS), num())]
        if k < 0.93:
            return [pad + "Print %s" % num()]
        return [pad + "Print %s" % strexp()]
    k = rnd.random()
    if k < 0.3:
        body = statement(indent + 1, depth + 1)
        other = statement(indent + 1, depth + 1)
        return ([pad + "If %s Then" % num()] + body + [pad + "Else"] + other + [pad + "End If"])
    if k < 0.55:
        v = rnd.choice(LONGS)
        return ([pad + "For %s = %s To %s" % (v, rnd.choice(["1", "0", "5", "-2"]), rnd.choice(["3", "5", "0"]))] +
                statement(indent + 1, depth + 1) + [pad + "Next %s" % v])
    if k < 0.75:
        return ([pad + "Select Case %s" % num(), pad + "Case 0, 1"] + statement(indent + 1, depth + 1) +
                [pad + "Case 2 To 10"] + statement(indent + 1, depth + 1) +
                [pad + "Case Else"] + statement(indent + 1, depth + 1) + [pad + "End Select"])
    v = rnd.choice(LONGS)
    return ([pad + "Do While %s < 3 And %s > -10" % (v, v), pad + "    %s = %s + 1" % (v, v)] +
            statement(indent + 1, depth + 1) + [pad + "Loop"])


def program():
    lines = ["Sub Main()",
             "    Dim a As Long, b As Long, c As Long",
             "    Dim i1 As Integer, i2 As Integer",
             "    Dim d1 As Double, d2 As Double",
             "    Dim s1 As String, s2 As String",
             "    Dim v1 As Variant, v2 As Variant",
             "    Dim arr(5) As Long",
             "    a = 3: b = -4: c = 0: i1 = 7: i2 = 2: d1 = 1.5: d2 = -0.25",
             '    s1 = "abc": s2 = "Hello"',
             "    On Error Resume Next"]
    for _ in range(rnd.randint(4, 14)):
        lines.extend(statement(1))
        lines.append("    If Err.Number <> 0 Then Print \"E\"; Err.Number: Err.Clear")
    lines.append('    Print a; b; c; i1; i2; d1; d2; s1; s2')
    lines.append('    Print v1; v2; arr(0); arr(1); arr(5)')
    lines.append("End Sub")
    lines.append("Function Twice(x As Long) As Long")
    lines.append("    Twice = x * 2")
    lines.append("End Function")
    lines.append("Function Scale(x As Long, f As Double) As Double")
    lines.append("    Scale = x * f")
    lines.append("End Function")
    lines.append("Function Pad(n As Long) As String")
    lines.append('    Pad = Right$("00" & n, 3)')
    lines.append("End Function")
    lines.append("Sub Bump(ByRef n As Long, ByVal by As Variant)")
    lines.append("    n = n + 1")
    lines.append("    by = by + 1")
    lines.append("End Sub")
    return "\r\n".join(lines) + "\r\n"


REASONS = {}


def run(mode, path):
    try:
        r = subprocess.run([WFC, mode, path], capture_output=True, text=True,
                           timeout=10, stdin=subprocess.DEVNULL)
    except subprocess.TimeoutExpired:
        return ("timeout", "")
    if r.returncode == 3:
        lines = [l for l in r.stderr.strip().split("\n") if l.strip()]
        key = lines[-1].strip()[-90:] if lines else "?"
        REASONS[key] = REASONS.get(key, 0) + 1
    out = r.stdout.replace("\r\n", "\n")
    return (r.returncode, "\n".join(l.rstrip() for l in out.split("\n") if l.strip()))


diffs = 0
skipped = 0
work = tempfile.mkdtemp(prefix="fuzz")
for n in range(COUNT):
    text = program()
    path = os.path.join(work, "f%d.bas" % n)
    with open(path, "w", newline="") as f:
        f.write(text)
    i = run("--interpret", path)
    nat = run("--native", path)
    if nat[0] == 3:
        skipped += 1
        continue
    if i != nat:
        diffs += 1
        keep = os.path.abspath("fuzz_fail_%d_%d.bas" % (SEED, n))
        with open(keep, "w", newline="") as f:
            f.write(text)
        print("DIFF", keep, i[0], nat[0])
        a, b = i[1].split("\n"), nat[1].split("\n")
        for x, y in zip(a, b):
            if x != y:
                print("   I:", x[:100])
                print("   N:", y[:100])
                break
        else:
            print("   lengths", len(a), len(b))
print("%d programs, %d not compilable, %d differ" % (COUNT, skipped, diffs))
for k, v in sorted(REASONS.items(), key=lambda kv: -kv[1])[:12]:
    print("  ", v, k)
