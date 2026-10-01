# REQ-0272 — Class events: `Event`, `RaiseEvent`, `WithEvents`

## Statement

- A class module may declare `[Public] Event Name(params)`.
- `Private|Public|Dim WithEvents x As Source` declares an object field whose
  `x_Name` procedures handle `Source`'s `Name` event.
- `Set x = obj` subscribes the holding instance to `obj`'s events and
  unsubscribes it from the previous referent; `Set x = Nothing` unsubscribes.
- `RaiseEvent Name(args)` inside the declaring class runs each subscribed
  handler synchronously, in subscription order. ByRef arguments (for example
  `Cancel As Boolean`) are visible to the raiser after each handler.
- Subscriptions hold the handler instance weakly, so a handler does not keep
  itself alive through its source. An event with no subscriber is a no-op.

## Scope

`WithEvents` is supported on class-module fields only (as in VB6, not in
standard modules). Argument types of `RaiseEvent` are not checked against the
event declaration; they are checked against each handler's parameters.
`RaiseEvent` of an undeclared event is `WFC0015`.

## Verification

Corpus `23-events`.
