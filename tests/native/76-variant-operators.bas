' Variant operators over many pairs of values (the interpreter is
' the oracle): the result with its type, or the error number.
Option Explicit
Dim r As Variant

Sub Show(label As String)
    If Err.Number <> 0 Then
        Print label; " -> error "; Err.Number
        Err.Clear
    Else
        Print label; " -> "; TypeName(r); ":"; r
    End If
End Sub

Sub Check(a As Variant, b As Variant, la As String)
    On Error Resume Next
    r = "?": r = a + b: Show la & " +"
    r = "?": r = a - b: Show la & " -"
    r = "?": r = a * b: Show la & " *"
    r = "?": r = a / b: Show la & " /"
    r = "?": r = a \ b: Show la & " \"
    r = "?": r = a Mod b: Show la & " Mod"
    r = "?": r = a & b: Show la & " &"
    r = "?": r = a ^ b: Show la & " ^"
    r = "?": r = a = b: Show la & " ="
    r = "?": r = a <> b: Show la & " <>"
    r = "?": r = a < b: Show la & " <"
    r = "?": r = a > b: Show la & " >"
    r = "?": r = a <= b: Show la & " <="
    r = "?": r = a >= b: Show la & " >="
    r = "?": r = a And b: Show la & " And"
    r = "?": r = a Or b: Show la & " Or"
    r = "?": r = a Xor b: Show la & " Xor"
    r = "?": r = -a: Show "-" & la
    r = "?": r = Not a: Show "Not " & la
End Sub

