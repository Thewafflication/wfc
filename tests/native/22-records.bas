Type Point
    x As Long
    y As Long
End Type

Type Player
    name As String
    score As Double
    where As Point
    alive As Boolean
End Type

Dim origin As Point
Dim champion As Player

Sub Move(p As Point, ByVal dx As Long, ByVal dy As Long)
    p.x = p.x + dx
    p.y = p.y + dy
End Sub

Sub Describe(who As Player)
    Print who.name; " scored "; who.score; " at ("; who.where.x; ","; who.where.y; ")"
End Sub

Sub Reward(who As Player, ByRef amount As Double)
    who.score = who.score + amount
    amount = 0
End Sub

Sub Main()
    Dim a As Point, b As Point
    Dim hero As Player, copy As Player
    Dim bonus As Double
    a.x = 3
    a.y = 4
    b = a
    b.x = b.x * 10
    Print a.x; a.y; b.x; b.y
    Move a, 5, -1
    Print a.x; a.y
    origin.x = 7
    Move origin, 1, 1
    Print origin.x; origin.y
    hero.name = "Ada"
    hero.score = 1.5
    hero.where = a
    hero.alive = True
    bonus = 2.25
    Reward hero, bonus
    Print bonus
    Describe hero
    copy = hero
    copy.name = copy.name & " II"
    copy.where.x = 99
    Describe hero
    Describe copy
    champion = copy
    champion.name = "Champ"
    Print champion.name; copy.name; hero.name
    Print hero.alive; champion.where.y
    Print Len(hero.name) + Len(copy.name)
    Move hero.where, 1, 1
    Print hero.where.x; hero.where.y
End Sub
