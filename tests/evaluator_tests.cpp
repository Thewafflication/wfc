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
    expect_success("Print \"&H\" & Hex(Asc(\"A\"))", "&H41");
    expect_success("Print \"[\" & Str(42) & \"]\"", "[ 42]");
    expect_success("Print \"[\" & Str$(-42) & \"]\"", "[-42]");
    expect_success("Print \"[\" & Str(0) & \"]\"", "[ 0]");
    expect_success("Print Len(Str(100))", "4");
    expect_success("Print Val(\"2457\")", "2457");
    expect_success("Print Val(\" 2 45 7th Street\")", "2457");
    expect_success("Print Val(\"- 42 trailing\")", "-42");
    expect_success("Print Val(\"+17\")", "17");
    expect_success("Print Val(\"words 42\")", "0");
    expect_success("Print Val(\"12,345\")", "12");
    expect_success("Print Val(\"   \")", "0");
    expect_success("Print Abs(-42)", "42");
    expect_success("Print Abs(0) + Abs(7)", "7");
    expect_success("Print Sgn(-42) & \" \" & Sgn(0) & \" \" & Sgn(42)", "-1 0 1");
    expect_success("Print Abs(Sgn(-100))", "1");
    expect_success("Print \"[\" & CStr(42) & \"]\"", "[42]");
    expect_success("Print CStr(True) & \" \" & CStr(False)", "True False");
    expect_success("Print CStr(\"already text\")", "already text");
    expect_success("Print Len(CStr(-100))", "4");
    expect_success("Print CLng(42)", "42");
    expect_success("Print CLng(True) & \" \" & CLng(False)", "-1 0");
    expect_success("Print CLng(\"  -42  \")", "-42");
    expect_success("Print CLng(\"+17\")", "17");
    expect_success("Print CLng(CStr(123)) + 1", "124");
    expect_program_success("Print CBool(True): Print CBool(False)", "True\nFalse");
    expect_program_success("Print CBool(42): Print CBool(0): Print CBool(-1)", "True\nFalse\nTrue");
    expect_program_success("Print CBool(\" true \"): Print CBool(\"FALSE\")", "True\nFalse");
    expect_program_success("Print CBool(\"-2\"): Print CBool(\"+0\")", "True\nFalse");
    expect_success("Print CByte(0) & \" \" & CByte(255)", "0 255");
    expect_success("Print CByte(42) + 1", "43");
    expect_success("Print CInt(-32768) & \" \" & CInt(32767)", "-32768 32767");
    expect_success("Print CInt(True) & \" \" & CInt(False)", "-1 0");
    expect_success("Print CInt(\"  -17  \") & \" \" & CInt(\"+42\")", "-17 42");
    expect_success("Print CInt(CStr(1000)) + 1", "1001");
    expect_program_success(
        "Print IsNumeric(42): Print IsNumeric(True): Print IsNumeric(\"  -17 \")",
        "True\nTrue\nTrue");
    expect_program_success(
        "Print IsNumeric(\"\"): Print IsNumeric(\"12x\"): Print IsNumeric(\"abc\")",
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
    expect_success("Print IIf(1 < 2, \"yes\", \"no\")", "yes");
    expect_success("Print IIf(1 > 2, \"yes\", \"no\")", "no");
    expect_success("Print IIf(True, 10, 20) + 1", "11");
    expect_success("Print Choose(2, \"a\", \"b\", \"c\")", "b");
    expect_success("Print Choose(1, 10, 20) + Choose(3, 1, 2, 3)", "13");

    expect_failure("", "WFC0001");
    expect_failure("Printer \"no\"", "WFC0001");
    expect_failure("Print", "WFC0002");
    expect_failure("Print value", "WFC0002");
    expect_failure("Print \"unterminated", "WFC0003");
    expect_failure("Print \"ok\" trailing", "WFC0004");
    expect_failure("Print (1 + 2", "WFC0005");
    expect_failure("Print 2147483648", "WFC0006");
    expect_failure("Print -2147483649", "WFC0006");
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
    expect_program_failure("Print Val(\"12.5\")", "WFC0084");
    expect_program_failure("Print Val(\"1E2\")", "WFC0084");
    expect_program_failure("Print Val(\"&H10\")", "WFC0085");
    expect_program_failure("Print Val(\"2147483648\")", "WFC0009");
    expect_program_failure("Print Abs(-2147483648)", "WFC0009");
    expect_program_failure("Print CLng(\"2147483648\")", "WFC0009");
    expect_program_failure("Print CLng(\"\")", "WFC0086");
    expect_program_failure("Print CLng(\"12 trailing\")", "WFC0086");
    expect_program_failure("Print CLng(\"12.5\")", "WFC0086");
    expect_program_failure("Print CBool(\"\")", "WFC0087");
    expect_program_failure("Print CBool(\"yes\")", "WFC0087");
    expect_program_failure("Print CBool(\"1.0\")", "WFC0087");
    expect_program_failure("Print CBool(\"2147483648\")", "WFC0009");
    expect_program_failure("Print CByte(-1)", "WFC0009");
    expect_program_failure("Print CByte(256)", "WFC0009");
    expect_program_failure("Print CByte(\"1\")", "WFC0073");
    expect_program_failure("Print CInt(32768)", "WFC0009");
    expect_program_failure("Print CInt(-32769)", "WFC0009");
    expect_program_failure("Print CInt(\"40000\")", "WFC0009");
    expect_program_failure("Print CInt(\"\")", "WFC0088");
    expect_program_failure("Print CInt(\"12.5\")", "WFC0088");
    expect_program_failure("Print IIf(42, \"a\", \"b\")", "WFC0021");
    expect_program_failure("Print Choose(0, \"a\", \"b\")", "WFC0089");
    expect_program_failure("Print Choose(3, \"a\", \"b\")", "WFC0089");
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
