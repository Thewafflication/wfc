# REQ-0292 — Component persistence: `PropertyBag` and form files

**Content type:** Project requirement

**Status:** Accepted (first increment of MP-0005 persistence)

## Statement

- **`PropertyBag`.** `New PropertyBag` (also `As PropertyBag`) stores named
  Variant values. `WriteProperty Name, Value[, DefaultValue]` stores a value;
  when `DefaultValue` is given and equals the value (same subtype) the property
  is not stored and any earlier value is removed. `ReadProperty(Name[, Default])`
  returns the stored value with its original subtype (Integer, Long, Single,
  Double, Currency, Date, Boolean, Byte, String), else `Default`, else `Empty`.
  Names compare without regard to case. `Contents` reads and writes the bag as a
  byte array (a WFC-private text format, not the VB6 binary format) so one bag
  can be loaded into another. Writing an object raises 5.
- **Form files.** `ui::write_frm_header` writes the `VERSION` line and the form
  block in VB6 layout (three-space indents, strings with doubled quotes,
  `.frx` references as `"file":0000`). Parsing what it writes gives an equivalent
  form for every `.frm` in the test corpus. Comments in property lines are not
  preserved, and `.frx` data is referenced, not rewritten.

## Known limits

The VB6 binary property-bag format, persistable objects in a bag, and writing
new `.frx` records are not implemented.

## Verification

`TC-MP0002-corpus-176-property-bag` and `TC-MP0004-ui-units` (the round trip
over the corpus forms).