Sub Main()
    Check Empty, Empty, "Empty|Empty"
    Check Empty, Null, "Empty|Null"
    Check Empty, 7, "Empty|7"
    Check Empty, -3, "Empty|-3"
    Check Empty, 70000&, "Empty|70000&"
    Check Empty, 2147483647&, "Empty|2147483647&"
    Check Empty, 2.5, "Empty|2.5"
    Check Empty, 1.5!, "Empty|1.5!"
    Check Empty, "12", "Empty|12"
    Check Empty, "abc", "Empty|abc"
    Check Empty, "", "Empty|"
    Check Empty, True, "Empty|True"
    Check Empty, False, "Empty|False"
    Check Empty, 32767, "Empty|32767"
    Check Empty, "2.5e1", "Empty|2.5e1"
    Check Null, Empty, "Null|Empty"
    Check Null, Null, "Null|Null"
    Check Null, 7, "Null|7"
    Check Null, -3, "Null|-3"
    Check Null, 70000&, "Null|70000&"
    Check Null, 2147483647&, "Null|2147483647&"
    Check Null, 2.5, "Null|2.5"
    Check Null, 1.5!, "Null|1.5!"
    Check Null, "12", "Null|12"
    Check Null, "abc", "Null|abc"
    Check Null, "", "Null|"
    Check Null, True, "Null|True"
    Check Null, False, "Null|False"
    Check Null, 32767, "Null|32767"
    Check Null, "2.5e1", "Null|2.5e1"
    Check 7, Empty, "7|Empty"
    Check 7, Null, "7|Null"
    Check 7, 7, "7|7"
    Check 7, -3, "7|-3"
    Check 7, 70000&, "7|70000&"
    Check 7, 2147483647&, "7|2147483647&"
    Check 7, 2.5, "7|2.5"
    Check 7, 1.5!, "7|1.5!"
    Check 7, "12", "7|12"
    Check 7, "abc", "7|abc"
    Check 7, "", "7|"
    Check 7, True, "7|True"
    Check 7, False, "7|False"
    Check 7, 32767, "7|32767"
    Check 7, "2.5e1", "7|2.5e1"
    Check -3, Empty, "-3|Empty"
    Check -3, Null, "-3|Null"
    Check -3, 7, "-3|7"
    Check -3, -3, "-3|-3"
    Check -3, 70000&, "-3|70000&"
    Check -3, 2147483647&, "-3|2147483647&"
    Check -3, 2.5, "-3|2.5"
    Check -3, 1.5!, "-3|1.5!"
    Check -3, "12", "-3|12"
    Check -3, "abc", "-3|abc"
    Check -3, "", "-3|"
    Check -3, True, "-3|True"
    Check -3, False, "-3|False"
    Check -3, 32767, "-3|32767"
    Check -3, "2.5e1", "-3|2.5e1"
    Check 70000&, Empty, "70000&|Empty"
    Check 70000&, Null, "70000&|Null"
    Check 70000&, 7, "70000&|7"
    Check 70000&, -3, "70000&|-3"
    Check 70000&, 70000&, "70000&|70000&"
    Check 70000&, 2147483647&, "70000&|2147483647&"
    Check 70000&, 2.5, "70000&|2.5"
    Check 70000&, 1.5!, "70000&|1.5!"
    Check 70000&, "12", "70000&|12"
    Check 70000&, "abc", "70000&|abc"
    Check 70000&, "", "70000&|"
    Check 70000&, True, "70000&|True"
    Check 70000&, False, "70000&|False"
    Check 70000&, 32767, "70000&|32767"
    Check 70000&, "2.5e1", "70000&|2.5e1"
    Check 2147483647&, Empty, "2147483647&|Empty"
    Check 2147483647&, Null, "2147483647&|Null"
    Check 2147483647&, 7, "2147483647&|7"
    Check 2147483647&, -3, "2147483647&|-3"
    Check 2147483647&, 70000&, "2147483647&|70000&"
    Check 2147483647&, 2147483647&, "2147483647&|2147483647&"
    Check 2147483647&, 2.5, "2147483647&|2.5"
    Check 2147483647&, 1.5!, "2147483647&|1.5!"
    Check 2147483647&, "12", "2147483647&|12"
    Check 2147483647&, "abc", "2147483647&|abc"
    Check 2147483647&, "", "2147483647&|"
    Check 2147483647&, True, "2147483647&|True"
    Check 2147483647&, False, "2147483647&|False"
    Check 2147483647&, 32767, "2147483647&|32767"
    Check 2147483647&, "2.5e1", "2147483647&|2.5e1"
    Check 2.5, Empty, "2.5|Empty"
    Check 2.5, Null, "2.5|Null"
    Check 2.5, 7, "2.5|7"
    Check 2.5, -3, "2.5|-3"
    Check 2.5, 70000&, "2.5|70000&"
    Check 2.5, 2147483647&, "2.5|2147483647&"
    Check 2.5, 2.5, "2.5|2.5"
    Check 2.5, 1.5!, "2.5|1.5!"
    Check 2.5, "12", "2.5|12"
    Check 2.5, "abc", "2.5|abc"
    Check 2.5, "", "2.5|"
    Check 2.5, True, "2.5|True"
    Check 2.5, False, "2.5|False"
    Check 2.5, 32767, "2.5|32767"
    Check 2.5, "2.5e1", "2.5|2.5e1"
    Check 1.5!, Empty, "1.5!|Empty"
    Check 1.5!, Null, "1.5!|Null"
    Check 1.5!, 7, "1.5!|7"
    Check 1.5!, -3, "1.5!|-3"
    Check 1.5!, 70000&, "1.5!|70000&"
    Check 1.5!, 2147483647&, "1.5!|2147483647&"
    Check 1.5!, 2.5, "1.5!|2.5"
    Check 1.5!, 1.5!, "1.5!|1.5!"
    Check 1.5!, "12", "1.5!|12"
    Check 1.5!, "abc", "1.5!|abc"
    Check 1.5!, "", "1.5!|"
    Check 1.5!, True, "1.5!|True"
    Check 1.5!, False, "1.5!|False"
    Check 1.5!, 32767, "1.5!|32767"
    Check 1.5!, "2.5e1", "1.5!|2.5e1"
    Check "12", Empty, "12|Empty"
    Check "12", Null, "12|Null"
    Check "12", 7, "12|7"
    Check "12", -3, "12|-3"
    Check "12", 70000&, "12|70000&"
    Check "12", 2147483647&, "12|2147483647&"
    Check "12", 2.5, "12|2.5"
    Check "12", 1.5!, "12|1.5!"
    Check "12", "12", "12|12"
    Check "12", "abc", "12|abc"
    Check "12", "", "12|"
    Check "12", True, "12|True"
    Check "12", False, "12|False"
    Check "12", 32767, "12|32767"
    Check "12", "2.5e1", "12|2.5e1"
    Check "abc", Empty, "abc|Empty"
    Check "abc", Null, "abc|Null"
    Check "abc", 7, "abc|7"
    Check "abc", -3, "abc|-3"
    Check "abc", 70000&, "abc|70000&"
    Check "abc", 2147483647&, "abc|2147483647&"
    Check "abc", 2.5, "abc|2.5"
    Check "abc", 1.5!, "abc|1.5!"
    Check "abc", "12", "abc|12"
    Check "abc", "abc", "abc|abc"
    Check "abc", "", "abc|"
    Check "abc", True, "abc|True"
    Check "abc", False, "abc|False"
    Check "abc", 32767, "abc|32767"
    Check "abc", "2.5e1", "abc|2.5e1"
    Check "", Empty, "|Empty"
    Check "", Null, "|Null"
    Check "", 7, "|7"
    Check "", -3, "|-3"
    Check "", 70000&, "|70000&"
    Check "", 2147483647&, "|2147483647&"
    Check "", 2.5, "|2.5"
    Check "", 1.5!, "|1.5!"
    Check "", "12", "|12"
    Check "", "abc", "|abc"
    Check "", "", "|"
    Check "", True, "|True"
    Check "", False, "|False"
    Check "", 32767, "|32767"
    Check "", "2.5e1", "|2.5e1"
    Check True, Empty, "True|Empty"
    Check True, Null, "True|Null"
    Check True, 7, "True|7"
    Check True, -3, "True|-3"
    Check True, 70000&, "True|70000&"
    Check True, 2147483647&, "True|2147483647&"
    Check True, 2.5, "True|2.5"
    Check True, 1.5!, "True|1.5!"
    Check True, "12", "True|12"
    Check True, "abc", "True|abc"
    Check True, "", "True|"
    Check True, True, "True|True"
    Check True, False, "True|False"
    Check True, 32767, "True|32767"
    Check True, "2.5e1", "True|2.5e1"
    Check False, Empty, "False|Empty"
    Check False, Null, "False|Null"
    Check False, 7, "False|7"
    Check False, -3, "False|-3"
    Check False, 70000&, "False|70000&"
    Check False, 2147483647&, "False|2147483647&"
    Check False, 2.5, "False|2.5"
    Check False, 1.5!, "False|1.5!"
    Check False, "12", "False|12"
    Check False, "abc", "False|abc"
    Check False, "", "False|"
    Check False, True, "False|True"
    Check False, False, "False|False"
    Check False, 32767, "False|32767"
    Check False, "2.5e1", "False|2.5e1"
    Check 32767, Empty, "32767|Empty"
    Check 32767, Null, "32767|Null"
    Check 32767, 7, "32767|7"
    Check 32767, -3, "32767|-3"
    Check 32767, 70000&, "32767|70000&"
    Check 32767, 2147483647&, "32767|2147483647&"
    Check 32767, 2.5, "32767|2.5"
    Check 32767, 1.5!, "32767|1.5!"
    Check 32767, "12", "32767|12"
    Check 32767, "abc", "32767|abc"
    Check 32767, "", "32767|"
    Check 32767, True, "32767|True"
    Check 32767, False, "32767|False"
    Check 32767, 32767, "32767|32767"
    Check 32767, "2.5e1", "32767|2.5e1"
    Check "2.5e1", Empty, "2.5e1|Empty"
    Check "2.5e1", Null, "2.5e1|Null"
    Check "2.5e1", 7, "2.5e1|7"
    Check "2.5e1", -3, "2.5e1|-3"
    Check "2.5e1", 70000&, "2.5e1|70000&"
    Check "2.5e1", 2147483647&, "2.5e1|2147483647&"
    Check "2.5e1", 2.5, "2.5e1|2.5"
    Check "2.5e1", 1.5!, "2.5e1|1.5!"
    Check "2.5e1", "12", "2.5e1|12"
    Check "2.5e1", "abc", "2.5e1|abc"
    Check "2.5e1", "", "2.5e1|"
    Check "2.5e1", True, "2.5e1|True"
    Check "2.5e1", False, "2.5e1|False"
    Check "2.5e1", 32767, "2.5e1|32767"
    Check "2.5e1", "2.5e1", "2.5e1|2.5e1"
End Sub
