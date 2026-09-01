#include "wfc/evaluator.hpp"

#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {

int failures = 0;

void expect_success(const std::string_view source, const std::string_view expected) {
    const auto result = wfc::evaluate_print_statement(source);
    if (!result.success || result.output != expected || !result.diagnostic.empty()) {
        std::cerr << "expected success for [" << source << "] but got ["
                  << result.diagnostic << "]\n";
        ++failures;
    }
}

void expect_failure(const std::string_view source, const std::string_view code) {
    const auto result = wfc::evaluate_print_statement(source);
    if (result.success || !result.output.empty() || !result.diagnostic.starts_with(code)) {
        std::cerr << "expected " << code << " for [" << source << "] but got ["
                  << result.diagnostic << "]\n";
        ++failures;
    }
}

void expect_program_success(const std::string_view source, const std::string_view expected) {
    const auto result = wfc::evaluate_program(source);
    if (!result.success || result.output != expected || !result.diagnostic.empty()) {
        std::cerr << "expected program success for [" << source << "] but got ["
                  << result.diagnostic << "]\n";
        ++failures;
    }
}

void expect_program_failure(const std::string_view source, const std::string_view code) {
    const auto result = wfc::evaluate_program(source);
    if (result.success || !result.output.empty() || !result.diagnostic.starts_with(code)) {
        std::cerr << "expected " << code << " for program [" << source << "] but got ["
                  << result.diagnostic << "]\n";
        ++failures;
    }
}

}  // namespace

