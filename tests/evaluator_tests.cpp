#include "wfc/evaluator.hpp"

#include <cstdlib>
#include <iostream>
#include <string_view>
#include <vector>

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

void expect_classes_success(
    const std::vector<wfc::ClassModuleSource>& classes,
    const std::string_view source,
    const std::string_view expected) {
    const auto result = wfc::evaluate_program(source, classes);
    if (!result.success || result.output != expected || !result.diagnostic.empty()) {
        std::cerr << "expected success for [" << source << "] with classes but got ["
                  << result.diagnostic << "]\n";
        ++failures;
    }
}

void expect_classes_failure(
    const std::vector<wfc::ClassModuleSource>& classes,
    const std::string_view source,
    const std::string_view code) {
    const auto result = wfc::evaluate_program(source, classes);
    if (result.success || !result.output.empty() || !result.diagnostic.starts_with(code)) {
        std::cerr << "expected " << code << " for [" << source << "] with classes but got ["
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
    expect_success(
        "Print Hex(\" 15.5 \" ) & \" \" & Hex(\"+17.5\") & \" \" & Oct(\"7.5\")",
        "10 12 10");
    expect_success("Print Hex(\"2.55e2\") & \" \" & Oct(\"8e0\")", "FF 10");
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
    expect_success("Print CByte(True) & \" \" & CByte(False)", "255 0");
    expect_success("Print CByte(2.5) & \" \" & CByte(3.5) & \" \" & CByte(254.6)", "2 4 255");
    expect_success(
        "Print CByte(\" 2.5 \" ) & \" \" & CByte(\"+3.5\") & \" \" & CByte(\"2.546e2\")",
        "2 4 255");
    expect_success(
        "Print TypeName(CVar(42)) & \" \" & TypeName(CVar(2.5)) & \" \" & TypeName(CVar(True)) & \" \" & TypeName(CVar(\"x\"))",
        "Long Double Boolean String");
    expect_success("Print CVar(42) + 1 & \" \" & CVar(\"value\")", "43 value");
    expect_success("Print Hex(MacID(\"TEXT\")) & \" \" & Hex(MacID(\"XLS8\"))", "54455854 584C5338");
    expect_success("Print MacID(\"ABCD\")", "1094861636");
    expect_success("Print Error(5) & \" | \" & Error$(11)", "Invalid procedure call or argument | Division by zero");
    expect_program_success(
        "Print Error(6): Print Error(7): Print Error(9): Print Error(13): Print Error(28)",
        "Overflow\nOut of memory\nSubscript out of range\nType mismatch\nOut of stack space");
    expect_program_success(
        "Print Error(53): Print Error(70): Print Error(76): Print Error(91)",
        "File not found\nPermission denied\nPath not found\nObject variable or With block variable not set");
    expect_success("Print \"[\" & Error() & \"] [\" & Error(0) & \"]\"", "[] []");
    expect_success("Print Error(1000)", "Application-defined or object-defined error");
    expect_success("Print Error(65535)", "Application-defined or object-defined error");
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
    expect_success("Print Format(42) & \" \" & Format(3.5) & \" \" & Format(\"hi\")",
                   "42 3.5 hi");
    expect_program_success("Print Format(True): Print Format(False)", "True\nFalse");
    expect_success(
        "Print Format(1234.5678, \"General Number\") & \" \" & Format(1234, \"General Number\")",
        "1234.5678 1234");
    expect_success("Print Format(True, \"General Number\") & \" \" & Format(False, \"General Number\")",
                   "-1 0");
    expect_success("Print Format(3, \"Fixed\") & \" \" & Format(-3.456, \"Fixed\")",
                   "3.00 -3.46");
    expect_success("Print Format(-0.001, \"Fixed\") & \" \" & Format(-0.0, \"Fixed\")",
                   "0.00 0.00");
    expect_success(
        "Print Format(1234.5, \"Standard\") & \" \" & Format(-1234567.891, \"Standard\")",
        "1,234.50 -1,234,567.89");
    expect_success("Print Format(100, \"Standard\")", "100.00");
    expect_success("Print Format(0.5, \"Percent\")", "50.00%");
    expect_success(
        "Print Format(1234.5678, \"Scientific\") & \" \" & Format(0, \"Scientific\") & \" \" & "
        "Format(0.0001234, \"Scientific\")",
        "1.23E+03 0.00E+00 1.23E-04");
    expect_program_success(
        "Print Format(True, \"Yes/No\"): Print Format(0, \"True/False\"): "
        "Print Format(1, \"On/Off\")",
        "Yes\nFalse\nOn");
    expect_success("Print Format$(42, \"Fixed\")", "42.00");
    expect_success(
        "Print Rnd() & \" \" & Rnd() & \" \" & Rnd()",
        "0.7055475 0.533424 0.5795186");
    expect_program_success(
        "Print Rnd(): Print Rnd(): Print Rnd(0)",
        "0.7055475\n0.533424\n0.533424");
    expect_success("Print Rnd(0)", "0");
    expect_success(
        "Print Rnd(1) & \" \" & Rnd(2) & \" \" & Rnd(3)",
        "0.7055475 0.533424 0.5795186");
    expect_success("Print Rnd(-5) & \" \" & Rnd(-5)", "0.08105588 0.08105588");
    expect_program_success(
        "Print Rnd(-5): Print Rnd(1)",
        "0.08105588\n0.30735058");
    expect_program_success(
        "Randomize 42\nPrint Rnd()\nPrint Rnd()\nRandomize 42\nPrint Rnd()",
        "0.21560597\n0.52674717\n0.21560597");
    expect_program_success(
        "Randomize 1\nPrint Rnd()\nRandomize 2\nPrint Rnd()",
        "0.11961287\n0.7612116");
    expect_program_success(
        "Randomize\nPrint Rnd() >= 0 And Rnd() < 1",
        "True");
    expect_success("Print TypeName(Rnd())", "Single");
    // Single literals, declarations, arithmetic promotion, and conversion.
    expect_success("Print 3.5!", "3.5");
    expect_success("Print 42!", "42");
    expect_success("Print -3.5!", "-3.5");
    expect_success("Print TypeName(3.5!) & \" \" & VarType(3.5!)", "Single 4");
    expect_program_success(
        "Dim x As Single: x = 2.5: Print x & \" \" & TypeName(x)",
        "2.5 Single");
    expect_program_success(
        "Dim x!: x = 2.5: Print x & \" \" & TypeName(x)",
        "2.5 Single");
    expect_program_success(
        "Const pi As Single = 3.14: Print pi & \" \" & TypeName(pi)",
        "3.14 Single");
    expect_program_success(
        "Dim y As Single\nDim z As Double\ny = 2.5\nz = y\nPrint z & \" \" & TypeName(z)",
        "2.5 Double");
    expect_success("Print CSng(2.25) & \" \" & TypeName(CSng(2.25))", "2.25 Single");
    expect_success("Print TypeName(2.5! + 1)", "Single");
    expect_success("Print TypeName(2.5! + 1.0#)", "Double");
    expect_success("Print 2.5! + 1.5! & \" \" & TypeName(2.5! + 1.5!)", "4 Single");
    expect_success("Print 5! / 2!", "2.5");
    expect_success("Print Abs(-3.5!) & \" \" & TypeName(Abs(-3.5!))", "3.5 Single");
    expect_success("Print Int(-2.5!) & \" \" & TypeName(Int(-2.5!))", "-3 Single");
    expect_success("Print Fix(-2.5!) & \" \" & TypeName(Fix(-2.5!))", "-2 Single");
    expect_success("Print Sgn(-3.5!)", "-1");
    expect_success("Print Round(2.5!) & \" \" & TypeName(Round(2.5!))", "2 Single");
    expect_success(
        "Print CLng(3.5!) & \" \" & CInt(3.5!) & \" \" & CByte(3.5!) & \" \" & CDbl(3.5!)",
        "4 4 4 3.5");
    expect_success("Print CBool(0!)", "False");
    expect_success("Print CStr(3.5!)", "3.5");
    expect_success("Print IsNumeric(3.5!)", "True");
    expect_success("Print Str(3.5!)", " 3.5");
    expect_success("Print Hex(255!)", "FF");
    expect_success("Print Format(3.5!, \"Fixed\")", "3.50");
    expect_success("Print 3.5! = 3.5", "True");
    expect_success("Print 3.5! = 3.5#", "True");
    expect_success("Print 3.5! < 4", "True");
    // Currency literals, declarations, exact arithmetic, and conversion.
    expect_success("Print 10.5@", "10.5");
    expect_success("Print 42@", "42");
    expect_success("Print -1.5@", "-1.5");
    expect_success("Print TypeName(5@) & \" \" & VarType(5@)", "Currency 6");
    expect_program_success(
        "Dim x As Currency: x = 10.5: Print x & \" \" & TypeName(x)",
        "10.5 Currency");
    expect_program_success(
        "Dim x@: x = 10.5: Print x & \" \" & TypeName(x)",
        "10.5 Currency");
    expect_program_success(
        "Const c As Currency = 10.5: Print c & \" \" & TypeName(c)",
        "10.5 Currency");
    expect_program_success(
        "Dim x As Currency\nDim y As Double\nx = 10.5\ny = x\nPrint y & \" \" & TypeName(y)",
        "10.5 Double");
    expect_success("Print CCur(5) & \" \" & TypeName(CCur(5))", "5 Currency");
    expect_success("Print CCur(\"10.5\")", "10.5");
    expect_success("Print CCur(3.5!)", "3.5");
    expect_success("Print 10@ + 5@", "15");
    expect_success("Print 10.5@ - 3.25@", "7.25");
    expect_success("Print 100.5@ * 3@", "301.5");
    expect_success("Print 10@ / 4@", "2.5");
    expect_success("Print 10@ / 3@", "3.3333");
    expect_success("Print TypeName(5@ + 3)", "Currency");
    expect_success("Print 5@ + 3.5! & \" \" & TypeName(5@ + 3.5!)", "8.5 Single");
    expect_success("Print 5@ + 3.5 & \" \" & TypeName(5@ + 3.5)", "8.5 Double");
    expect_success("Print Abs(-5.5@) & \" \" & TypeName(Abs(-5.5@))", "5.5 Currency");
    expect_success("Print Int(-2.5@) & \" \" & Fix(-2.5@)", "-3 -2");
    expect_success(
        "Print Round(1.2345@, 2) & \" \" & TypeName(Round(1.2345@, 2))",
        "1.23 Currency");
    expect_success("Print CLng(5.5@)", "6");
    expect_success("Print CDbl(10.5@) & \" \" & TypeName(CDbl(10.5@))", "10.5 Double");
    expect_success("Print CSng(10.5@) & \" \" & TypeName(CSng(10.5@))", "10.5 Single");
    expect_success("Print IsNumeric(5@)", "True");
    expect_success("Print CStr(5@)", "5");
    expect_success("Print Str(1.5@)", " 1.5");
    expect_success("Print Hex(255@)", "FF");
    expect_success("Print Format(10.5@, \"Fixed\")", "10.50");
    expect_program_success(
        "Print 5@ = 5: Print 5@ = 5.0#: Print 5@ < 6: Print 5.5@ > 5",
        "True\nTrue\nTrue\nTrue");
    expect_success("Print 922337203685477.5807@", "922337203685477.5807");
    expect_program_success("Print CBool(0@): Print CBool(5@)", "False\nTrue");
    expect_success("Print 5@ & 3@", "53");
    // Integer (VB6's 16-bit type) literals, declarations, promotion, and
    // conversion. Not to be confused with this codebase's own `Integer` C++
    // alias, which is the 32-bit Long.
    expect_success("Print 5%", "5");
    expect_success("Print -5%", "-5");
    expect_success("Print 32767%", "32767");
    expect_success("Print -32768%", "-32768");
    expect_failure("Print 32768%", "WFC0006");
    expect_failure("Print -32769%", "WFC0006");
    expect_failure("Print 1.5%", "WFC0006");
    expect_success("Print TypeName(5%) & \" \" & VarType(5%)", "Integer 2");
    expect_program_success(
        "Dim x As Integer: x = 5: Print x & \" \" & TypeName(x)", "5 Integer");
    expect_program_success(
        "Dim x%: x = 5: Print x & \" \" & TypeName(x)", "5 Integer");
    expect_program_success(
        "Const c As Integer = 5: Print c & \" \" & TypeName(c)", "5 Integer");
    expect_program_failure("Dim x As Integer: x = 40000", "WFC0009");
    expect_program_success("Dim x As Integer: x = 3.5: Print x", "4");
    expect_success("Print TypeName(CInt(5)) & \" \" & VarType(CInt(5))", "Integer 2");
    expect_success("Print CInt(\"5\") & \" \" & CInt(3.5!)", "5 4");
    expect_success("Print 5% + 3%", "8");
    expect_success("Print TypeName(5% + 3%)", "Integer");
    expect_program_failure("Print 32000% + 32000%", "WFC0009");
    expect_success("Print TypeName(5% + 3&)", "Long");
    expect_success("Print TypeName(5% + 3.5)", "Double");
    expect_success("Print TypeName(5% + 3.5!)", "Single");
    expect_success("Print TypeName(5% + 3@)", "Currency");
    expect_success("Print TypeName(5% + CDec(3))", "Decimal");
    expect_success("Print 5% / 2%", "2.5");
    expect_success("Print 5% \\ 2% & \" \" & 5% Mod 3%", "2 2");
    expect_success("Print Abs(-5%) & \" \" & TypeName(Abs(-5%))", "5 Integer");
    expect_success("Print Int(-2%) & \" \" & Fix(-2%)", "-2 -2");
    expect_success(
        "Print Round(5%) & \" \" & TypeName(Round(5%))", "5 Integer");
    expect_success("Print CLng(5%) & \" \" & TypeName(CLng(5%))", "5 Long");
    expect_success("Print CByte(5%)", "5");
    expect_program_success("Print CBool(0%): Print CBool(5%)", "False\nTrue");
    expect_success("Print CDbl(5%) & \" \" & CSng(5%) & \" \" & CCur(5%)", "5 5 5");
    expect_success("Print CDec(5%)", "5");
    expect_success("Print CStr(5%)", "5");
    expect_success("Print IsNumeric(5%)", "True");
    expect_success("Print Hex(5%) & \" \" & Oct(8%)", "5 10");
    expect_success("Print Str(5%)", " 5");
    expect_program_success(
        "Print 5% = 5: Print 5% = 5&: Print 5% < 10: Print 5% = 5.0#",
        "True\nTrue\nTrue\nTrue");
    // Fixed-size one-dimensional arrays: Dim arr(n)/arr(lo To hi), indexed
    // read/write, LBound/UBound, IsArray, TypeName/VarType.
    expect_program_success(
        "Dim arr(3) As Long\narr(0) = 10\narr(3) = 40\nPrint arr(0) & \" \" & arr(3)",
        "10 40");
    expect_program_success(
        "Dim arr(1 To 5) As String\narr(1) = \"a\"\narr(5) = \"z\"\n"
        "Print arr(1) & arr(5)",
        "az");
    expect_program_success(
        "Dim arr(-2 To 2) As Long\narr(-2) = 42\nPrint arr(-2) & \" \" & LBound(arr) & "
        "\" \" & UBound(arr)",
        "42 -2 2");
    expect_program_success("Dim arr(3) As Long\nPrint CStr(IsArray(arr))", "True");
    expect_success("Print CStr(IsArray(5))", "False");
    expect_program_success(
        "Dim arr(3) As Long\nPrint TypeName(arr) & \" \" & VarType(arr)", "Long() 8195");
    expect_program_success(
        "Const n As Long = 3\nDim arr(n) As Long\nDim i As Long\nFor i = 0 To n\n"
        "arr(i) = i * i\nNext i\nFor i = 0 To n\nPrint arr(i)\nNext i",
        "0\n1\n4\n9");
    expect_program_failure("Dim arr(3) As Long\nPrint arr(4)", "WFC0111");
    expect_program_failure("Dim arr(3) As Long\narr(4) = 1", "WFC0111");
    expect_program_failure("Dim arr(3) As Long\narr(0) = \"text\"", "WFC0016");
    expect_program_failure("Dim arr(5 To 2) As Long", "WFC0117");
    expect_program_failure("Dim arr(3) As Variant", "WFC0012");
    expect_program_failure("Dim arr(3) As Object", "WFC0012");
    // Dynamic arrays: Dim arr() As Type declares an unallocated array;
    // ReDim/ReDim Preserve allocate/reallocate it (REQ-0207).
    expect_program_success(
        "Dim arr() As Long\nPrint CStr(IsArray(arr)) & \" \" & TypeName(arr)",
        "True Long()");
    expect_program_failure("Dim arr() As Long\nPrint UBound(arr)", "WFC0111");
    expect_program_failure("Dim arr() As Long\nPrint LBound(arr)", "WFC0111");
    expect_program_failure("Dim arr() As Long\nPrint arr(0)", "WFC0111");
    expect_program_failure("Dim arr() As Long\narr(0) = 1", "WFC0111");
    expect_program_success(
        "Dim arr() As Long\nReDim arr(3)\narr(0) = 10\narr(3) = 40\n"
        "Print arr(0) & \" \" & arr(3) & \" \" & LBound(arr) & \" \" & UBound(arr)",
        "10 40 0 3");
    expect_program_success(
        "Dim arr() As String\nReDim arr(1 To 3)\narr(1) = \"a\"\narr(3) = \"c\"\n"
        "Print arr(1) & arr(3) & \" \" & LBound(arr) & \" \" & UBound(arr)",
        "ac 1 3");
    expect_program_success(
        "Dim arr() As Long\nReDim arr(3)\narr(0) = 1\narr(1) = 2\narr(2) = 3\narr(3) = 4\n"
        "ReDim Preserve arr(5)\n"
        "Print arr(0) & \" \" & arr(3) & \" \" & arr(4) & \" \" & arr(5) & \" \" & UBound(arr)",
        "1 4 0 0 5");
    expect_program_success(
        "Dim arr() As Long\nReDim arr(5)\narr(0) = 1\narr(4) = 5\narr(5) = 6\n"
        "ReDim Preserve arr(3)\n"
        "Print arr(0) & \" \" & arr(3) & \" \" & UBound(arr)",
        "1 0 3");
    expect_program_success(
        "Dim arr() As Long\nReDim arr(3)\narr(2) = 9\nReDim arr(3)\nPrint arr(2)",
        "0");
    expect_program_failure(
        "Dim arr(3) As Long\nReDim arr(5)", "WFC0145");
    expect_program_failure("ReDim arr(5)", "WFC0145");
    expect_program_failure("Dim x As Long\nReDim x(5)", "WFC0145");
    expect_program_failure("Dim arr() As Long\nReDim arr(5 To 2)", "WFC0117");
    expect_program_failure("Dim arr() As Long\nReDim arr(2, 3)", "WFC0115");
    expect_program_success(
        "Dim arr() As Long\nIf True Then\nReDim arr(2)\narr(1) = 7\nEnd If\nPrint arr(1)",
        "7");
    expect_program_success(
        "Dim arr() As Long\nReDim Preserve arr(2)\narr(0) = 5\n"
        "Print arr(0) & \" \" & UBound(arr)",
        "5 2");
    // Minimal object-reference stub: Nothing, Set, Is, IsObject.
    expect_success("Print TypeName(Nothing) & \" \" & VarType(Nothing)", "Nothing 9");
    expect_program_success("Dim x As Object\nPrint CStr(IsObject(x))", "True");
    expect_success("Print CStr(IsObject(5))", "False");
    expect_success("Print CStr(Nothing Is Nothing)", "True");
    expect_program_success(
        "Dim x As Object\nSet x = Nothing\nPrint CStr(x Is Nothing)", "True");
    expect_program_success(
        "Dim x As Variant\nSet x = Nothing\nPrint TypeName(x) & \" \" & CStr(x Is Nothing)",
        "Nothing True");
    expect_program_failure("Dim x As Object\nx = Nothing", "WFC0108");
    expect_program_failure("Dim x As Long\nSet x = Nothing", "WFC0109");
    expect_program_failure("Dim x As Object\nSet x = 5", "WFC0106");
    expect_program_failure("Print 5 Is Nothing", "WFC0107");
    expect_program_failure("Print Nothing = Nothing", "WFC0107");
    expect_program_failure("Print CStr(Nothing)", "WFC0106");
    // User-defined Sub/Function procedures: declarations, forward
    // reference, recursion, ByVal/ByRef parameters, local scope, Exit
    // Sub/Function, and the Call statement.
    expect_program_success(
        "Function Add(x As Long, y As Long) As Long\nAdd = x + y\nEnd Function\n"
        "Print Add(2, 3)",
        "5");
    expect_program_success(
        "Print Square(5)\n\nFunction Square(x As Long) As Long\nSquare = x * x\n"
        "End Function",
        "25");
    expect_program_success(
        "Function Factorial(n As Long) As Long\nIf n <= 1 Then\nFactorial = 1\n"
        "Else\nFactorial = n * Factorial(n - 1)\nEnd If\nEnd Function\n"
        "Print Factorial(10)",
        "3628800");
    expect_program_success(
        "Function IsEven(n As Long) As Boolean\nIf n = 0 Then\nIsEven = True\n"
        "Else\nIsEven = IsOdd(n - 1)\nEnd If\nEnd Function\n\n"
        "Function IsOdd(n As Long) As Boolean\nIf n = 0 Then\nIsOdd = False\n"
        "Else\nIsOdd = IsEven(n - 1)\nEnd If\nEnd Function\n\n"
        "Print CStr(IsEven(10)) & \" \" & CStr(IsOdd(10))",
        "True False");
    expect_program_success(
        "Sub Greet(name As String)\nPrint \"Hello, \" & name\nEnd Sub\n"
        "Call Greet(\"World\")",
        "Hello, World");
    expect_program_success(
        "Sub Increment(ByRef x As Long)\nx = x + 1\nEnd Sub\n"
        "Dim n As Long\nn = 5\nCall Increment(n)\nPrint n",
        "6");
    expect_program_success(
        "Sub NoOp(ByVal x As Long)\nx = x + 100\nEnd Sub\n"
        "Dim n As Long\nn = 5\nCall NoOp(n)\nPrint n",
        "5");
    expect_program_success(
        // ByRef is VB6's default when neither ByVal nor ByRef is written.
        "Sub Increment(x As Long)\nx = x + 1\nEnd Sub\n"
        "Dim n As Long\nn = 5\nCall Increment(n)\nPrint n",
        "6");
    expect_program_success(
        "Dim n As Long\nn = 10\n\nSub Modify()\nDim n As Long\nn = 99\n"
        "Print \"local: \" & n\nEnd Sub\n\n"
        "Call Modify()\nPrint \"module: \" & n",
        "local: 99\nmodule: 10");
    expect_program_success(
        "Function Greet(name As Variant) As String\nGreet = \"Hi \" & name\nEnd Function\n"
        "Print Greet(\"Bob\")\nPrint Greet(5)",
        "Hi Bob\nHi 5");
    expect_program_success(
        "Function NoReturn() As Long\nDim x As Long\nx = 5\nEnd Function\n"
        "Print NoReturn()",
        "0");
    expect_program_success(
        "Sub EarlyExit()\nPrint \"before\"\nExit Sub\nPrint \"after\"\nEnd Sub\n"
        "Call EarlyExit()",
        "before");
    expect_program_success(
        "Function EarlyExit() As Long\nEarlyExit = 1\nExit Function\nEarlyExit = 2\n"
        "End Function\nPrint EarlyExit()",
        "1");
    expect_program_success(
        "Function Test() As Long\nTest = 5\nEnd Function\n"
        "Dim x As Long\nx = Test() + Test()\nPrint x",
        "10");
    expect_program_success(
        "Function Foo() As Long\nFoo = 5\nEnd Function\n"
        "If False Then\nPrint Foo()\nEnd If\nPrint \"done\"",
        "done");
    expect_program_success(
        "Sub Foo()\nDim arr(3) As Long\narr(1) = 42\nPrint arr(1)\nEnd Sub\nCall Foo()",
        "42");
    expect_program_failure(
        "Print Foo()\n\nSub Foo()\nPrint \"hi\"\nEnd Sub", "WFC0122");
    expect_program_failure(
        "Function Add(x As Long, y As Long) As Long\nAdd = x + y\nEnd Function\n"
        "Print Add(1)",
        "WFC0072");
    expect_program_failure(
        "Sub Foo()\nEnd Sub\nSub Foo()\nEnd Sub", "WFC0119");
    expect_program_failure("Call Bar()", "WFC0015");
    expect_program_failure("Exit Sub", "WFC0124");
    expect_program_failure(
        "Function Foo() As Long\nFoo = 1\nEnd Function\nExit Function", "WFC0125");
    expect_program_failure(
        "Sub Foo(n As Long)\nExit Function\nEnd Sub\nCall Foo(1)", "WFC0125");
    // Class modules: fields, methods, Property Get/Let/Set, New, and Is
    // identity, each supplied as a separate wfc::ClassModuleSource (this
    // evaluator's stand-in for a real VB6 project's separate .cls files).
    expect_classes_success(
        {{"Counter", "Public value As Long\n\n"
                     "Sub Increment()\nvalue = value + 1\nEnd Sub\n\n"
                     "Function GetValue() As Long\nGetValue = value\nEnd Function"}},
        "Dim c As New Counter\nCall c.Increment()\nCall c.Increment()\nPrint c.GetValue()",
        "2");
    expect_classes_success(
        {{"Counter", "Public value As Long"}},
        "Dim c As New Counter\nc.value = 5\nPrint c.value & \" \" & TypeName(c) & \" \" & "
        "VarType(c) & \" \" & CStr(IsObject(c))",
        "5 Counter 9 True");
    expect_classes_success(
        {{"Box", "Public m_value As Long\n\n"
                 "Property Get Value() As Long\nValue = m_value * 2\nEnd Property\n\n"
                 "Property Let Value(v As Long)\nm_value = v \\ 2\nEnd Property"}},
        "Dim b As New Box\nb.Value = 20\nPrint b.Value & \" \" & b.m_value",
        "20 10");
    expect_classes_success(
        {{"Box", "Public m_inner As Variant\n\n"
                 "Property Set Inner(v As Object)\nSet m_inner = v\nEnd Property\n\n"
                 "Property Get Inner() As Variant\nSet Inner = m_inner\nEnd Property"},
         {"Counter", "Public value As Long"}},
        "Dim b As New Box\nDim c As New Counter\nc.value = 5\nSet b.Inner = c\n"
        "Dim c2 As Counter\nSet c2 = b.Inner\nc2.value = 99\nPrint c.value & \" \" & "
        "CStr(c2 Is c)",
        "99 True");
    // A method calling a sibling method of its own class unqualified
    // (including itself, for recursion) -- the implicit-Me equivalent of
    // `Me.Method(...)`.
    expect_classes_success(
        {{"Calc", "Function Factorial(n As Long) As Long\nIf n <= 1 Then\nFactorial = 1\n"
                  "Else\nFactorial = n * Factorial(n - 1)\nEnd If\nEnd Function\n\n"
                  "Function DoubleFactorial(n As Long) As Long\n"
                  "DoubleFactorial = Factorial(n) * 2\nEnd Function"}},
        "Dim c As New Calc\nPrint c.DoubleFactorial(5)",
        "240");
    // Nothing (the class-typed-variable default) rejects member access, Set
    // enforces the declared class, and `Is` compares identity.
    expect_classes_success(
        {{"Counter", "Public value As Long"}},
        "Dim c As Counter\nPrint CStr(c Is Nothing)\nDim c2 As New Counter\n"
        "Print CStr(c2 Is New Counter)",
        "True\nFalse");
    expect_classes_failure(
        {{"Counter", "Public value As Long"}},
        "Dim c As Counter\nCall c.Increment()", "WFC0106");
    expect_classes_failure({}, "Dim x As New Nope", "WFC0134");
    expect_classes_failure(
        {{"A", "Public v As Long"}, {"B", "Public v As Long"}},
        "Dim a As A\nDim b As New B\nSet a = b", "WFC0137");
    expect_classes_failure(
        {{"Foo", "Sub Bar(n As Long)\nEnd Sub"}},
        "Dim f As New Foo\nCall f.Bar()", "WFC0072");
    expect_classes_failure(
        {{"Foo", "Public x As Long\nSub x()\nEnd Sub"}}, "Print \"unused\"", "WFC0128");
    expect_classes_failure(
        {{"Foo", "Public x As Long"}}, "Dim f As New Foo\nPrint f.Nope", "WFC0135");
    // The Me keyword and the Class_Initialize/Class_Terminate lifecycle
    // hooks. Class_Initialize runs against a fully field-initialized
    // instance at New; Class_Terminate runs when the last reference to an
    // instance is dropped -- via Set (including Set x = Nothing), at the
    // end of a Sub/Function/Property call for that call's own locals, and
    // at the end of the program for any surviving module-level variable.
    expect_classes_success(
        {{"Counter", "Public n As Long\n\n"
                     "Sub Class_Initialize()\nn = 100\nEnd Sub\n\n"
                     "Sub Bump()\nn = n + 1\nPrint Me.n\nEnd Sub"}},
        "Dim c As New Counter\nCall c.Bump()\nCall c.Bump()",
        "101\n102");
    expect_classes_success(
        {{"Foo", "Sub Class_Terminate()\nPrint \"terminated\"\nEnd Sub"}},
        "Dim x As New Foo\nPrint \"before\"\nSet x = Nothing\nPrint \"after\"",
        "before\nterminated\nafter");
    expect_classes_success(
        {{"Foo", "Sub Class_Terminate()\nPrint \"local-terminated\"\nEnd Sub"}},
        "Sub MakeOne()\nDim x As New Foo\nPrint \"inside\"\nEnd Sub\n"
        "Call MakeOne()\nPrint \"outside\"",
        "inside\nlocal-terminated\noutside");
    expect_classes_success(
        {{"Foo", "Sub Class_Terminate()\nPrint \"program-end-terminated\"\nEnd Sub"}},
        "Dim x As New Foo\nPrint \"hi\"",
        "hi\nprogram-end-terminated");
    expect_classes_success(
        {{"Foo", "Public v As Long\n\n"
                 "Function GetSelf() As Variant\nSet GetSelf = Me\nEnd Function"}},
        "Dim x As New Foo\nx.v = 7\nDim y As Variant\nSet y = x.GetSelf()\n"
        "Print y.v & \" \" & CStr(y Is x)",
        "7 True");
    expect_classes_success(
        // Two same-frame variables aliasing the same instance still
        // terminate it exactly once, at whichever alias is drained last.
        {{"Foo", "Sub Class_Terminate()\nPrint \"terminated-once\"\nEnd Sub"}},
        "Sub Test()\nDim a As New Foo\nDim b As Variant\nSet b = a\n"
        "Print \"before end\"\nEnd Sub\nCall Test()\nPrint \"after\"",
        "before end\nterminated-once\nafter");
    expect_classes_failure({}, "Print Me", "WFC0138");
    expect_classes_failure({}, "Dim Me As Long", "WFC0017");
    expect_classes_failure(
        {{"Foo", "Sub Class_Initialize(n As Long)\nEnd Sub"}},
        "Print \"unused\"", "WFC0139");
    expect_classes_failure(
        {{"Foo", "Function Class_Terminate() As Long\nEnd Function"}},
        "Print \"unused\"", "WFC0139");
    // Class-module refinements (REQ-0205): an unqualified sibling write to
    // a Property Let (including when the property's own name collides with
    // its value parameter's name -- the local parameter must shadow it),
    // class-typed/As Object fields and Function/Property Get return types
    // (with cross-class forward references resolved regardless of --class
    // order), and indexed Property Get/Let/Set.
    expect_classes_success(
        {{"Box", "Public m_v As Long\n\n"
                 "Property Get V() As Long\nV = m_v\nEnd Property\n\n"
                 "Property Let V(v As Long)\nm_v = v\nEnd Property\n\n"
                 "Sub SetIt()\nV = 42\nEnd Sub"}},
        "Dim b As New Box\nb.V = 99\nPrint b.V\nCall b.SetIt()\nPrint b.V",
        "99\n42");
    expect_classes_success(
        {{"Counter", "Public n As Long"},
         {"Box", "Public m_v As Variant\n\n"
                 "Property Set V(v As Object)\nSet m_v = v\nEnd Property\n\n"
                 "Property Get V() As Variant\nSet V = m_v\nEnd Property\n\n"
                 "Sub Attach()\nSet V = New Counter\nEnd Sub"}},
        "Dim b As New Box\nCall b.Attach()\nPrint TypeName(b.V)",
        "Counter");
    expect_classes_success(
        {{"Counter", "Public n As Long"},
         {"Holder", "Public c As Counter"}},
        "Dim h As New Holder\nDim c As New Counter\nc.n = 5\nSet h.c = c\n"
        "Print h.c.n & \" \" & CStr(h.c Is c)",
        "5 True");
    expect_classes_failure(
        {{"A", "Public v As Long"}, {"B", "Public v As Long"}, {"Holder", "Public a As A"}},
        "Dim h As New Holder\nDim b As New B\nSet h.a = b", "WFC0137");
    expect_classes_success(
        {{"Counter", "Public n As Long"}, {"Holder", "Public obj As Object"}},
        "Dim h As New Holder\nDim c As New Counter\nSet h.obj = c\n"
        "Print TypeName(h.obj) & \" \" & h.obj.n",
        "Counter 0");
    expect_classes_success(
        // Holder references Counter before Counter is scanned -- the
        // two-pass scan_classes (register every class name, then scan
        // every body) resolves this regardless of --class order.
        {{"Holder", "Public c As Counter"}, {"Counter", "Public n As Long"}},
        "Dim h As New Holder\nPrint TypeName(h.c) & \" \" & CStr(h.c Is Nothing)",
        "Nothing True");
    expect_classes_success(
        {{"Counter", "Public n As Long"}},
        "Function MakeCounter() As Counter\nSet MakeCounter = New Counter\n"
        "MakeCounter.n = 7\nEnd Function\nDim c As Counter\nSet c = MakeCounter()\n"
        "Print c.n & \" \" & TypeName(c)",
        "7 Counter");
    expect_classes_failure(
        {{"Counter", "Public n As Long"}},
        "Function MakeCounter() As Counter\nMakeCounter = New Counter\nEnd Function\n"
        "Print MakeCounter().n",
        "WFC0108");
    expect_classes_success(
        {{"Pair", "Public a As Long\nPublic b As Long\n\n"
                  "Property Get Item(i As Long) As Long\n"
                  "If i = 0 Then\nItem = a\nElse\nItem = b\nEnd If\nEnd Property\n\n"
                  "Property Let Item(i As Long, v As Long)\n"
                  "If i = 0 Then\na = v\nElse\nb = v\nEnd If\nEnd Property"}},
        "Dim p As New Pair\np.Item(0) = 10\np.Item(1) = 20\n"
        "Print p.Item(0) & \" \" & p.Item(1)",
        "10 20");
    expect_classes_success(
        {{"Store", "Public m_v As Variant\n\n"
                   "Property Set Item(i As Long, v As Object)\nSet m_v = v\nEnd Property\n\n"
                   "Property Get Item(i As Long) As Variant\nSet Item = m_v\nEnd Property"},
         {"Counter", "Public n As Long"}},
        "Dim s As New Store\nDim c As New Counter\nc.n = 3\nSet s.Item(0) = c\n"
        "Print s.Item(0).n",
        "3");
    expect_classes_failure(
        {{"Pair", "Public a As Long\n\n"
                  "Property Get Item(i As Long) As Long\nItem = a\nEnd Property"}},
        "Dim p As New Pair\nPrint p.Item()", "WFC0072");
    // Optional parameters (with and without a default), ParamArray, Static
    // locals, and Public/Private class-member visibility (REQ-0206).
    expect_program_success(
        "Function Greet(name As String, Optional greeting As String = \"Hello\") As String\n"
        "Greet = greeting & \", \" & name\nEnd Function\n"
        "Print Greet(\"World\")\nPrint Greet(\"Bob\", \"Hi\")",
        "Hello, World\nHi, Bob");
    expect_program_success(
        "Function Add(a As Long, Optional b As Long) As Long\nAdd = a + b\nEnd Function\n"
        "Print Add(5)\nPrint Add(5, 10)",
        "5\n15");
    expect_program_failure(
        "Function Add(a As Long, Optional b As Long) As Long\nAdd = a + b\nEnd Function\n"
        "Print Add()",
        "WFC0072");
    expect_program_failure(
        "Sub Foo(Optional a As Long, b As Long)\nEnd Sub", "WFC0140");
    expect_program_failure(
        "Sub Foo(Optional a As Long = \"text\")\nEnd Sub", "WFC0016");
    expect_program_success(
        "Function Total(ParamArray nums() As Long) As Long\n"
        "Dim i As Long\nDim s As Long\ns = 0\n"
        "For i = LBound(nums) To UBound(nums)\ns = s + nums(i)\nNext i\n"
        "Total = s\nEnd Function\n"
        "Print Total(1, 2, 3, 4)\nPrint Total()",
        "10\n0");
    expect_program_success(
        "Function Sum2(base As Long, ParamArray nums() As Long) As Long\n"
        "Dim i As Long\nDim s As Long\ns = base\n"
        "For i = LBound(nums) To UBound(nums)\ns = s + nums(i)\nNext i\n"
        "Sum2 = s\nEnd Function\n"
        "Print Sum2(100, 1, 2, 3)",
        "106");
    expect_program_failure(
        "Sub Foo(ParamArray nums() As Long, x As Long)\nEnd Sub", "WFC0141");
    expect_program_success(
        "Function NextId() As Long\nStatic counter As Long\ncounter = counter + 1\n"
        "NextId = counter\nEnd Function\n"
        "Print NextId()\nPrint NextId()\nPrint NextId()",
        "1\n2\n3");
    expect_program_failure("Static x As Long", "WFC0144");
    expect_classes_success(
        {{"Foo", "Dim secret As Long\n\n"
                 "Sub SetSecret(v As Long)\nsecret = v\nEnd Sub\n\n"
                 "Function GetSecret() As Long\nGetSecret = secret\nEnd Function"}},
        "Dim f As New Foo\nCall f.SetSecret(42)\nPrint f.GetSecret()",
        "42");
    expect_classes_failure(
        {{"Foo", "Dim secret As Long\nSub SetSecret(v As Long)\nsecret = v\nEnd Sub"}},
        "Dim f As New Foo\nPrint f.secret", "WFC0142");
    expect_classes_failure(
        {{"Foo", "Private secret As Long\n\n"
                 "Public Sub SetSecret(v As Long)\nsecret = v\nEnd Sub\n\n"
                 "Private Function Helper() As Long\nHelper = secret * 2\nEnd Function"}},
        "Dim f As New Foo\nCall f.SetSecret(10)\nPrint f.Helper()", "WFC0142");
    expect_classes_success(
        {{"Foo", "Private secret As Long\n\n"
                 "Public Sub SetSecret(v As Long)\nsecret = v\nEnd Sub\n\n"
                 "Private Function Helper() As Long\nHelper = secret * 2\nEnd Function\n\n"
                 "Public Function DoubleSecret() As Long\nDoubleSecret = Helper()\n"
                 "End Function"}},
        "Dim f As New Foo\nCall f.SetSecret(10)\nPrint f.DoubleSecret()",
        "20");
    expect_classes_success(
        // Private is per-class, not per-instance: a Foo method may reach
        // another Foo instance's private field.
        {{"Foo", "Private v As Long\nPublic other As Foo\n\n"
                 "Sub SetV(n As Long)\nv = n\nEnd Sub\n\n"
                 "Function CompareToOther() As Boolean\n"
                 "CompareToOther = (v = other.v)\nEnd Function"}},
        "Dim a As New Foo\nDim b As New Foo\nCall a.SetV(5)\nCall b.SetV(5)\n"
        "Set a.other = b\nPrint a.CompareToOther()",
        "True");
    expect_classes_failure(
        {{"Foo", "Private v As Long\nSub SetV(n As Long)\nv = n\nEnd Sub"},
         {"Bar", "Public target As Foo\nFunction Peek() As Long\nPeek = target.v\nEnd Function"}},
        "Dim a As New Foo\nDim b As New Bar\nCall a.SetV(5)\nSet b.target = a\nPrint b.Peek()",
        "WFC0142");
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
    expect_program_failure("Dim value As Byte", "WFC0012");
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
    expect_program_success("Dim value%: value = 5: Print TypeName(value)", "Integer");
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
    expect_program_failure("Print LCase()", "WFC0072");
    expect_program_failure("Print LCase(\"a\", \"b\")", "WFC0072");
    expect_program_failure("Print UCase()", "WFC0072");
    expect_program_failure("Print UCase(\"a\", \"b\")", "WFC0072");
    expect_program_failure("Print LTrim()", "WFC0072");
    expect_program_failure("Print LTrim(\"a\", \"b\")", "WFC0072");
    expect_program_failure("Print RTrim()", "WFC0072");
    expect_program_failure("Print RTrim(\"a\", \"b\")", "WFC0072");
    expect_program_failure("Print Trim()", "WFC0072");
    expect_program_failure("Print Trim(\"a\", \"b\")", "WFC0072");
    expect_program_failure("Print Asc()", "WFC0072");
    expect_program_failure("Print Asc(\"a\", \"b\")", "WFC0072");
    expect_program_failure("Print Chr()", "WFC0072");
    expect_program_failure("Print Chr(1, 2)", "WFC0072");
    expect_program_failure("Print StrReverse()", "WFC0072");
    expect_program_failure("Print StrReverse(\"a\", \"b\")", "WFC0072");
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
    expect_program_failure("Print CStr()", "WFC0072");
    expect_program_failure("Print CStr(1, 2)", "WFC0072");
    expect_program_failure("Print CLng()", "WFC0072");
    expect_program_failure("Print CLng(1, 2)", "WFC0072");
    expect_program_failure("Print CBool()", "WFC0072");
    expect_program_failure("Print CBool(1, 2)", "WFC0072");
    expect_program_failure("Print CInt()", "WFC0072");
    expect_program_failure("Print CInt(1, 2)", "WFC0072");
    expect_program_failure("Print Val()", "WFC0072");
    expect_program_failure("Print Val(\"1\", \"2\")", "WFC0072");
    expect_program_failure("Print Str()", "WFC0072");
    expect_program_failure("Print Str(1, 2)", "WFC0072");
    expect_program_failure("Print Hex()", "WFC0072");
    expect_program_failure("Print Hex(1, 2)", "WFC0072");
    expect_program_failure("Print Oct()", "WFC0072");
    expect_program_failure("Print Oct(1, 2)", "WFC0072");
    expect_program_failure("Print Hex(\"a\")", "WFC0099");
    expect_program_failure("Print Oct(\"1e\")", "WFC0099");
    expect_program_failure("Print Hex(\"2147483648\")", "WFC0009");
    expect_program_failure("Print Oct(True)", "WFC0073");
    expect_program_failure("Print Hex(2147483648#)", "WFC0009");
    expect_program_failure("Print Str(\"a\")", "WFC0073");
    expect_program_failure("Print Val(42)", "WFC0073");
    expect_program_failure("Print Abs(\"42\")", "WFC0073");
    expect_program_failure("Print Sgn(False)", "WFC0073");
    expect_program_failure("Print Abs()", "WFC0072");
    expect_program_failure("Print Abs(1, 2)", "WFC0072");
    expect_program_failure("Print Sgn()", "WFC0072");
    expect_program_failure("Print Sgn(1, 2)", "WFC0072");
    expect_program_failure("Print Int()", "WFC0072");
    expect_program_failure("Print Int(1, 2)", "WFC0072");
    expect_program_failure("Print Fix()", "WFC0072");
    expect_program_failure("Print Fix(1, 2)", "WFC0072");
    expect_program_failure("Print Sqr()", "WFC0072");
    expect_program_failure("Print Sqr(1, 2)", "WFC0072");
    expect_program_failure("Print Sin()", "WFC0072");
    expect_program_failure("Print Sin(1, 2)", "WFC0072");
    expect_program_failure("Print Cos()", "WFC0072");
    expect_program_failure("Print Cos(1, 2)", "WFC0072");
    expect_program_failure("Print Tan()", "WFC0072");
    expect_program_failure("Print Tan(1, 2)", "WFC0072");
    expect_program_failure("Print Atn()", "WFC0072");
    expect_program_failure("Print Atn(1, 2)", "WFC0072");
    expect_program_failure("Print Exp()", "WFC0072");
    expect_program_failure("Print Exp(1, 2)", "WFC0072");
    expect_program_failure("Print Log()", "WFC0072");
    expect_program_failure("Print Log(1, 2)", "WFC0072");
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
    expect_program_failure("Print CByte()", "WFC0072");
    expect_program_failure("Print CByte(1, 2)", "WFC0072");
    expect_program_failure("Print CVar()", "WFC0072");
    expect_program_failure("Print CVar(1, 2)", "WFC0072");
    expect_program_failure("Print MacID(\"ABC\")", "WFC0100");
    expect_program_failure("Print MacID(42)", "WFC0073");
    expect_program_failure("Print MacID()", "WFC0072");
    expect_program_failure("Print MacID(\"AB\", \"CD\")", "WFC0072");
    expect_program_failure("Print IsNumeric()", "WFC0072");
    expect_program_failure("Print IsNumeric(1, 2)", "WFC0072");
    expect_program_failure("Print TypeName()", "WFC0072");
    expect_program_failure("Print TypeName(1, 2)", "WFC0072");
    expect_program_failure("Print VarType()", "WFC0072");
    expect_program_failure("Print VarType(1, 2)", "WFC0072");
    expect_program_failure("Print IsArray()", "WFC0072");
    expect_program_failure("Print IsArray(1, 2)", "WFC0072");
    expect_program_failure("Print IsObject()", "WFC0072");
    expect_program_failure("Print IsObject(1, 2)", "WFC0072");
    expect_program_failure("Print IsNull()", "WFC0072");
    expect_program_failure("Print IsNull(1, 2)", "WFC0072");
    expect_program_failure("Print IsEmpty()", "WFC0072");
    expect_program_failure("Print IsEmpty(1, 2)", "WFC0072");
    expect_program_failure("Print IsError()", "WFC0072");
    expect_program_failure("Print IsError(1, 2)", "WFC0072");
    expect_program_failure("Print IsMissing()", "WFC0072");
    expect_program_failure("Print IsMissing(1, 2)", "WFC0072");
    expect_program_failure("Print Error(1.5)", "WFC0073");
    expect_program_failure("Print Error(-1)", "WFC0101");
    expect_program_failure("Print Error(65536)", "WFC0101");
    expect_program_failure("Print Error(1, 2)", "WFC0072");
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
    expect_program_failure("Print Format()", "WFC0072");
    expect_program_failure("Print Format(1, 2, 3)", "WFC0072");
    expect_program_failure("Print Format(42, 5)", "WFC0073");
    expect_program_failure("Print Format(\"x\", \"Fixed\")", "WFC0073");
    expect_program_failure("Print Format(42, \"Nope\")", "WFC0102");
    expect_program_failure("Print Format(1e308, \"Percent\")", "WFC0009");
    expect_program_failure("Print Rnd(\"x\")", "WFC0073");
    expect_program_failure("Print Rnd(1, 2)", "WFC0072");
    expect_program_failure("Randomize \"x\"", "WFC0073");
    expect_program_failure("Dim Randomize As Long", "WFC0017");
    expect_program_failure("Dim x As Single: x = 1e40", "WFC0009");
    expect_program_failure("Const pi As Single = 1e40", "WFC0009");
    expect_program_failure("Print CSng(1e40)", "WFC0009");
    expect_program_failure("Print 1e38! * 1e38!", "WFC0009");
    expect_program_failure("Print 1! / 0!", "WFC0008");
    expect_program_failure("Dim x As Single: x = \"text\"", "WFC0016");
    expect_program_failure("Print 1.23456@", "WFC0006");
    expect_program_failure("Print 1.5E2@", "WFC0006");
    expect_program_failure("Print 922337203685478@", "WFC0006");
    expect_program_failure("Print 10@ / 0@", "WFC0008");
    expect_program_failure("Print 100000000000@ * 100000000000@", "WFC0009");
    expect_program_failure(
        "Print -922337203685477.5807@ - 0.0002@",
        "WFC0009");
    expect_program_failure("Dim x As Currency: x = \"text\"", "WFC0016");
    expect_program_failure("Print CCur()", "WFC0072");
    expect_program_failure("Print CCur(1, 2)", "WFC0072");
    expect_program_failure("Print CCur(\"abc\")", "WFC0103");
    expect_program_failure("Print CCur(1e40)", "WFC0009");
    expect_program_failure("Dim x As Integer: x = \"text\"", "WFC0016");
    expect_program_failure("Print CInt()", "WFC0072");
    expect_program_failure("Print CInt(1, 2)", "WFC0072");
    expect_program_failure("Print CInt(\"abc\")", "WFC0088");
    expect_program_failure("Print CInt(40000)", "WFC0009");
    expect_program_failure("Print CInt(Null)", "WFC0104");
    expect_program_failure("Print 5@ = \"text\"", "WFC0018");
    expect_program_failure("Print 5 / 0", "WFC0008");
    expect_program_failure("Print 1e308 + 1e308", "WFC0009");
    expect_program_failure("Print -1e308 - 1e308", "WFC0009");
    expect_program_failure("Print 1e308 * 10", "WFC0009");
    expect_program_failure("Print 1e308 / 1e-308", "WFC0009");
    expect_program_failure("Print CDbl(\"x\")", "WFC0095");
    expect_program_failure("Print CDbl(\"\")", "WFC0095");
    expect_program_failure("Print CDbl(\"1e9999\")", "WFC0009");
    expect_program_failure("Print CSng(\"1.2.3\")", "WFC0095");
    expect_program_failure("Print CSng(1e100)", "WFC0009");
    expect_program_failure("Print CSng(\"-1e100\")", "WFC0009");
    expect_program_failure("Print CDbl()", "WFC0072");
    expect_program_failure("Print CDbl(1, 2)", "WFC0072");
    expect_program_failure("Print CSng()", "WFC0072");
    expect_program_failure("Print CSng(1, 2)", "WFC0072");
    expect_program_failure("Print Sqr(-1)", "WFC0096");
    expect_program_failure("Print Log(0)", "WFC0096");
    expect_program_failure("Print Exp(1000)", "WFC0009");
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

    // Decimal: CDec conversion, exact 96-bit-mantissa arithmetic, and type
    // preservation through the numeric functions.
    expect_success("Print TypeName(CDec(2)) & \" \" & VarType(CDec(2))", "Decimal 14");
    expect_success("Print CDec(2) + CDec(3)", "5");
    expect_success("Print CDec(\"10.25\") * CDec(\"2\")", "20.5");
    expect_success("Print CDec(10) / CDec(4)", "2.5");
    expect_success(
        "Print CDec(1) / CDec(3)",
        "0.3333333333333333333333333333");
    expect_success(
        "Print CDec(\"123456789012345\") * CDec(\"123456789012345\")",
        "15241578753238669120562399025");
    expect_success("Print CDec(2.5) + CDec(2.5)", "5");
    expect_success("Print -CDec(5.5)", "-5.5");
    expect_success("Print CDec(5@)", "5");
    expect_success("Print CDec(True)", "-1");
    expect_success(
        "Print Abs(CDec(-5.5)) & \" \" & TypeName(Abs(CDec(-5.5)))",
        "5.5 Decimal");
    expect_success("Print Int(CDec(-2.5)) & \" \" & Fix(CDec(-2.5))", "-3 -2");
    expect_success("Print Round(CDec(2.5), 0)", "2");
    expect_success("Print Round(CDec(1.5), 0)", "2");
    expect_success(
        "Print Round(CDec(1.2345), 2) & \" \" & TypeName(Round(CDec(1.2345), 2))",
        "1.23 Decimal");
    expect_success("Print CLng(CDec(5.5))", "6");
    expect_success("Print CStr(CDec(10.5))", "10.5");
    expect_success(
        "Print CStr(CBool(CDec(0))) & \" \" & CStr(CBool(CDec(1)))",
        "False True");
    expect_success("Print IsNumeric(CDec(5))", "True");
    expect_success("Print Hex(CDec(255))", "FF");
    expect_program_failure("Print CDec(Null)", "WFC0104");
    expect_program_failure("Print CDec(\"notanumber\")", "WFC0105");
    expect_program_failure("Print CDec(1e40)", "WFC0009");
    expect_program_failure("Print CDec(10) / CDec(0)", "WFC0008");
    expect_program_failure("Print CDec()", "WFC0072");
    expect_program_failure("Print CDec(1, 2)", "WFC0072");

    // Decimal promotion order in mixed-type arithmetic: Decimal dominates
    // Single, Currency, Long, and (contrary to Double's dominance over every
    // other numeric type) Double too, in both operand orders. Verified
    // against a local VB6 6.00.8176 reference probe (see REQ-0198).
    expect_program_success(
        "Dim s As Single\ns = 1.5\nPrint TypeName(CDec(1) + s) & \" \" & (CDec(1) + s)",
        "Decimal 2.5");
    expect_program_success(
        "Dim s As Single\ns = 1.5\nPrint TypeName(s + CDec(1)) & \" \" & (s + CDec(1))",
        "Decimal 2.5");
    expect_program_success(
        "Dim s As Single\ns = 1.5\nPrint TypeName(CDec(1) * s) & \" \" & (CDec(1) * s)",
        "Decimal 1.5");
    expect_program_success(
        "Dim c As Currency\nc = 2.5\nPrint TypeName(CDec(1) + c) & \" \" & (CDec(1) + c)",
        "Decimal 3.5");
    expect_program_success(
        "Dim c As Currency\nc = 2.5\nPrint TypeName(c + CDec(1)) & \" \" & (c + CDec(1))",
        "Decimal 3.5");
    expect_success(
        "Print TypeName(CDec(1) + 3) & \" \" & (CDec(1) + 3)",
        "Decimal 4");
    expect_success(
        "Print TypeName(3 + CDec(1)) & \" \" & (3 + CDec(1))",
        "Decimal 4");
    expect_success(
        "Print TypeName(CDec(1) + 1234567.89) & \" \" & (CDec(1) + 1234567.89)",
        "Decimal 1234568.89");
    expect_success(
        "Print TypeName(1234567.89 + CDec(1)) & \" \" & (1234567.89 + CDec(1))",
        "Decimal 1234568.89");
    expect_success(
        "Print TypeName(CDec(1) / 3) & \" \" & (CDec(1) / 3)",
        "Decimal 0.3333333333333333333333333333");

    // Scalar Variant: Empty/Null literals, IsNull/IsEmpty, retyping
    // assignment, and three-valued-logic propagation through the operators.
    // Verified against the local VB6 6.00.8176 reference (see REQ-0197).
    expect_success("Print TypeName(Null) & \" \" & TypeName(Empty)", "Null Empty");
    expect_success("Print VarType(Null) & \" \" & VarType(Empty)", "1 0");
    expect_success("Print CStr(IsNull(Null)) & \" \" & CStr(IsEmpty(Empty))", "True True");
    expect_success("Print CStr(IsNull(Empty)) & \" \" & CStr(IsEmpty(Null))", "False False");
    expect_success("Print Null & \"x\"", "x");
    expect_success("Print \"x\" & Null", "x");
    expect_success("Print Empty & \"x\"", "x");
    expect_program_failure("Print Null & Null", "WFC0104");
    expect_program_failure("Print CBool(Null)", "WFC0104");
    expect_program_failure("Print CStr(Null)", "WFC0104");
    expect_program_success("Dim n: n = Empty + 5: Print n", "5");
    expect_program_success("Dim n: n = (Empty = 0): Print CStr(n)", "True");
    expect_program_success("Dim n: n = (Empty = \"\"): Print CStr(n)", "True");
    expect_program_success("Dim n: n = Null + 5: Print CStr(IsNull(n))", "True");
    expect_program_success("Dim n: n = (Null = 5): Print CStr(IsNull(n))", "True");
    expect_program_success("Dim n: n = (Null And False): Print CStr(IsNull(n))", "False");
    expect_program_success("Dim n: n = (Null And True): Print CStr(IsNull(n))", "True");
    expect_program_success("Dim n: n = (Null Or True): Print CStr(IsNull(n))", "False");
    expect_program_success("Dim n: n = (Null Or False): Print CStr(IsNull(n))", "True");
    expect_program_success("Dim n: n = (Not Null): Print CStr(IsNull(n))", "True");
    expect_program_success(
        "Dim x As Long\nx = 9\nIf Null Then\nx = 1\nElse\nx = 2\nEnd If\nPrint x",
        "2");
    expect_program_success("Dim x As Variant\nx = 5\nPrint TypeName(x)", "Long");
    expect_program_success(
        "Dim x As Variant\nx = 5\nx = \"hi\"\nPrint TypeName(x)",
        "String");
    expect_program_success(
        "Dim x As Variant\nx = 5\nx = 3.5\nx = True\nPrint TypeName(x)",
        "Boolean");
    expect_program_success("Dim x\nPrint TypeName(x)", "Empty");
    expect_program_success(
        "Dim x As Variant\nx = 1\nDim y As Long\ny = x\nPrint y",
        "1");
    expect_program_failure(
        "Dim x As Variant\nx = \"hi\"\nDim y As Long\ny = x",
        "WFC0016");

    if (failures != 0) {
        std::cerr << failures << " evaluator test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "all evaluator tests passed\n";
    return EXIT_SUCCESS;
}
