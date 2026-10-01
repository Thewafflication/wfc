DefInt A-C
DefStr S
DefDbl X-Z
Function Half(n)
    Half = n / 2
End Function
Function Describe(a, sname)
    Describe = TypeName(a) & "/" & TypeName(sname)
End Function
Sub Main()
    Dim alpha, sx, xray, other
    Print TypeName(alpha), TypeName(sx), TypeName(xray), TypeName(other)
    b = 4000
    Print TypeName(b)
    cc = 7: cc = cc * 2
    Print cc, TypeName(cc)
    sname = "q": Print TypeName(sname)
    z = 1: Print z / 3
    Print Describe(1, "x")
    More
End Sub
Function Greet$(n$)
    Greet$ = "hi " & n$
End Function
Function Twice%(n%)
    Twice% = n% * 2
End Function
Function Implicit(a)
    Implicit = a & "!"
End Function
Sub More()
    Print Greet$("bob"), Twice%(4), Greet("al"), Implicit("q")
End Sub
