Option Explicit
Sub Main()
    Print 5., 5. + 1, 5.E2, 3.!, 3.#, 1D3, .5, 3.5D2, &H10&, 1E+3!
    Print TypeName(5.), TypeName(3.!), TypeName(3.#), TypeName(1D3)
End Sub
