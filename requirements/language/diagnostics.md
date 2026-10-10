# Diagnostic Catalog

**Content type:** Project reference (diagnostic registry)

**Status:** Active — the authoritative list of `WFC` diagnostic codes. The `TC-MP0002-diagnostic-catalog` test fails when a code raised in `src/` is missing here, or an active code listed here is no longer raised.

## Format

A failed run reports one diagnostic:

```text
line 1, column 10: WFC0002 at byte 10: expected expression
```

A diagnostic that is also a catchable VB run-time error is followed by the VB form when it is not handled:

```text
line 1, column 21: WFC0008 at byte 21: division by zero
Run-time error '11': Division by zero
```

Many codes are shared by several related failures (for example `WFC0010` covers several "expected keyword" cases), so the message text is what distinguishes them. Messages may be reworded. Tools and tests that need a stable key should match the code, not the message.

## Stability rules

1. A code, once released in a commit on `master`, keeps its meaning and is never renumbered. Some existing codes are shared by several related failures; from now on a new kind of failure takes a new code rather than borrowing an existing one.
2. A code that stops being raised is moved to [Retired codes](#retired-codes) with the requirement that retired it; its number stays reserved.
3. A new diagnostic takes the next unused number in its range and is added to this catalog in the same change that introduces it, together with the requirement that defines it.
4. Changing whether a code is catchable by `On Error`, or the `Err.Number` it maps to, is a behavior change and is recorded in the defining requirement.

## Ranges

| Range | Use |
| --- | --- |
| `WFC0001`–`WFC0299` | Language and library diagnostics (syntax, binding, type, and run-time failures) |
| `WFC0300` | A VB run-time error carrying its own `Err.Number` and description |
| `WFC0301`–`WFC0399` | Control transfer, file, and directive diagnostics |
| `WFC0998`–`WFC0999` | Internal control signals; never reported to the user |
| `WFC9000`–`WFC9099` | Reserved: documented unsupported legacy features (see below) |

## Catchable diagnostics

A diagnostic with an `Err.Number` in the table below is a run-time error: an active `On Error` handler receives it with that number and VB6's standard description. Every other diagnostic stops the program and cannot be trapped. The mapping is implemented by `Interpreter::runtime_error_number()` in `src/interpreter/core.cpp`.

## Active codes

142 codes are raised by the current source.

| Code | Summary | `On Error` (`Err.Number`) | Requirements |
| --- | --- | --- | --- |
| `WFC0001` | Expected statement; expected Print statement | — | — |
| `WFC0002` | Expected expression | — | — |
| `WFC0003` | Unterminated string literal | — | — |
| `WFC0004` | Unexpected trailing input; expected line break; unsupported operator | — | — |
| `WFC0005` | Expected opening parenthesis after procedure name; expected closing parenthesis | — | `REQ-0160` |
| `WFC0006` | Integer literal out of range (and 10 more; see [Messages by code](#messages-by-code)) | — | `REQ-0185`, `REQ-0195`, `REQ-0196`, `REQ-0199`, `REQ-0279` |
| `WFC0007` | Operator requires numeric operands; operator requires integer operands | 13 (Type mismatch) | `REQ-0181` |
| `WFC0008` | Division by zero | 11 (Division by zero) | `REQ-0181`, `REQ-0195`, `REQ-0196`, `REQ-0198` |
| `WFC0009` | Numeric overflow; integer overflow | 6 (Overflow) | `REQ-0175`, `REQ-0181`, `REQ-0182`, `REQ-0183`, `REQ-0189`, `REQ-0193`, `REQ-0195`, `REQ-0196`, `REQ-0198`, `REQ-0199`, `REQ-0218`, `REQ-0223`, `REQ-0247` |
| `WFC0010` | Expected GoTo or GoSub (and 5 more; see [Messages by code](#messages-by-code)) | — | — |
| `WFC0011` | Expected parameter name (and 18 more; see [Messages by code](#messages-by-code)) | — | `REQ-0208`, `REQ-0224` |
| `WFC0012` | Expected As Integer, As Long, As Double, As Single, As Currency, As String, As Boolean, As Object, or As Variant (and 6 more; see [Messages by code](#messages-by-code)) | — | `REQ-0186`, `REQ-0214` |
| `WFC0013` | Duplicate variable or constant declaration; duplicate variable declaration | — | `REQ-0271` |
| `WFC0014` | Expected assignment operator (and 5 more; see [Messages by code](#messages-by-code)) | — | — |
| `WFC0015` | Undeclared variable; event is not declared in this class; undeclared procedure | — | `REQ-0202`, `REQ-0209`, `REQ-0213`, `REQ-0265`, `REQ-0272` |
| `WFC0016` | Default value type mismatch (and 9 more; see [Messages by code](#messages-by-code)) | 13 (Type mismatch) | `REQ-0184`, `REQ-0186`, `REQ-0195`, `REQ-0201`, `REQ-0202`, `REQ-0203`, `REQ-0206`, `REQ-0209`, `REQ-0210`, `REQ-0211`, `REQ-0215`, `REQ-0270` |
| `WFC0017` | Reserved keyword cannot be a parameter name; reserved keyword cannot be a variable name; reserved keyword cannot be a constant name | — | `REQ-0179`, `REQ-0204` |
| `WFC0018` | Like requires String operands (and 3 more; see [Messages by code](#messages-by-code)) | 13 (Type mismatch) | `REQ-0142`, `REQ-0156`, `REQ-0268` |
| `WFC0019` | Logical operator requires Boolean operands | 13 (Type mismatch) | `REQ-0142` |
| `WFC0020` | Concatenation requires String or Long operands | 13 (Type mismatch) | `REQ-0142` |
| `WFC0021` | If condition must be Boolean; IIf condition must be Boolean; Switch expressions must be Boolean | — | `REQ-0143` |
| `WFC0022` | Expected Then | — | `REQ-0143` |
| `WFC0023` | Expected Print or assignment branch | — | `REQ-0143` |
| `WFC0024` | Expected End If | — | `REQ-0144` |
| `WFC0025` | Expected If after End (and 5 more; see [Messages by code](#messages-by-code)) | — | `REQ-0144` |
| `WFC0026` | Duplicate Else | — | `REQ-0144` |
| `WFC0027` | Declarations are not supported in conditional blocks | — | `REQ-0144`, `REQ-0206`, `REQ-0271` |
| `WFC0028` | ElseIf condition must be Boolean | — | `REQ-0145` |
| `WFC0029` | Expected Then after ElseIf | — | `REQ-0145` |
| `WFC0030` | ElseIf is not permitted after Else | — | `REQ-0145` |
| `WFC0031` | While condition must be Boolean | — | `REQ-0146` |
| `WFC0032` | Expected Wend | — | `REQ-0146` |
| `WFC0033` | Unexpected Wend | — | `REQ-0146` |
| `WFC0035` | Do condition must be Boolean | — | `REQ-0147`, `REQ-0148` |
| `WFC0036` | Expected While or Until after Do | — | `REQ-0147` |
| `WFC0037` | Expected Loop | — | `REQ-0147` |
| `WFC0038` | Unexpected Loop | — | `REQ-0147` |
| `WFC0040` | Expected While or Until after Loop | — | `REQ-0148`, `REQ-0225` |
| `WFC0041` | Expected Do, For, Sub, Function, or Property after Exit | — | `REQ-0150`, `REQ-0152` |
| `WFC0042` | Exit Do is not inside a Do loop | — | `REQ-0150` |
| `WFC0043` | Expected For control variable; expected For Each control variable | — | `REQ-0151`, `REQ-0209` |
| `WFC0044` | Expected To | — | `REQ-0151` |
| `WFC0045` | For control variable must be numeric; For bounds and Step must be numeric | — | `REQ-0151` |
| `WFC0046` | Expected Next | — | `REQ-0151`, `REQ-0209` |
| `WFC0047` | For Step cannot be zero; For control variable overflow | — | `REQ-0151` |
| `WFC0048` | Unexpected Next | — | `REQ-0151` |
| `WFC0049` | Next variable does not match For | — | `REQ-0151`, `REQ-0209` |
| `WFC0052` | Exit For is not inside a For loop | — | `REQ-0152` |
| `WFC0053` | Case value must match selector type | 13 (Type mismatch) | `REQ-0153`, `REQ-0154`, `REQ-0156` |
| `WFC0054` | Expected Case after Select; expected End Select; expected Case or End Select | — | `REQ-0153` |
| `WFC0055` | Expected Select after End | — | `REQ-0153` |
| `WFC0056` | Duplicate Case Else | — | `REQ-0153` |
| `WFC0057` | Case is not permitted after Case Else | — | `REQ-0153` |
| `WFC0058` | Unexpected Case | — | `REQ-0153` |
| `WFC0059` | Expected Case value | — | `REQ-0154` |
| `WFC0060` | Case range requires same-type Long or String values | 13 (Type mismatch) | `REQ-0155` |
| `WFC0061` | Expected relational operator after Case Is | — | `REQ-0156` |
| `WFC0062` | Cannot assign to constant | — | `REQ-0157`, `REQ-0227` |
| `WFC0064` | Constant initializer cannot reference a variable | — | `REQ-0157`, `REQ-0227` |
| `WFC0065` | Expected Explicit, Compare, or Base after Option | — | `REQ-0158`, `REQ-0226` |
| `WFC0066` | Option directives must precede module statements | — | `REQ-0158`, `REQ-0226` |
| `WFC0067` | Duplicate Option Explicit | — | `REQ-0158` |
| `WFC0068` | Option directives are only valid at module level | — | `REQ-0158`, `REQ-0226` |
| `WFC0069` | Duplicate Option Compare | — | `REQ-0159` |
| `WFC0070` | Expected Binary or Text after Option Compare | — | `REQ-0159` |
| `WFC0071` | Unsupported function | — | `REQ-0160`, `REQ-0161` |
| `WFC0072` | Function received the wrong number of arguments (and 4 more; see [Messages by code](#messages-by-code)) | — | `REQ-0160`, `REQ-0161`, `REQ-0176`, `REQ-0177`, `REQ-0178`, `REQ-0180`, `REQ-0182`, `REQ-0183`, `REQ-0187`, `REQ-0188`, `REQ-0189`, `REQ-0191`, `REQ-0192`, `REQ-0193`, `REQ-0194`, `REQ-0196`, `REQ-0198`, `REQ-0199`, `REQ-0202`, `REQ-0203`, `REQ-0205`, `REQ-0206`, `REQ-0211`, `REQ-0213`, `REQ-0217`, `REQ-0218`, `REQ-0224` |
| `WFC0073` | Error requires a Long number (and 66 more; see [Messages by code](#messages-by-code)) | 13 (Type mismatch) | `REQ-0160`, `REQ-0161`, `REQ-0175`, `REQ-0176`, `REQ-0177`, `REQ-0178`, `REQ-0180`, `REQ-0183`, `REQ-0187`, `REQ-0188`, `REQ-0189`, `REQ-0191`, `REQ-0192`, `REQ-0193`, `REQ-0194`, `REQ-0196`, `REQ-0198`, `REQ-0199`, `REQ-0201`, `REQ-0218`, `REQ-0239` |
| `WFC0074` | Constant initializer cannot call a procedure; constant initializer cannot call a function | — | `REQ-0160`, `REQ-0161` |
| `WFC0075` | Function length cannot be negative | 5 (Invalid procedure call or argument) | `REQ-0163`, `REQ-0164` |
| `WFC0076` | InStr start must be positive; Replace start must be positive; Mid start must be positive | 5 (Invalid procedure call or argument) | `REQ-0164` |
| `WFC0077` | Asc requires a non-empty String | 5 (Invalid procedure call or argument) | `REQ-0165`, `REQ-0177` |
| `WFC0078` | ChrB code must be in the byte range; Chr code must be in the character range | 5 (Invalid procedure call or argument) | `REQ-0165`, `REQ-0177` |
| `WFC0079` | String fill code must be in the character range | 5 (Invalid procedure call or argument) | — |
| `WFC0080` | String requires a non-empty fill String | 5 (Invalid procedure call or argument) | — |
| `WFC0081` | Unsupported comparison method | — | `REQ-0167`, `REQ-0169` |
| `WFC0082` | Replace count must be -1 or non-negative | 5 (Invalid procedure call or argument) | `REQ-0168` |
| `WFC0083` | InStrRev start must be -1 or positive | 5 (Invalid procedure call or argument) | `REQ-0169` |
| `WFC0086` | CLng requires a numeric value | 13 (Type mismatch) | `REQ-0173` |
| `WFC0087` | CBool requires a Boolean or numeric value | 13 (Type mismatch) | `REQ-0174` |
| `WFC0088` | CInt requires a numeric value | 13 (Type mismatch) | `REQ-0199` |
| `WFC0091` | RGB component must be non-negative | 5 (Invalid procedure call or argument) | `REQ-0176` |
| `WFC0092` | QBColor index must be from 0 through 15 | 5 (Invalid procedure call or argument) | `REQ-0176` |
| `WFC0093` | StrConv conversion is not supported in the current model | — | `REQ-0178` |
| `WFC0094` | Round digit count must be non-negative | 5 (Invalid procedure call or argument) | `REQ-0180` |
| `WFC0095` | CDbl requires a numeric value; CSng requires a numeric value | 13 (Type mismatch) | `REQ-0182` |
| `WFC0096` | Sqr argument must be non-negative; Log argument must be positive | 5 (Invalid procedure call or argument) | `REQ-0183` |
| `WFC0097` | Type-declaration character requires an unsupported value type | — | `REQ-0186`, `REQ-0195`, `REQ-0196` |
| `WFC0098` | CByte requires a numeric value | 13 (Type mismatch) | `REQ-0175` |
| `WFC0099` | Hex requires a numeric value; Oct requires a numeric value | — | `REQ-0189` |
| `WFC0100` | MacID requires exactly four bytes | — | `REQ-0191` |
| `WFC0101` | Invalid procedure call or argument; Error number is outside the valid range | 5 (Invalid procedure call or argument) | `REQ-0192` |
| `WFC0103` | CCur requires a numeric value | 13 (Type mismatch) | `REQ-0196` |
| `WFC0104` | Invalid use of Null | 94 (Invalid use of Null) | `REQ-0172`, `REQ-0197`, `REQ-0198`, `REQ-0199` |
| `WFC0105` | CDec requires a numeric value | 13 (Type mismatch) | `REQ-0198` |
| `WFC0106` | Set requires an object reference; Invalid use of Nothing; Object parameter requires an object reference | 91 (Object variable or With block variable not set) | `REQ-0200`, `REQ-0203`, `REQ-0212`, `REQ-0217`, `REQ-0228`, `REQ-0229`, `REQ-0235`, `REQ-0236` |
| `WFC0107` | Is requires object operands; TypeOf requires an object operand; object comparison requires Is | 424 (Object required) | `REQ-0200` |
| `WFC0108` | Object assignment requires Set | — | `REQ-0200`, `REQ-0205`, `REQ-0212`, `REQ-0214`, `REQ-0234`, `REQ-0235` |
| `WFC0109` | Set requires an Object or Variant target | — | `REQ-0200` |
| `WFC0111` | Array subscript out of range | 9 (Subscript out of range) | `REQ-0201`, `REQ-0207`, `REQ-0208`, `REQ-0210`, `REQ-0212`, `REQ-0219` |
| `WFC0115` | ReDim dimension count does not match the array's declared dimension count; index count does not match array dimensions | — | `REQ-0201`, `REQ-0207`, `REQ-0208`, `REQ-0210`, `REQ-0212`, `REQ-0219` |
| `WFC0117` | Array lower bound must not exceed the upper bound | — | `REQ-0201`, `REQ-0207`, `REQ-0210`, `REQ-0212` |
| `WFC0118` | Expected property name; expected procedure name | — | `REQ-0202` |
| `WFC0119` | Duplicate or reserved procedure name | — | `REQ-0202` |
| `WFC0120` | Expected End Function; expected End Property (a `Property Get`) | — | `REQ-0202` |
| `WFC0121` | Expected End Sub; expected End Property (a `Property Let`/`Set`) | — | `REQ-0202` |
| `WFC0122` | A Sub cannot be used in an expression | — | `REQ-0202`, `REQ-0203`, `REQ-0213`, `REQ-0217` |
| `WFC0123` | Procedure call nesting is too deep | 28 (Out of stack space) | `REQ-0202`, `REQ-0203` |
| `WFC0124` | Exit Sub is not inside a Sub; Exit Property is not inside a Property | — | `REQ-0202`, `REQ-0203` |
| `WFC0125` | Exit Function is not inside a Function | — | `REQ-0202`, `REQ-0203` |
| `WFC0126` | Duplicate or reserved class name | — | `REQ-0203` |
| `WFC0127` | Expected a class member declaration (Dim, Public, Private, Sub, Function, or Property) | — | `REQ-0203` |
| `WFC0128` | Duplicate or reserved class member name; duplicate Property accessor for this name | — | `REQ-0203` |
| `WFC0129` | Expected Get, Let, or Set after Property | — | `REQ-0203` |
| `WFC0131` | Property Let/Set requires at least one parameter | — | `REQ-0203`, `REQ-0205` |
| `WFC0132` | Property Set's last parameter (the value) must be declared As Object | — | `REQ-0203`, `REQ-0205`, `REQ-0228` |
| `WFC0133` | Expected End Property | — | `REQ-0203` |
| `WFC0134` | Unknown class name | — | `REQ-0203`, `REQ-0233` |
| `WFC0135` | Unknown member; unknown member (no Property Set accessor) | — | `REQ-0203`, `REQ-0204`, `REQ-0205`, `REQ-0217`, `REQ-0233` |
| `WFC0136` | Member access requires an object reference; With requires an object reference; indexing requires an array | 424 (Object required) | `REQ-0203`, `REQ-0235`, `REQ-0236` |
| `WFC0137` | Set source does not match the target's declared class; argument does not match the parameter's declared class | — | `REQ-0203`, `REQ-0205`, `REQ-0212`, `REQ-0214`, `REQ-0228`, `REQ-0231`, `REQ-0233` |
| `WFC0138` | Me is only valid inside a class member | — | `REQ-0204` |
| `WFC0139` | Class_Initialize/Class_Terminate must be a parameterless Sub | — | `REQ-0204` |
| `WFC0140` | A required parameter cannot follow an Optional parameter | — | `REQ-0206` |
| `WFC0141` | ParamArray parameter must be declared as an array: name(); ParamArray requires an element type: As Integer, As Long, As Double, As Single, As Currency, As String, As Boolean, or As Variant; ParamArray must be the last parameter | — | `REQ-0206`, `REQ-0211` |
| `WFC0142` | Member is not accessible outside its class | — | `REQ-0206`, `REQ-0217`, `REQ-0233` |
| `WFC0144` | Static is only valid inside a Sub, Function, or Property | — | `REQ-0206` |
| `WFC0145` | ReDim requires an array bound; ReDim requires a previously declared dynamic array (Dim identifier()) | — | `REQ-0207` |
| `WFC0146` | Erase requires an array argument | — | `REQ-0208` |
| `WFC0147` | Expected As in Open statement (and 3 more; see [Messages by code](#messages-by-code)) | — | `REQ-0209` |
| `WFC0148` | LBound/UBound dimension is out of range | 9 (Subscript out of range) | `REQ-0210`, `REQ-0212` |
| `WFC0149` | Array parameter must be declared as an array: name() (and 6 more; see [Messages by code](#messages-by-code)) | — | `REQ-0211`, `REQ-0215`, `REQ-0231` |
| `WFC0150` | An array return type must be a fixed scalar type; expected closing parenthesis | — | `REQ-0216` |
| `WFC0151` | ReDim Preserve may only change a multi-dimensional array's last dimension | 9 (Subscript out of range) | `REQ-0219` |
| `WFC0152` | Duplicate Option Base | — | `REQ-0226` |
| `WFC0153` | Expected 0 or 1 after Option Base | — | `REQ-0226` |
| `WFC0300` | A VB run-time error raised by `Err.Raise`, the `Error` statement, or a run-time failure that already carries its VB error number; the message is the error description. | The raised number | — |
| `WFC0301` | Label not defined | — | — |
| `WFC0310` | A conditional-compilation directive (`#If`/`#ElseIf`/`#Const`) is malformed or cannot be evaluated; the message names the problem. | — | `REQ-0240` |
| `WFC0321` | Unsupported Open mode; expected For after Open path | — | — |
| `WFC0900` | Internal error: the evaluator hit an unexpected condition and stopped; the message names it. Please report it. | � | � |
| `WFC0998` | Internal: an `End` statement stopped the program. Never shown as an error. | — | — |
| `WFC0999` | Internal: a pending `GoTo`/`GoSub`/`Resume` jump unwinding to its target. Never shown as an error. | — | — |

## Retired codes

These numbers are reserved and must not be reused.

| Code | Formerly reported | Retired by | Originally defined in |
| --- | --- | --- | --- |
| `WFC0034` | A `Dim` declaration inside a `While` block | REQ-0271 (block-level `Dim` allowed) | `REQ-0146` |
| `WFC0039` | A `Dim` declaration inside a `Do` block | REQ-0271 (block-level `Dim` allowed) | `REQ-0147` |
| `WFC0050` | A `Dim` declaration inside a `For` block | REQ-0271 (block-level `Dim` allowed) | `REQ-0151` |
| `WFC0063` | A constant declaration inside a control-flow block | REQ-0271 (block-level declarations allowed) | `REQ-0157` |
| `WFC0089` | `Choose`/`Switch` would return `Null` | The Variant model returns `Null` as VB6 does | `REQ-0154` |
| `WFC0102` | Unsupported `Format` named style | REQ-0193 (unknown style text is a custom picture) | `REQ-0193` |
| `WFC0116` | Unsupported fixed-size array form | REQ-0207 (dynamic arrays) | `REQ-0201` |
| `WFC0130` | `Property Get` with parameters | REQ-0205 (indexed properties) | `REQ-0203` |

## Reserved: unsupported legacy features (`WFC9000`–`WFC9099`)

Reserved by the maintainer's legacy-feature decisions of 2026-10-09 ([`legacy-feature-dispositions.md`](../../planning/legacy-feature-dispositions.md)). Each will report `<Feature> is not supported by WFC 1.0` and map to the VB run-time error a VB6 program already handles for that situation. None is raised yet; numbers are assigned here when the owning milestone implements the diagnostic.

| Planned code | Feature | Planned `Err.Number` | Lands with |
| --- | --- | --- | --- |
| `WFC9001` | DDE links and `Link*` members (`REQ-0130`) | 282 | MP-0004 |
| `WFC9002` | Intrinsic `Data` control / DAO (`REQ-0062`, `REQ-0128`) | 3170 | MP-0004 |
| `WFC9003` | `SaveToOle1File` and OLE1 objects (`REQ-0063`) | 445 | MP-0005 |
| `WFC9004` | `UserDocument` / ActiveX Documents (`REQ-0066`) | — (load/compile error) | MP-0003 (project loading) |
| `WFC9005` | Showing property pages at run time (`REQ-0065`) | 445 | MP-0005 |
| `WFC9006` | Displaying help (`.chm`/`.hlp`) | — | MP-0004 |
| `WFC9007` | `___MSJetSQLHelp` dependency (approved removal) | — | No later than MP-0008 |

## Messages by code

Codes raised with more than three distinct messages, with every message text as written in the source.

- **`WFC0006`**: "integer literal out of range"; "invalid numeric literal"; "unterminated Date literal"; "invalid Date literal"; "numeric literal is malformed"; "Integer literal is out of range"; "Currency literal does not support exponent notation"; "Currency literal supports at most four decimal digits"; "Currency literal is out of range"; "Long literal suffix requires an integer"; "Integer literal suffix requires an integer"
- **`WFC0010`**: "expected GoTo or GoSub"; "expected Next after On Error Resume"; "expected GoTo or Resume after On Error"; "expected statement"; "expected Is after TypeOf operand"; "expected Get, Let or Set after Property"
- **`WFC0011`**: "expected parameter name"; "expected interface name after Implements"; "expected event name"; "expected field name"; "expected variable name"; "expected label after GoSub"; "expected label"; "expected label after GoTo"; "expected label after Resume"; "expected member name after '.'"; "expected Enum name"; "expected Enum member name"; "expected array name"; "expected constant name"; "expected variable name after Set"; "expected class name after Is"; "expected class name after New"; "expected procedure name after Call"; "IsMissing requires a parameter name"
- **`WFC0012`**: "expected As Integer, As Long, As Double, As Single, As Currency, As String, As Boolean, As Object, or As Variant"; "expected As Integer, As Long, As Double, As Single, As Currency, As String, As Boolean, As Object, As Variant, or a known class name"; "expected a fixed string length"; "type-declaration character cannot be combined with As"; "fixed String length must be 1 to 65526"; "expected a type after As"; "expected As Integer, As Long, As Double, As Single, As Currency, As String, or As Boolean"
- **`WFC0014`**: "expected assignment operator"; "expected start in Mid statement"; "expected comma after file number"; "expected comma before variable"; "expected comma in FileCopy"; "expected constant initializer"
- **`WFC0016`**: "default value type mismatch"; "assignment type mismatch"; "identifier type-declaration character mismatch"; "LSet/RSet require String operands"; "Mid statement requires String operands"; "Line Input requires a String or Variant variable"; "Enum member value must be a Long"; "constant initializer type mismatch"; "argument type mismatch"; "ByRef argument type mismatch"
- **`WFC0018`**: "Like requires String operands"; "comparison requires operands of the same type"; "Boolean ordering is not supported"; "ordering is not supported for this type"
- **`WFC0025`**: "expected If after End"; "expected End Type"; "expected End Enum"; "expected Enum after End"; "expected End With"; "expected With after End"
- **`WFC0072`**: "function received the wrong number of arguments"; "positional argument follows a named argument"; "argument specified more than once"; "procedure received the wrong number of arguments"; "function received an empty argument"
- **`WFC0073`**: "Error requires a Long number"; "On ... GoTo selector must be numeric"; "Err.Raise requires a Long number"; "SetAttr requires a path and attributes"; "Rnd requires a numeric argument"; "file number must be a Long"; "Mid requires Long arguments"; "file position must be a Long"; "Open requires a String path"; "path must be a String"; "FileCopy requires String paths"; "FileLen requires a String path"; "Environ requires a String name"; "Dir requires a String pattern"; "Input requires Long arguments"; "Spc/Tab requires a Long argument"; "Randomize requires a numeric seed"; "Type mismatch"; "Split requires a String delimiter"; "Split limit must be Long"; "Split requires a String argument"; "function requires a one-dimensional array argument"; "Join requires a String delimiter"; "Join element must be a scalar"; "Filter requires a String match"; "Filter include must be Boolean"; "Filter compare must be Long"; "Chr requires a Long argument"; "Abs requires a numeric argument"; "Sgn requires a numeric argument"; "QBColor requires a Long color index"; "RGB requires Long red, green, and blue components"; "LBound requires an array argument"; "UBound requires an array argument"; "LBound/UBound dimension must be Long"; "Int requires a numeric argument"; "Fix requires a numeric argument"; "math function requires a numeric argument"; "Round requires a numeric argument"; "Round requires a Long digit count"; "Format requires a String Style argument"; "Format with a Style argument requires a Long, Double, or Boolean expression"; "CStr does not accept an array argument"; "Choose requires a Long index"; "CByte requires a numeric argument"; "Error requires a Long argument"; "CDbl requires a numeric value"; "CSng requires a numeric value"; "CCur requires a numeric value"; "Space requires a Long argument"; "String requires a Long count and a Long or String fill"; "InStr start requires a Long argument"; "InStr requires String arguments"; "InStr compare requires a Long argument"; "InStrRev requires String arguments"; "InStrRev start and compare require Long arguments"; "StrComp requires String arguments"; "StrComp compare requires a Long argument"; "Replace requires String arguments"; "Replace start, count, and compare require Long arguments"; "Str requires a numeric argument"; "Hex requires a numeric argument"; "Oct requires a numeric argument"; "function requires a String argument"; "StrConv requires a Long conversion argument"; "function length requires a Long argument"; "Mid start and length require Long arguments"
- **`WFC0147`**: "expected As in Open statement"; "expected As in Name statement"; "expected In after For Each control variable"; "For Each requires an array"
- **`WFC0149`**: "array parameter must be declared as an array: name()"; "array parameter requires an explicit element type: As Integer, As Long, As Double, As Single, As Currency, As String, As Boolean, As Object, or As Variant"; "array parameters must be passed ByRef"; "array parameters cannot be Optional"; "a Static array must have fixed bounds (Static arr(n) As Type)"; "a Static array's element type must be a fixed scalar type, not Variant or Object"; "array argument must be a variable"