int main() {
    expect_success("Print \"Hello, WFC!\"", "Hello, WFC!");
    expect_success("  pRiNt  \"case insensitive\"  \r\n", "case insensitive");
    expect_success("Print \"VB says \"\"hello\"\".\"", "VB says \"hello\".");
    expect_success("Print \"\"", "");
    expect_success("Print 42", "42");
    expect_success("Print 2 + 3 * 4", "14");
    expect_success("Print (2 + 3) * 4", "20");
    expect_success("Print -10 + +3", "-7");
    expect_success("Print -2147483648", "-2147483648");
    expect_success("Print 17 \\ 5", "3");
    expect_success("Print 17 Mod 5", "2");
    expect_success("Print 2 + 3 & \" items\"", "5 items");
    expect_success("Print \"page \" & 2 & \" of \" & (1 + 2)", "page 2 of 3");
    expect_success("Print Len(\"four\")", "4");
    expect_success("Print LCase$(\"WFC 123!\")", "wfc 123!");
    expect_success("Print UCASE(\"Wfc 123!\")", "WFC 123!");
    expect_success("Print \"[\" & LTrim$(\"  left \") & \"]\"", "[left ]");
    expect_success("Print \"[\" & RTrim(\" right  \") & \"]\"", "[ right]");
    expect_success("Print \"[\" & Trim$(\"  both  \") & \"]\"", "[both]");
    expect_success("Print Left$(\"abcdef\", 3)", "abc");
    expect_success("Print Right(\"abcdef\", 2 + 1)", "def");
    expect_success("Print Left(\"abc\", 0) & Right$(\"abc\", 99)", "abc");
    expect_success("Print Mid(\"abcdef\", 3)", "cdef");
    expect_success("Print Mid$(\"abcdef\", 2, 3)", "bcd");
    expect_success("Print \"[\" & Mid(\"abc\", 9, 2) & \"]\"", "[]");
    expect_success("Print Mid(\"abc\", 1, 99)", "abc");
    expect_success("Print Asc(\"WFC\")", "87");
    expect_success("Print Chr$(87) & Chr(70) & Chr$(67)", "WFC");
    expect_success("Print Chr$(Asc(\"A\"))", "A");
    expect_success("Print StrReverse(\"WFC 123!\")", "!321 CFW");
    expect_success("Print \"[\" & StrReverse(\"\") & \"]\"", "[]");
    expect_success("Print StrReverse(LCase(\"AbC\"))", "cba");
    expect_success("Print \"[\" & Space$(3) & \"]\"", "[   ]");
    expect_success("Print \"[\" & Space(0) & \"]\"", "[]");
    expect_success("Print String$(4, \"*\")", "****");
    expect_success("Print String(3, 61)", "===");
    expect_success("Print String$(2, \"ab\")", "aa");
    expect_success("Print Len(Space(5)) + Len(String(2, 88))", "7");
    expect_success("Print InStr(\"Hello, WFC!\", \"WFC\")", "8");
    expect_success("Print InStr(\"abcabc\", \"bc\")", "2");
    expect_success("Print InStr(3, \"abcabc\", \"bc\")", "5");
    expect_success("Print InStr(\"abc\", \"z\")", "0");
    expect_success("Print InStr(\"abc\", \"\")", "1");
    expect_success("Print InStr(\"\", \"\")", "0");
    expect_success("Print InStr(4, \"abc\", \"a\")", "0");
    expect_success("Print InStr(\"aXbXc\", \"x\")", "0");
    expect_program_success("Option Compare Text\nPrint InStr(\"aXbXc\", \"x\")", "2");
    expect_success("Print InStrRev(\"abcabc\", \"bc\")", "5");
    expect_success("Print InStrRev(\"abcabc\", \"bc\", 4)", "2");
    expect_success("Print InStrRev(\"abcabc\", \"bc\", 2)", "0");
    expect_success("Print InStrRev(\"abc\", \"\")", "3");
    expect_success("Print InStrRev(\"\", \"\")", "0");
    expect_success("Print InStrRev(\"abc\", \"a\", 4)", "0");
    expect_program_success(
        "Option Compare Binary\n"
        "Print InStrRev(\"aXbXcX\", \"x\", -1, vbTextCompare)",
        "6");
    expect_program_success("Option Compare Text\nPrint InStrRev(\"aXbXc\", \"x\")", "4");
    expect_success("Print StrComp(\"abc\", \"abc\")", "0");
    expect_success("Print StrComp(\"abc\", \"abd\")", "-1");
    expect_success("Print StrComp(\"abd\", \"abc\")", "1");
    expect_success("Print StrComp(\"abc\", \"ABC\")", "1");
    expect_success("Print StrComp(\"abc\", \"ab\")", "1");
    expect_program_success("Option Compare Text\nPrint StrComp(\"abc\", \"ABC\")", "0");
    expect_success("Print vbBinaryCompare", "0");
    expect_success("Print vbTextCompare", "1");
    expect_success("Print vbDatabaseCompare", "2");
    expect_program_success(
        "Const comparison As Long = vbTextCompare\nPrint comparison",
        "1");
    expect_success("Print InStr(1, \"aXbXc\", \"x\", vbTextCompare)", "2");
    expect_program_success(
        "Option Compare Text\n"
        "Print InStr(1, \"aXbXc\", \"x\", vbBinaryCompare)\n"
        "Print StrComp(\"abc\", \"ABC\", vbBinaryCompare)\n"
        "Print StrComp(\"abc\", \"ABC\", vbTextCompare)",
        "0\n1\n0");
    expect_success("Print Replace(\"abcabc\", \"b\", \"X\")", "aXcaXc");
    expect_success("Print Replace(\"aaa\", \"a\", \"bb\")", "bbbbbb");
    expect_success("Print Replace(\"abc\", \"z\", \"Y\")", "abc");
    expect_success("Print \"[\" & Replace(\"abc\", \"\", \"Y\") & \"]\"", "[abc]");
    expect_success("Print \"[\" & Replace(\"abab\", \"ab\", \"\") & \"]\"", "[]");
    expect_success("Print Replace(\"aXbXc\", \"x\", \"-\")", "aXbXc");
    expect_program_success("Option Compare Text\nPrint Replace(\"aXbXc\", \"x\", \"-\")", "a-b-c");
    expect_success("Print Replace(\"prefix abc abc\", \"abc\", \"X\", 8)", "X X");
    expect_success("Print Replace(\"prefix abc abc\", \"abc\", \"X\", 8, 1)", "X abc");
    expect_success("Print Replace(\"prefix abc\", \"abc\", \"X\", 8, 0)", "abc");
    expect_success("Print \"[\" & Replace(\"abc\", \"a\", \"X\", 9) & \"]\"", "[]");
    expect_success(
        "Print Replace(\"aXbXc\", \"x\", \"-\", 1, -1, vbTextCompare)",
        "a-b-c");
    expect_program_success(
        "Option Compare Text\n"
        "Print Replace(\"aXbXc\", \"x\", \"-\", 1, -1, vbBinaryCompare)",
        "aXbXc");
    expect_success("Print Hex(255)", "FF");
    expect_success("Print Hex$(0)", "0");
    expect_success("Print Hex(-1)", "FFFFFFFF");
    expect_success("Print Hex(4096)", "1000");
    expect_success("Print Oct(8)", "10");
    expect_success("Print Oct$(0)", "0");
    expect_success("Print Oct(-1)", "37777777777");
    expect_success("Print Hex(15.5) & \" \" & Hex(17.5) & \" \" & Oct(7.5)", "10 12 10");
    expect_success("Print Hex(-1.5) & \" \" & Oct(8.5)", "FFFFFFFE 10");
    expect_success("Print \"&H\" & Hex(Asc(\"A\"))", "&H41");
    expect_success("Print \"[\" & Str(42) & \"]\"", "[ 42]");
    expect_success("Print \"[\" & Str$(-42) & \"]\"", "[-42]");
    expect_success("Print \"[\" & Str(0) & \"]\"", "[ 0]");
    expect_success("Print Len(Str(100))", "4");
    expect_success("Print \"[\" & Str(2.5) & \"] [\" & Str$(-2.5) & \"]\"", "[ 2.5] [-2.5]");
    expect_success("Print \"[\" & Str(-0.0) & \"] [\" & Str(1e3) & \"]\"", "[ 0] [ 1000]");
    expect_success("Print Val(\"2457\")", "2457");
    expect_success("Print Val(\" 2 45 7th Street\")", "2457");
    expect_success("Print Val(\"- 42 trailing\")", "-42");
    expect_success("Print Val(\"+17\")", "17");
    expect_success("Print Val(\"words 42\")", "0");
    expect_success("Print Val(\"12,345\")", "12");
    expect_success("Print Val(\"   \")", "0");
    expect_success("Print Val(\"&HFF\") & \" \" & Val(\"&HFFFF\") & \" \" & Val(\"&H10000\")", "255 -1 65536");
    expect_success("Print Val(\"&O10\") & \" \" & Val(\"&O177777\")", "8 -1");
    expect_success("Print Abs(-42)", "42");
    expect_success("Print Abs(0) + Abs(7)", "7");
    expect_success("Print Sgn(-42) & \" \" & Sgn(0) & \" \" & Sgn(42)", "-1 0 1");
    expect_success("Print Abs(Sgn(-100))", "1");
    expect_success("Print Abs(-2.5) & \" \" & Sgn(-0.25) & \" \" & Sgn(0.0)", "2.5 -1 0");
    expect_success("Print TypeName(Abs(-2.5)) & \" \" & TypeName(Sgn(-2.5))", "Double Long");
    expect_success("Print \"[\" & CStr(42) & \"]\"", "[42]");
    expect_success("Print CStr(True) & \" \" & CStr(False)", "True False");
    expect_success("Print CStr(\"already text\")", "already text");
    expect_success("Print Len(CStr(-100))", "4");
    expect_success("Print CLng(42)", "42");
    expect_success("Print CLng(True) & \" \" & CLng(False)", "-1 0");
    expect_success("Print CLng(\"  -42  \")", "-42");
    expect_success("Print CLng(\"+17\")", "17");
    expect_success("Print CLng(\"2.5\") & \" \" & CLng(\"3.5\") & \" \" & CLng(\"-2.6e1\")", "2 4 -26");
    expect_success("Print CLng(CStr(123)) + 1", "124");
    expect_program_success("Print CBool(True): Print CBool(False)", "True\nFalse");
    expect_program_success("Print CBool(42): Print CBool(0): Print CBool(-1)", "True\nFalse\nTrue");
    expect_program_success("Print CBool(\"1.5\"): Print CBool(\"-2e-3\"): Print CBool(\"0.0\")", "True\nTrue\nFalse");
    expect_program_success("Print CBool(\" true \"): Print CBool(\"FALSE\")", "True\nFalse");
    expect_program_success("Print CBool(\"-2\"): Print CBool(\"+0\")", "True\nFalse");
    expect_success("Print CByte(0) & \" \" & CByte(255)", "0 255");
    expect_success("Print CByte(42) + 1", "43");
    expect_success("Print CByte(2.5) & \" \" & CByte(3.5) & \" \" & CByte(254.6)", "2 4 255");
    expect_success(
        "Print CByte(\" 2.5 \" ) & \" \" & CByte(\"+3.5\") & \" \" & CByte(\"2.546e2\")",
        "2 4 255");
    expect_success(
        "Print TypeName(CVar(42)) & \" \" & TypeName(CVar(2.5)) & \" \" & TypeName(CVar(True)) & \" \" & TypeName(CVar(\"x\"))",
        "Long Double Boolean String");
    expect_success("Print CVar(42) + 1 & \" \" & CVar(\"value\")", "43 value");
    expect_success("Print CInt(-32768) & \" \" & CInt(32767)", "-32768 32767");
    expect_success("Print CInt(True) & \" \" & CInt(False)", "-1 0");
    expect_success("Print CInt(\"  -17  \") & \" \" & CInt(\"+42\")", "-17 42");
    expect_success("Print CInt(CStr(1000)) + 1", "1001");
    expect_success("Print CInt(\"-2.5\") & \" \" & CInt(\"2.6\") & \" \" & CInt(\"3e2\")", "-2 3 300");
    expect_program_success(
        "Print IsNumeric(42): Print IsNumeric(True): Print IsNumeric(\"  -17 \")",
        "True\nTrue\nTrue");
    expect_program_success(
        "Print IsNumeric(\"\"): Print IsNumeric(\"12x\"): Print IsNumeric(\"abc\")",
        "False\nFalse\nFalse");
    expect_program_success(
        "Print IsNumeric(\" -3.14 \"): Print IsNumeric(\"+2e3\"): Print IsNumeric(\".5\")",
        "True\nTrue\nTrue");
    expect_program_success(
        "Print IsNumeric(\"1e\"): Print IsNumeric(\"NaN\"): Print IsNumeric(\"+\")",
        "False\nFalse\nFalse");
    expect_success(
        "Print TypeName(42) & \" \" & TypeName(True) & \" \" & TypeName(\"x\")",
        "Long Boolean String");
    expect_success("Print TypeName(1 = 1) & \" \" & TypeName(Len(\"ab\"))", "Boolean Long");
    expect_success(
        "Print VarType(42) & \" \" & VarType(True) & \" \" & VarType(\"x\")",
        "3 11 8");
    expect_success("Print vbLong & \" \" & vbBoolean & \" \" & vbString", "3 11 8");
    expect_success("Print VarType(\"y\") = vbString", "True");
    expect_success("Print TypeName(3.14) & \" \" & VarType(3.14)", "Double 5");
    expect_success("Print IIf(1 < 2, \"yes\", \"no\")", "yes");
    expect_success("Print IIf(1 > 2, \"yes\", \"no\")", "no");
    expect_success("Print IIf(True, 10, 20) + 1", "11");
    expect_success("Print Choose(2, \"a\", \"b\", \"c\")", "b");
    expect_success("Print Choose(1, 10, 20) + Choose(3, 1, 2, 3)", "13");
    expect_success("Print Switch(1 > 2, \"a\", 2 > 1, \"b\", True, \"c\")", "b");
    expect_success("Print Switch(False, 1, True, 42) + 1", "43");
    expect_success("Print Int(-7) & \" \" & Fix(-7) & \" \" & Int(42)", "-7 -7 42");
    expect_success("Print Int(Val(\"100\")) + Fix(5)", "105");
    expect_success("Print Int(2.5) & \" \" & Fix(2.5) & \" \" & Int(-2.5) & \" \" & Fix(-2.5)",
                   "2 2 -3 -2");
    expect_success("Print TypeName(Int(2.5)) & \" \" & TypeName(Fix(-2.5))", "Double Double");
    expect_success("Print Round(42) & \" \" & Round(-7, 0) & \" \" & Round(100, 3)",
                   "42 -7 100");
    expect_success("Print Round(2.5) & \" \" & Round(3.5) & \" \" & Round(-2.5)", "2 4 -2");
    expect_success("Print Round(1.25, 1) & \" \" & Round(1.35, 1) & \" \" & Round(-1.25, 1)",
                   "1.2 1.4 -1.2");
    expect_success("Print TypeName(Round(2.0)) & \" \" & Round(1.234567890123456, 20)",
                   "Double 1.234567890123456");
    // Double literals, arithmetic, comparison, and conversion.
    expect_success("Print 3.14", "3.14");
    expect_success("Print .5 + .25", "0.75");
    expect_success("Print 1.5 + 2.5", "4");
    expect_success("Print 5 / 2", "2.5");
    expect_success("Print 6 / 3", "2");
    expect_success("Print 2 * 1.5", "3");
    expect_success("Print -3.5", "-3.5");
    expect_success("Print 1e3", "1000");
    expect_success("Print TypeName(1#) & \" \" & VarType(1#) & \" \" & 1.25#", "Double 5 1.25");
    expect_success("Print TypeName(42&) & \" \" & VarType(42&) & \" \" & -42&", "Long 3 -42");
    expect_success("Print 2147483648#", "2147483648");
    expect_success("Print 1 & 2", "12");
    expect_success("Print 1.5 & \"!\"", "1.5!");
    expect_program_success(
        "Print 1.5 < 2: Print 3.0 = 3: Print 2.5 > 2.5",
        "True\nTrue\nFalse");
    expect_success("Print CLng(2.5) & \" \" & CLng(3.5) & \" \" & CLng(3.7)", "2 4 4");
    expect_success("Print CInt(-2.5) & \" \" & CInt(2.6)", "-2 3");
    expect_success("Print CStr(3.5) & \" \" & CStr(2.0)", "3.5 2");
    expect_program_success("Print IsNumeric(3.14): Print CBool(1.5)", "True\nTrue");
    expect_success("Print CDbl(3) & \" \" & CDbl(\"2.5\") & \" \" & CDbl(\"+1.5\")",
                   "3 2.5 1.5");
    expect_success("Print CDbl(\"  -1.5  \") + 0.5 & \" \" & CDbl(True)", "-1 -1");
    expect_success("Print CSng(2.25) + CSng(0.5)", "2.75");
    expect_success("Print CLng(CDbl(\"3.5\"))", "4");
    // Floating-point math functions.
    expect_success("Print Sqr(9) & \" \" & Sqr(0.25)", "3 0.5");
    expect_success("Print Exp(0) & \" \" & Log(1)", "1 0");
    expect_program_success("Print Sin(0): Print Cos(0): Print Tan(0)", "0\n1\n0");
    expect_success("Print CLng(Atn(1) * 4 * 1000000)", "3141593");
    expect_success("Print CLng(Log(Exp(3)))", "3");
    // Integer-division operators round Double operands (banker's rounding).
    expect_success("Print 5.0 \\ 2 & \" \" & 7.5 \\ 2 & \" \" & 5.5 Mod 2", "2 4 0");
    expect_program_failure("Print \"x\" \\ 2", "WFC0007");
    expect_success("Print AscW(\"A\") & \" \" & ChrW(66)", "65 B");
    expect_success("Print ChrW(AscW(\"z\"))", "z");
    expect_success("Print LenB(\"WFC\") & \" \" & AscB(\"A\") & \" \" & ChrB(66)",
                   "3 65 B");
    expect_success("Print AscB(ChrB(255)) & \" \" & LenB(ChrB$(0))", "255 1");
    expect_success(
        "Print LeftB(\"WFC\", 2) & \" \" & RightB$(\"WFC\", 2) & \" \" & "
        "MidB(\"WFC\", 2, 1) & \" \" & InStrB(\"WFC\", \"FC\")",
        "WF FC F 2");
    expect_success("Print AscB(LeftB(ChrB(255) & \"x\", 1))", "255");
    expect_success(
        "Print StrConv(\"aBc\", vbUpperCase) & \" \" & StrConv(\"aBc\", vbLowerCase)",
        "ABC abc");
    expect_success(
        "Print StrConv(\"the quICK fox\", vbProperCase)", "The Quick Fox");
    expect_success("Print StrConv(\"ab1cd\", 3)", "Ab1Cd");
    expect_success("Print vbTrue & \" \" & vbFalse & \" \" & vbUseDefault", "-1 0 -2");
    expect_success(
        "Print vbMethod & \" \" & vbGet & \" \" & vbLet & \" \" & vbSet",
        "1 2 4 8");
    expect_success(
        "Print vbReadOnly & \" \" & vbHidden & \" \" & vbDirectory & \" \" & vbArchive",
        "1 2 16 32");
    expect_success("Print vbYes & \" \" & vbNo & \" \" & vbCancel", "6 7 2");
    expect_success(
        "Print vbSunday & \" \" & vbSaturday & \" \" & vbUseSystemDayOfWeek",
        "1 7 0");
    expect_success(
        "Print vbYesNo & \" \" & vbCritical & \" \" & vbExclamation & \" \" & "
        "vbDefaultButton2 & \" \" & vbSystemModal",
        "4 16 48 256 4096");
    expect_success(
        "Print vbHide & \" \" & vbNormalFocus & \" \" & vbMinimizedNoFocus",
        "0 1 6");
    expect_success(
        "Print vbFirstFourDays & \" \" & vbCalHijri & \" \" & vbShortTime",
        "2 1 4");
    expect_success(
        "Print AscB(vbTab) & \" \" & AscB(vbCr) & \" \" & AscB(vbLf) & \" \" & "
        "AscB(vbNullChar)",
        "9 13 10 0");
    expect_success(
        "Print Len(vbCrLf) & \" \" & Len(vbNewLine) & \" \" & Len(vbNullString)",
        "2 2 0");
    expect_success("Print vbObjectError", "-2147221504");
    expect_success(
        "Print vbEmpty & \" \" & vbInteger & \" \" & vbDouble & \" \" & vbByte & "
        "\" \" & vbArray",
        "0 2 5 17 8192");
    expect_success("Print VarType(42) = vbLong", "True");
    expect_success("Print VarType(\"x\") = vbString", "True");
    expect_success(
        "Print vbIMEModeHangul & \" \" & vbIMEModeHangulFull & \" \" & vbIMEOn",
        "10 9 1");
    expect_program_success(
        "Print IsArray(42): Print IsObject(\"x\"): Print IsNull(0)",
        "False\nFalse\nFalse");
    expect_program_success(
        "Print IsEmpty(1): Print IsError(True): Print IsMissing(7)",
        "False\nFalse\nFalse");
    expect_success("Print RGB(255, 0, 0) & \" \" & RGB(0, 0, 255)", "255 16711680");
    expect_success("Print RGB(300, 128, 0)", "33023");
    expect_success("Print QBColor(0) & \" \" & QBColor(7) & \" \" & QBColor(15)",
                   "0 12632256 16777215");
    expect_success("Print QBColor(1) & \" \" & QBColor(12)", "8388608 255");

    expect_failure("", "WFC0001");
    expect_failure("Printer \"no\"", "WFC0001");
    expect_failure("Print", "WFC0002");
    expect_failure("Print value", "WFC0002");
    expect_failure("Print \"unterminated", "WFC0003");
    expect_failure("Print \"ok\" trailing", "WFC0004");
    expect_failure("Print (1 + 2", "WFC0005");
    expect_failure("Print 2147483648", "WFC0006");
    expect_failure("Print -2147483649", "WFC0006");
    expect_failure("Print 1.5&", "WFC0006");
    expect_failure("Print 1e3&", "WFC0006");
    expect_failure("Print \"one\" + \"two\"", "WFC0007");
    expect_failure("Print 1 \\ 0", "WFC0008");
    expect_failure("Print 2147483647 + 1", "WFC0009");

    expect_program_success(
        "Dim count As Long\r\n"
        "count = 6\r\n"
        "Let count = count * 7\r\n"
        "Print count",
        "42");
    expect_program_success(
        "dim Greeting as string: GREETING = \"Hello\": Print greeting & \"!\"",
        "Hello!");
    expect_program_success(
        "' defaults and comments\n"
        "Dim number As Long ' initialized to zero\n"
        "Dim text As String\n"
        "Print text & number\n"
        "Print number + 1",
        "0\n1");
    expect_program_success(
        "Rem module comment\n"
        "Dim value As Long: Rem statement comment\n"
        "value = 42\n"
        "rEm mixed-case comment\n"
        "Print value",
        "42");
    expect_program_success(
        "Const base As Long = 6\n"
        "Const answer As Long = base * 7\n"
        "Const greeting As String = \"Hello\" & \"!\"\n"
        "Const ready As Boolean = answer = 42\n"
        "Print answer\n"
        "Print greeting\n"
        "Print ready",
        "42\nHello!\nTrue");
    expect_program_success(
        "Dim value As Double\n"
        "Print TypeName(value) & \" \" & value\n"
        "value = 2\n"
        "value = value + 0.5\n"
        "Const scale As Double = 4\n"
        "Print value * scale\n"
        "Print VarType(scale)",
        "Double 0\n10\n5");
    expect_program_success(
        "Dim amount#\n"
        "amount = 2\n"
        "amount# = amount# + 0.5\n"
        "Dim count&\n"
        "count& = 4\n"
        "Dim label$\n"
        "label$ = \"typed\"\n"
        "Const scale# = 2\n"
        "Const suffix$ = \"!\"\n"
        "Print label & \" \" & amount * scale & suffix & \" \" & count",
        "typed 5! 4");
    expect_program_success(
        "Dim value As Double\nvalue# = 3\nPrint value# & \" \" & value",
        "3 3");
    expect_program_success(
        "' leading comment\n"
        "Rem another leading comment\n"
        "Option Explicit\n"
        "Dim value As Long\n"
        "value = 42\n"
        "Print value",
        "42");
    expect_program_success(
        "Option Compare Text\n"
        "Option Explicit\n"
        "Print \"Alpha\" = \"alpha\"\n"
        "Print \"alpha\" < \"Beta\"\n"
        "Select Case \"BETA\"\n"
        "Case \"alpha\" To \"gamma\"\n"
        "  Print \"selected\"\n"
        "End Select\n"
        "Select Case \"BETA\"\n"
        "Case Is = \"beta\"\n"
        "  Print \"equal\"\n"
        "End Select",
        "True\nTrue\nselected\nequal");
    expect_program_success(
        "Option Compare Binary\n"
        "Print \"Alpha\" = \"alpha\"",
        "False");
    expect_program_success(
        "Dim text As String\n"
        "text = \"WFC\"\n"
        "Print Len(\"Hello\")\n"
        "Print Len(text & \"!\")\n"
        "If Len(text) = 3 Then Print \"length\"",
        "5\n4\nlength");
    expect_program_success(
        "Dim text As String\n"
        "text = \"WfC\"\n"
        "Print LCase(text)\n"
        "Print UCase$(text)\n"
        "Print LCase(UCase(\"MiXeD\"))",
        "wfc\nWFC\nmixed");
    expect_program_success(
        "Print Trim(\"     \")\n"
        "Print UCase(Trim(\"  ready  \"))\n"
        "Print \"[\" & Trim(\"\tkept\t\") & \"]\"",
        "\nREADY\n[\tkept\t]");
    expect_program_success("If False Then Print Left(\"value\", -1)", "");
    expect_program_success("Print \"\"\nPrint \"second\"", "\nsecond");
    expect_program_success(
        "Print True\n"
        "Print False\n"
        "Print 2 + 3 * 4 = 14\n"
        "Print \"a\" & \"b\" = \"ab\"\n"
        "Print 2 < 10\n"
        "Print \"alpha\" < \"beta\"\n"
        "Print 3 <> 4\n"
        "Print 3 <= 3\n"
        "Print 4 > 3\n"
        "Print 4 >= 5\n"
        "Print True <> False",
        "True\nFalse\nTrue\nTrue\nTrue\nTrue\nTrue\nTrue\nTrue\nFalse\nTrue");
    expect_program_success(
        "Print Not False And False Or True\n"
        "Print True Xor True\n"
        "Print True Eqv True\n"
        "Print True Imp False",
        "True\nFalse\nTrue\nFalse");
    expect_program_success(
        "Dim ready As Boolean\n"
        "ready = 10 >= 5\n"
        "Print ready",
        "True");
    expect_program_success(
        "Dim total As Long\n"
        "If True Then total = 6 * 7 Else total = 1 \\ 0\n"
        "If total = 42 Then Print \"selected \" & total Else Print \"wrong\"",
        "selected 42");
    expect_program_success(
        "Dim ready As Boolean\n"
        "ready = False\n"
        "If ready Then Print \"wrong\" Else Print \"fallback\"",
        "fallback");
    expect_program_success(
        "Dim value As Long\n"
        "value = 9\n"
        "If False Then value = 2 Else Let value = value + 1\n"
        "Print value",
        "10");
    expect_program_success(
        "Dim total As Long\n"
        "If True Then\n"
        "  total = 6 * 7\n"
        "  If total = 42 Then\n"
        "    Print \"nested\"\n"
        "  End If\n"
        "Else\n"
        "  total = 1 \\ 0\n"
        "End If\n"
        "Print total",
        "nested\n42");
    expect_program_success(
        "If False Then\n"
        "  Print \"wrong\"\n"
        "Else\n"
        "  Print \"fallback\"\n"
        "End If",
        "fallback");
    expect_program_success(
        "Dim index As Long\n"
        "Dim total As Long\n"
        "index = 1\n"
        "While index <= 4\n"
        "  total = total + index\n"
        "  index = index + 1\n"
        "Wend\n"
        "Print total",
        "10");
    expect_program_success(
        "While False\n"
        "  Print 1 \\ 0\n"
        "Wend\n"
        "Print \"done\"",
        "done");
    expect_program_success(
        "Dim outer As Long\n"
        "Dim inner As Long\n"
        "While outer < 2\n"
        "  inner = 0\n"
        "  While inner < 2\n"
        "    Print outer * 2 + inner\n"
        "    inner = inner + 1\n"
        "  Wend\n"
        "  outer = outer + 1\n"
        "Wend",
        "0\n1\n2\n3");
    expect_program_success(
        "Dim index As Long\n"
        "Do While index < 3\n"
        "  Print index\n"
        "  index = index + 1\n"
        "Loop",
        "0\n1\n2");
    expect_program_success(
        "Dim index As Long\n"
        "index = 3\n"
        "Do Until index = 0\n"
        "  index = index - 1\n"
        "Loop\n"
        "Print index",
        "0");
    expect_program_success(
        "Do While False\n"
        "  Print 1 \\ 0\n"
        "Loop\n"
        "Print \"done\"",
        "done");
    expect_program_success(
        "Dim index As Long\n"
        "Do\n"
        "  Print index\n"
        "  index = index + 1\n"
        "Loop While index < 3",
        "0\n1\n2");
    expect_program_success(
        "Dim index As Long\n"
        "Do\n"
        "  index = index + 1\n"
        "Loop Until index = 2\n"
        "Print index",
        "2");
    expect_program_success(
        "Do\n"
        "  Print \"once\"\n"
        "Loop While False",
        "once");
    expect_program_success(
        "Dim index As Long\n"
        "Do While True\n"
        "  index = index + 1\n"
        "  If index = 3 Then\n"
        "    Exit Do\n"
        "    index = 99\n"
        "  End If\n"
        "  Print index\n"
        "Loop\n"
        "Print index",
        "1\n2\n3");
    expect_program_success(
        "Do\n"
        "  Print \"once\"\n"
        "  Exit Do\n"
        "  Print \"wrong\"\n"
        "Loop While 1 \\ 0 = 0\n"
        "Print \"done\"",
        "once\ndone");
    expect_program_success(
        "Dim outer As Long\n"
        "Dim inner As Long\n"
        "Do While outer < 2\n"
        "  inner = 0\n"
        "  Do While True\n"
        "    inner = inner + 1\n"
        "    Exit Do\n"
        "  Loop\n"
        "  Print inner\n"
        "  outer = outer + 1\n"
        "Loop",
        "1\n1");
    expect_program_success(
        "Dim index As Long\n"
        "Dim total As Long\n"
        "For index = 1 To 4\n"
        "  total = total + index\n"
        "Next index\n"
        "Print total\n"
        "Print index",
        "10\n5");
    expect_program_success(
        "Dim index As Long\n"
        "For index = 3 To 1 Step -1\n"
        "  Print index\n"
        "Next",
        "3\n2\n1");
    expect_program_success(
        "Dim index As Long\n"
        "index = 99\n"
        "For index = 3 To 1\n"
        "  Print 1 \\ 0\n"
        "Next index\n"
        "Print index",
        "3");
    expect_program_success(
        "Dim outer As Long\n"
        "Dim inner As Long\n"
        "Dim total As Long\n"
        "For outer = 1 To 2\n"
        "  For inner = 1 To 2\n"
        "    total = total + outer + inner\n"
        "  Next inner\n"
        "Next outer\n"
        "Print total",
        "12");
    expect_program_success(
        "Dim index As Long\n"
        "For index = 1 To 10\n"
        "  If index = 3 Then\n"
        "    Exit For\n"
        "    Print \"wrong\"\n"
        "  End If\n"
        "  Print index\n"
        "Next index\n"
        "Print index",
        "1\n2\n3");
    expect_program_success(
        "Dim outer As Long\n"
        "Dim inner As Long\n"
        "For outer = 1 To 2\n"
        "  For inner = 1 To 3\n"
        "    Print outer * 10 + inner\n"
        "    Exit For\n"
        "  Next inner\n"
        "Next outer",
        "11\n21");
    expect_program_success(
        "Dim index As Long\n"
        "For index = 1 To 3\n"
        "  While True\n"
        "    Exit For\n"
        "    Print \"wrong\"\n"
        "  Wend\n"
        "  Print \"wrong\"\n"
        "Next index\n"
        "Print index",
        "1");
    expect_program_success(
        "Dim value As Long\n"
        "value = 2\n"
        "Select Case value\n"
        "Case 1\n"
        "  Print \"one\"\n"
        "Case 2\n"
        "  Print \"two\"\n"
        "Case Else\n"
        "  Print \"other\"\n"
        "End Select",
        "two");
    expect_program_success(
        "Select Case \"beta\"\n"
        "Case \"alpha\"\n"
        "  Print 1 \\ 0\n"
        "Case Else\n"
        "  Print \"fallback\"\n"
        "End Select",
        "fallback");
    expect_program_success(
        "Select Case True\n"
        "Case True\n"
        "  Print \"selected\"\n"
        "Case False\n"
        "  Print 1 \\ 0\n"
        "End Select",
        "selected");
    expect_program_success(
        "Select Case 7\n"
        "Case 1 To 3, 7 To 9, 20\n"
        "  Print \"selected\"\n"
        "Case Else\n"
        "  Print \"wrong\"\n"
        "End Select",
        "selected");
    expect_program_success(
        "Select Case \"m\"\n"
        "Case \"a\" To \"f\"\n"
        "  Print \"wrong\"\n"
        "Case \"g\" To \"z\"\n"
        "  Print \"selected\"\n"
        "End Select",
        "selected");
    expect_program_success(
        "Select Case 7\n"
        "Case Is < 0\n"
        "  Print \"negative\"\n"
        "Case Is >= 5, 1 To 3\n"
        "  Print \"selected\"\n"
        "Case Else\n"
        "  Print \"wrong\"\n"
        "End Select",
        "selected");
    expect_program_success(
        "Select Case \"beta\"\n"
        "Case Is > \"omega\"\n"
        "  Print \"wrong\"\n"
        "Case Is <= \"beta\"\n"
        "  Print \"selected\"\n"
        "End Select",
        "selected");
    expect_program_success(
        "Select Case True\n"
        "Case Is = True\n"
        "  Print \"selected\"\n"
        "End Select",
        "selected");
    expect_program_success(
        "Select Case 2\n"
        "Case 0, 1, 2, 1 \\ 0\n"
        "  Print \"selected\"\n"
        "Case Else\n"
        "  Print \"wrong\"\n"
        "End Select",
        "selected");
    expect_program_success(
        "Select Case \"beta\"\n"
        "Case \"alpha\", \"beta\", \"gamma\"\n"
        "  Print \"selected\"\n"
        "End Select",
        "selected");
    expect_program_success(
        "Dim result As String\n"
        "If False Then\n"
        "  result = \"first\"\n"
        "ElseIf 2 + 2 = 5 Then\n"
        "  result = \"second\"\n"
        "ElseIf True Then\n"
        "  result = \"third\"\n"
        "Else\n"
        "  result = \"fallback\"\n"
        "End If\n"
        "Print result",
        "third");
    expect_program_success(
        "If True Then\n"
        "  Print \"selected\"\n"
        "ElseIf 1 \\ 0 = 0 Then\n"
        "  Print \"wrong\"\n"
        "Else\n"
        "  Print \"wrong\"\n"
        "End If",
        "selected");
    expect_program_success(
        "If False Then\n"
        "  Print \"wrong\"\n"
        "ElseIf False Then\n"
        "  Print \"wrong\"\n"
        "Else\n"
        "  Print \"fallback\"\n"
        "End If",
        "fallback");
    expect_program_failure("Dim 1 As Long", "WFC0011");
    expect_program_failure("Dim value As Integer", "WFC0012");
    expect_program_failure("Dim value As Long: Dim VALUE As Long", "WFC0013");
    expect_program_failure("Dim value As Long: value 1", "WFC0014");
    expect_program_failure("missing = 1", "WFC0015");
    expect_program_failure("Print missing", "WFC0015");
    expect_program_failure("Dim value As Long: value = \"wrong\"", "WFC0016");
    expect_program_failure("Dim value As Double: value = \"wrong\"", "WFC0016");
    expect_program_failure("Const value As Double = \"wrong\"", "WFC0016");
    expect_program_failure("Dim value As Long: value# = 1", "WFC0016");
    expect_program_failure("Dim value#: Dim value As Double", "WFC0013");
    expect_program_failure("Dim value# As Double", "WFC0012");
    expect_program_failure("Dim value!", "WFC0097");
    expect_program_failure("Dim Print As Long", "WFC0017");
    expect_program_failure("Dim Rem As Long", "WFC0017");
    expect_program_failure("Const answer As Long = 42: answer = 1", "WFC0062");
    expect_program_failure(
        "If True Then\nConst local As Long = 1\nEnd If",
        "WFC0063");
    expect_program_failure(
        "Dim value As Long: value = 1: Const snapshot As Long = value",
        "WFC0064");
    expect_program_failure("Const wrong As Long = \"text\"", "WFC0016");
    expect_program_failure("Option Nope", "WFC0065");
    expect_program_failure("Dim value As Long\nOption Explicit", "WFC0066");
    expect_program_failure("Option Explicit\nOption Explicit", "WFC0067");
    expect_program_failure(
        "If True Then\nOption Explicit\nEnd If",
        "WFC0068");
    expect_program_failure("Option Compare Text\nOption Compare Binary", "WFC0069");
    expect_program_failure("Option Compare Database", "WFC0070");
    expect_program_failure("Dim vbTextCompare As Long", "WFC0017");
    expect_program_failure("Dim vbTrue As Long", "WFC0017");
    expect_program_failure("Dim vbReadOnly As Long", "WFC0017");
    expect_program_failure("Print Missing(\"value\")", "WFC0071");
    expect_program_failure("Print Len()", "WFC0072");
    expect_program_failure("Print Len(\"one\", \"two\")", "WFC0072");
    expect_program_failure("Print Left(\"value\")", "WFC0072");
    expect_program_failure("Print Right(\"value\", 1, 2)", "WFC0072");
    expect_program_failure("Print Left(\"value\",)", "WFC0072");
    expect_program_failure("Print Mid(\"value\")", "WFC0072");
    expect_program_failure("Print Mid(\"value\", 1, 2, 3)", "WFC0072");
    expect_program_failure("Print Space()", "WFC0072");
    expect_program_failure("Print Space(1, 2)", "WFC0072");
    expect_program_failure("Print String(3)", "WFC0072");
    expect_program_failure("Print String(3, \"*\", 1)", "WFC0072");
    expect_program_failure("Print InStr(\"a\")", "WFC0072");
    expect_program_failure("Print InStr(1, \"a\", \"b\", 0, 1)", "WFC0072");
    expect_program_failure("Print InStrRev(\"a\")", "WFC0072");
    expect_program_failure("Print InStrRev(\"a\", \"a\", -1, 0, 1)", "WFC0072");
    expect_program_failure("Print StrComp(\"a\")", "WFC0072");
    expect_program_failure("Print StrComp(\"a\", \"b\", 0, 1)", "WFC0072");
    expect_program_failure("Print Replace(\"a\", \"b\")", "WFC0072");
    expect_program_failure(
        "Print Replace(\"a\", \"b\", \"c\", 1, -1, 0, 1)",
        "WFC0072");
    expect_program_failure("Print Len(42)", "WFC0073");
    expect_program_failure("Print LCase(True)", "WFC0073");
    expect_program_failure("Print Left(42, 1)", "WFC0073");
    expect_program_failure("Print Right(\"value\", \"1\")", "WFC0073");
    expect_program_failure("Print Mid(\"value\", \"1\")", "WFC0073");
    expect_program_failure("Print Mid(\"value\", 1, \"2\")", "WFC0073");
    expect_program_failure("Print Asc(42)", "WFC0073");
    expect_program_failure("Print Chr(\"65\")", "WFC0073");
    expect_program_failure("Print StrReverse(False)", "WFC0073");
    expect_program_failure("Print Space(\"3\")", "WFC0073");
    expect_program_failure("Print String(\"3\", \"*\")", "WFC0073");
    expect_program_failure("Print String(3, True)", "WFC0073");
    expect_program_failure("Print InStr(\"a\", 1)", "WFC0073");
    expect_program_failure("Print InStr(\"1\", \"a\", \"b\")", "WFC0073");
    expect_program_failure("Print InStr(1, \"a\", \"b\", \"1\")", "WFC0073");
    expect_program_failure("Print InStrRev(1, \"a\")", "WFC0073");
    expect_program_failure("Print InStrRev(\"a\", \"a\", \"1\")", "WFC0073");
    expect_program_failure("Print StrComp(\"a\", 1)", "WFC0073");
    expect_program_failure("Print StrComp(42, \"a\")", "WFC0073");
    expect_program_failure("Print StrComp(\"a\", \"b\", \"1\")", "WFC0073");
    expect_program_failure("Print Replace(\"a\", \"b\", 3)", "WFC0073");
    expect_program_failure("Print Replace(42, \"b\", \"c\")", "WFC0073");
    expect_program_failure("Print Replace(\"a\", \"b\", \"c\", \"1\")", "WFC0073");
    expect_program_failure("Print Replace(\"a\", \"b\", \"c\", 1, \"1\")", "WFC0073");
    expect_program_failure("Print Hex(\"a\")", "WFC0073");
    expect_program_failure("Print Oct(True)", "WFC0073");
    expect_program_failure("Print Hex(2147483648#)", "WFC0009");
    expect_program_failure("Print Str(\"a\")", "WFC0073");
    expect_program_failure("Print Val(42)", "WFC0073");
    expect_program_failure("Print Abs(\"42\")", "WFC0073");
    expect_program_failure("Print Sgn(False)", "WFC0073");
    expect_program_failure("Const size As Long = Len(\"value\")", "WFC0074");
    expect_program_failure("Print Left(\"value\", -1)", "WFC0075");
    expect_program_failure("Print Mid(\"value\", 1, -1)", "WFC0075");
    expect_program_failure("Print Space(-1)", "WFC0075");
    expect_program_failure("Print String(-1, \"*\")", "WFC0075");
    expect_program_failure("Print Mid(\"value\", 0)", "WFC0076");
    expect_program_failure("Print InStr(0, \"value\", \"a\")", "WFC0076");
    expect_program_failure("Print Replace(\"a\", \"a\", \"b\", 0)", "WFC0076");
    expect_program_failure("Print Asc(\"\")", "WFC0077");
    expect_program_failure("Print Chr(-1)", "WFC0078");
    expect_program_failure("Print Chr(128)", "WFC0078");
    expect_program_failure("Print ChrB(256)", "WFC0078");
    expect_program_failure(
        "Print InStr(1, \"a\", \"a\", vbDatabaseCompare)",
        "WFC0081");
    expect_program_failure("Print StrComp(\"a\", \"A\", 99)", "WFC0081");
    expect_program_failure(
        "Print Replace(\"a\", \"a\", \"b\", 1, -1, vbDatabaseCompare)",
        "WFC0081");
    expect_program_failure("Print Replace(\"a\", \"a\", \"b\", 1, -2)", "WFC0082");
    expect_program_failure("Print InStrRev(\"a\", \"a\", 0)", "WFC0083");
    expect_program_failure("Print InStrRev(\"a\", \"a\", -2)", "WFC0083");
    expect_success("Print Val(\"12.5\")", "12.5");
    expect_success("Print Val(\"1E2\")", "100");
    expect_success("Print Val(\"3.14 pie\")", "3.14");
    expect_success("Print Val(\".5\")", "0.5");
    expect_success("Print Val(\"-2.5e1\")", "-25");
    expect_success("Print Val(\"-&H10\") & \" \" & Val(\"&H2Atrail\")", "-16 42");
    expect_program_failure("Print Val(\"&H100000000\")", "WFC0009");
    expect_program_failure("Print Val(\"2147483648\")", "WFC0009");
    expect_program_failure("Print Abs(-2147483648)", "WFC0009");
    expect_program_failure("Print CLng(\"2147483648\")", "WFC0009");
    expect_program_failure("Print CLng(\"\")", "WFC0086");
    expect_program_failure("Print CLng(\"12 trailing\")", "WFC0086");
    expect_program_failure("Print CLng(\"12e\")", "WFC0086");
    expect_program_failure("Print CBool(\"\")", "WFC0087");
    expect_program_failure("Print CBool(\"yes\")", "WFC0087");
    expect_program_failure("Print CBool(\"1e\")", "WFC0087");
    expect_program_failure("Print CBool(\"NaN\")", "WFC0087");
    expect_program_failure("Print CByte(-1)", "WFC0009");
    expect_program_failure("Print CByte(256)", "WFC0009");
    expect_program_failure("Print CByte(255.5)", "WFC0009");
    expect_program_failure("Print CByte(\"255.5\")", "WFC0009");
    expect_program_failure("Print CByte(\"1e\")", "WFC0098");
    expect_program_failure("Print CByte(\"NaN\")", "WFC0098");
    expect_program_failure("Print CVar()", "WFC0072");
    expect_program_failure("Print CVar(1, 2)", "WFC0072");
    expect_program_failure("Print CInt(32768)", "WFC0009");
    expect_program_failure("Print CInt(-32769)", "WFC0009");
    expect_program_failure("Print CInt(\"40000\")", "WFC0009");
    expect_program_failure("Print CInt(\"\")", "WFC0088");
    expect_program_failure("Print CInt(\"12e\")", "WFC0088");
    expect_program_failure("Print IIf(42, \"a\", \"b\")", "WFC0021");
    expect_program_failure("Print Choose(0, \"a\", \"b\")", "WFC0089");
    expect_program_failure("Print Choose(3, \"a\", \"b\")", "WFC0089");
    expect_program_failure("Print Switch(False, 1, False, 2)", "WFC0090");
    expect_program_failure("Print Switch(42, \"a\")", "WFC0021");
    expect_program_failure("Print Int(\"x\")", "WFC0073");
    expect_program_failure("Print RGB(1, 2)", "WFC0072");
    expect_program_failure("Print RGB(\"1\", 2, 3)", "WFC0073");
    expect_program_failure("Print RGB(-1, 0, 0)", "WFC0091");
    expect_program_failure("Print QBColor()", "WFC0072");
    expect_program_failure("Print QBColor(\"1\")", "WFC0073");
    expect_program_failure("Print QBColor(16)", "WFC0092");
    expect_program_failure("Print StrConv(\"a\")", "WFC0072");
    expect_program_failure("Print StrConv(\"a\", \"1\")", "WFC0073");
    expect_program_failure("Print StrConv(\"a\", vbUnicode)", "WFC0093");
    expect_program_failure("Print StrConv(\"a\", 99)", "WFC0093");
    expect_program_failure("Print Round(\"x\")", "WFC0073");
    expect_program_failure("Print Round(5, -1)", "WFC0094");
    expect_program_failure("Print Round(1, 2, 3)", "WFC0072");
    expect_program_failure("Print 5 / 0", "WFC0008");
    expect_program_failure("Print CDbl(\"x\")", "WFC0095");
    expect_program_failure("Print CDbl(\"\")", "WFC0095");
    expect_program_failure("Print CSng(\"1.2.3\")", "WFC0095");
    expect_program_failure("Print Sqr(-1)", "WFC0096");
    expect_program_failure("Print Log(0)", "WFC0096");
    expect_program_failure("Print Sqr(\"x\")", "WFC0073");
    expect_program_failure("Print 1 = \"1\"", "WFC0018");
    expect_program_failure("Print True < False", "WFC0018");
    expect_program_failure("Print 1 And 2", "WFC0019");
    expect_program_failure("Print True & \" value\"", "WFC0020");
    expect_program_failure("If 1 Then Print \"no\"", "WFC0021");
    expect_program_failure("If True Print \"no\"", "WFC0022");
    expect_program_failure("If True Then", "WFC0023");
    expect_program_failure("If False Then Print \"no\" Else", "WFC0023");
    expect_program_failure("If True Then\nPrint \"no\"", "WFC0024");
    expect_program_failure("If True Then\nEnd Nope", "WFC0025");
    expect_program_failure("If True Then\nElse\nElse\nEnd If", "WFC0026");
    expect_program_failure("If True Then\nDim local As Long\nEnd If", "WFC0027");
    expect_program_failure("If False Then\nElseIf 1 Then\nEnd If", "WFC0028");
    expect_program_failure("If False Then\nElseIf True\nEnd If", "WFC0029");
    expect_program_failure(
        "If False Then\nElse\nElseIf True Then\nEnd If",
        "WFC0030");
    expect_program_failure("While 1\nWend", "WFC0031");
    expect_program_failure("While True\nPrint \"no\"", "WFC0032");
    expect_program_failure("Wend", "WFC0033");
    expect_program_failure("While False\nDim local As Long\nWend", "WFC0034");
    expect_program_failure("While False: Print \"no\": Wend", "WFC0004");
    expect_program_failure("Do While 1\nLoop", "WFC0035");
    expect_program_failure("Do Nope\nLoop", "WFC0036");
    expect_program_failure("Do While True\nPrint \"no\"", "WFC0037");
    expect_program_failure("Loop", "WFC0038");
    expect_program_failure("Do Until True\nDim local As Long\nLoop", "WFC0039");
    expect_program_failure("Do While False: Print \"no\": Loop", "WFC0004");
    expect_program_failure("Do\nPrint \"no\"\nLoop", "WFC0040");
    expect_program_failure("Do\nPrint \"no\"\nLoop While 1", "WFC0035");
    expect_program_failure("Exit Nope", "WFC0041");
    expect_program_failure("Exit Do", "WFC0042");
    expect_program_failure("Dim Exit As Long", "WFC0017");
    expect_program_failure("For = 1 To 2\nNext", "WFC0043");
    expect_program_failure("Dim i As Long\nFor i = 1 2\nNext", "WFC0044");
    expect_program_failure("Dim i As Long\nFor i = \"a\" To \"b\"\nNext", "WFC0045");
    expect_program_failure("Dim i As Long\nFor i = 1 To 2\nPrint i", "WFC0046");
    expect_program_failure("Dim i As Long\nFor i = 1 To 2 Step 0\nNext", "WFC0047");
    expect_program_failure("Next", "WFC0048");
    expect_program_failure(
        "Dim i As Long\nDim j As Long\nFor i = 1 To 2\nNext j",
        "WFC0049");
    expect_program_failure("Dim i As Long\nFor i = 1 To 2\nDim j As Long\nNext", "WFC0050");
    expect_program_failure("Exit For", "WFC0052");
    expect_program_failure("Select Nope 1\nEnd Select", "WFC0054");
    expect_program_failure("Select Case 1\nPrint 1\nEnd Select", "WFC0054");
    expect_program_failure("Select Case 1\nCase 1\nEnd Nope", "WFC0055");
    expect_program_failure(
        "Select Case 1\nCase Else\nCase Else\nEnd Select",
        "WFC0056");
    expect_program_failure(
        "Select Case 1\nCase Else\nCase 1\nEnd Select",
        "WFC0057");
    expect_program_failure("Case 1", "WFC0058");
    expect_program_failure("Select Case 1\nCase \"1\"\nEnd Select", "WFC0053");
    expect_program_failure("Select Case 1\nCase 1,\nEnd Select", "WFC0059");
    expect_program_failure("Select Case 1\nCase , 1\nEnd Select", "WFC0059");
    expect_program_failure("Select Case True\nCase False To True\nEnd Select", "WFC0060");
    expect_program_failure("Select Case 1\nCase 0 To \"2\"\nEnd Select", "WFC0060");
    expect_program_failure("Select Case 1\nCase Is 1\nEnd Select", "WFC0061");
    expect_program_failure("Select Case True\nCase Is < False\nEnd Select", "WFC0018");

    if (failures != 0) {
        std::cerr << failures << " evaluator test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "all evaluator tests passed\n";
    return EXIT_SUCCESS;
}
