Sub T(label As String, v As Variant)
    If IsNull(v) Then
        Print label & "=Null"
    Else
        Print label & "=" & v
    End If
End Sub
Sub Main()
    On Error Resume Next
    T "log0", Log(0)
    T "logneg", Log(-1)
    T "exp1000", Exp(1000)
    T "pow1024", 2 ^ 1024
    T "tan", Tan(1.5707963267949)
    T "atn", Atn(1E300)
    T "sin", Round(Sin(1E10), 8)
    T "dateadd_bad", DateAdd("zz", 1, #1/1/2020#)
    T "dateadd_null", DateAdd("d", 1, Null)
    T "datediff_rev", DateDiff("d", #1/10/2020#, #1/1/2020#)
    T "datepart_q", DatePart("q", #8/15/2020#)
    T "weekday_fd", Weekday(#1/1/2020#, vbMonday)
    T "dateserial_bad", DateSerial(2020, 13, 1)
    T "dateserial_0", DateSerial(2020, 0, 1)
    T "dateserial_year", DateSerial(30, 1, 1)
    T "dateserial_99", DateSerial(99, 1, 1)
    T "timeserial_over", TimeSerial(25, 61, 61)
    T "datevalue_bad", DateValue("garbage")
    T "year_null", Year(Null)
    T "month_str", Month("2020-05-17")
    T "cdate_num", CDate(0.5)
    T "cdate_big", CDate(2958465)
    T "cdate_over", CDate(2958466)
    T "date_min", CDate(-657434)
    T "cdate_str_t", CDate("12:30")
    T "cdate_str_dt", CDate("Jan 5, 2020")
    T "isdate_num", IsDate(5)
    T "hour_frac", Hour(0.75)
    T "datepart_w", DatePart("w", #1/5/2020#)
    T "dateadd_m_end", DateAdd("m", 1, #1/31/2020#)
    T "dateadd_y_leap", DateAdd("yyyy", 1, #2/29/2020#)
    T "format_dow", Format(#1/5/2020#, "dddd")
    T "mid_dec", CDec("1.5") * 2
    T "int_dec", Int(CDec(-1.5))
    T "round_dec", Round(CDec(2.5))
    T "large_mul", 99999999999# * 99999999999#
    T "cur_overflow", CCur(922337203685477#) * 2
    T "mod_neg", -7 Mod 3
    T "mod_float", 7.5 Mod 2
    T "intdiv_neg", -7 \ 2
    T "pow_neg_frac", (-8) ^ (1 / 3)
    T "zero_pow_zero", 0 ^ 0
    T "div_zero_float", 1 / 0#
End Sub
