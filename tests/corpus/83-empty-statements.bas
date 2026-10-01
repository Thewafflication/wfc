Option Explicit
Sub Main()
	Dim x As Integer  ' trailing comment
	x = 3 : : x = x + 1 :
	IF x = 4 THEN
		Print "four" ; _
		  "!"
	ELSEIF x > 4 THEN
		Print "big"
	ELSE
		print "small"
	END IF
	SELECT CASE x
		CASE 4 : Print "c4"
	END SELECT
	Rem done
	For x = 1 To 2 : Print x : Next x
End Sub
