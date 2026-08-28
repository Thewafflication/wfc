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

    if (failures != 0) {
        std::cerr << failures << " evaluator test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "all evaluator tests passed\n";
    return EXIT_SUCCESS;
}
