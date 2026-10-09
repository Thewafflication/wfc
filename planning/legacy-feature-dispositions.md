# Legacy Feature Disposition Proposals

**Content type:** Decision proposal (for maintainer review)

**Status:** Partly decided — §1 (DDE) and §2 (`Data`/DAO) decided 2026-10-09;
§3–§6 still proposed. Each section ends with an acceptance record that stays blank until
the maintainer fills it in. The authoritative disposition table remains the one
in [`compatibility-profile-1.0.md`](compatibility-profile-1.0.md) ("Legacy
Features Requiring Explicit Disposition").

**Milestone:** MP-0002 exit gate — "no legacy candidate needed by MP-0003 or
MP-0004 lacks a controlled disposition".

**Author:** Claude, on behalf of the maintainer, 2026-10-01.

## How to read this

For each of the six candidates the profile asks one decision question. This
document answers it with a **recommendation**, lists the **alternatives** that
were weighed, states the **affected requirements** (taken from the discovery
inventory in `requirements/`), and spells out what would change in the
repository **on acceptance**. The recommendations share one design rule:

> Where a feature is not implemented, the *source-visible surface stays
> compilable* (members, properties, project entries and persisted values are
> accepted), and *using the missing behavior fails through one documented,
> trappable diagnostic* — never silently, never by pretending to succeed.

That rule is what `REQ-0130` already requires for an excluded DDE profile, and
it is what the approved `___MSJetSQLHelp` removal in the compatibility profile
established ("encountering a dependency shall produce a documented
unsupported-feature diagnostic").

### Proposed shared mechanism (needs the same acceptance)

* **One diagnostic family.** Reserve `WFC9000`–`WFC9099` for "documented
  unsupported legacy feature". Each carries a stable code, a message of the
  form `<Feature> is not supported by WFC 1.0 (see compatibility-profile
  "<anchor>")`, and a mapping to a *trappable VB run-time error number* so
  existing `On Error` handlers behave sensibly (numbers are given per feature
  below).
* **Loader behavior.** The project/form loader reports an unsupported project
  item with the same diagnostic family and names the item and file, rather
  than failing with a generic parse error. (The `.vbp` loader added in
  `REQ-0249` already rejects visual-designer entries this way.)
* **Persistence.** Properties of an unsupported feature are *read and written
  back unchanged* when forms are loaded and re-saved, so WFC never destroys
  data it does not understand.

## Summary

| # | Feature | Recommendation | Decision still needed from maintainer |
| --- | --- | --- | --- |
| 1 | DDE links and `Link*` members | ~~Disabled compatibility stub~~ **Decided 2026-10-09: deferred beyond 1.0** | None — decided |
| 2 | Intrinsic `Data` control and DAO/Jet | ~~Generic data binding retained; DAO/Jet engine removed~~ **Decided 2026-10-09: control and engine deferred to 2.0** | Whether generic binding (`REQ-0131`) stays in 1.0 |
| 3 | OLE1 conversion / `SaveToOle1File` | **Remove OLE1 conversion only; keep OLE2 hosting** | None beyond acceptance |
| 4 | `UserDocument` / ActiveX Documents | **Remove from 1.0 (deferred, not implemented)** | Confirm no corpus app ships an ActiveX Document |
| 5 | `PropertyPage` hosting | **Accept and persist at build time; no runtime hosting UI** | Confirm component authoring does not need design-time page hosting in 1.0 |
| 6 | WinHelp / context help | **Preserve IDs and API values; bridge to HTML Help (`.chm`); report `.hlp` as unsupported** | Confirm `.chm` bridging is wanted in 1.0 vs. deferred |

## 1. DDE links and `Link*` members

**Decision question:** Remove, provide a disabled compatibility stub, or
implement an optional legacy profile?

**What exists.** `REQ-0130` ("VB6 DDE link behavior") plus the `Link*`
members on `Form`, `MDIForm`, `PictureBox`, `Label` and `TextBox`
(`LinkMode`, `LinkTopic`, `LinkItem`, `LinkTimeout`, and the methods and
events `LinkExecute`, `LinkPoke`, `LinkRequest`, `LinkSend`, `LinkOpen`,
`LinkClose`, `LinkError`, `LinkNotify`). `REQ-0130` already states the
fallback: *"Where DDE is excluded, use of these members shall fail through one
documented compatibility diagnostic rather than appearing to succeed."*

**Recommendation — disabled compatibility stub.**

* Keep every `Link*` member declared with its reference name, type and
  DISPID so existing source and persisted forms load and compile.
* `LinkMode` reads `0` (`vbLinkNone`) and accepts assignment of `0`.
  Assigning a non-zero mode, or calling `LinkExecute`/`LinkPoke`/`LinkRequest`/
  `LinkSend`, raises VB run-time error **282** ("No foreign application
  responded to a DDE initiate") with the WFC unsupported-feature text. That is
  the error a real VB6 program already handles when a DDE server is not
  running, so well-written callers degrade correctly.
* A form that *persists* a non-zero `LinkMode`/`LinkTopic` loads with
  `LinkMode` forced to `0`, the values retained for re-save, and one load-time
  diagnostic.
* The `Link*` events never fire.

**Alternatives weighed.**

* *Remove outright.* Breaks compilation of any source that merely mentions a
  `Link*` member, contradicting the "source-visible surface stays compilable"
  rule and the exclusion text in `REQ-0130`.
* *Implement an optional legacy profile.* DDE is a Win32 messaging protocol
  (DDEML) used for inter-application scripting that modern applications do not
  expose; the test matrix (`REQ-0130` Verification) needs a controlled DDE
  client/server pair on all three architectures. High cost, no evidence of
  demand.

**Affected requirements:** `REQ-0130`; `REQ-0037`, `REQ-0038`, `REQ-0039`,
`REQ-0050`, `REQ-0057` (member inventories unchanged).

**On acceptance:** set `REQ-0130` to "Excluded from 1.0 — compatibility stub"
with the behavior above, add the `WFC90xx` entry, add a conformance test that
each stubbed member either returns the documented inert value or raises 282,
and record the variance in the release notes.

**Acceptance record:** _decision: **Deferred beyond 1.0** — DDE is
implemented, if at all, after all 1.0 milestones (after MP-0010). For 1.0,
`REQ-0130`'s excluded-profile clause governs: `Link*` members stay
source-visible and persisted values load, and use fails through one documented
diagnostic. The compatibility-stub details above (error 282, `LinkMode` forced
to `0`) are the planned shape of that diagnostic, finalized with the MP-0004
form work. date: 2026-10-09  by: maintainer_

## 2. Intrinsic `Data` control and DAO/Jet database behavior

**Decision question:** Retain generic binding only, implement DAO
compatibility, or remove the control?

**What exists.** `REQ-0062` (the `Data` class: 45 properties, 8 methods, 15
events), `REQ-0128` ("data control": connection, recordset, cursor, locking,
edit, BOF/EOF action, navigation, update, validation) and `REQ-0131` ("data
binding": `DataSource`, `DataMember`, `DataField`, `DataFormat`,
`DataChanged` on bound intrinsic controls). The `___MSJetSQLHelp` interface
was already removed (profile, "Approved Removal"), which narrows the Jet
surface but explicitly left the `Data` control and generic binding open.

**Recommendation — retain generic binding, remove the DAO/Jet engine.**

* **Keep** `REQ-0131`'s binding contract (`DataSource`/`DataMember`/
  `DataField`/`DataFormat`/`DataChanged`, current-record refresh, null and
  validation behavior) against a *provider-neutral* data-source interface —
  the same shape VB6 class modules can already implement as a "data source"
  (`GetDataMember`/`DataSourceBehavior`). This is the part existing
  applications use for bound controls and it is engine-agnostic.
* **Keep the `Data` control as a source-compatible shell**: all 45 properties
  and 15 events exist and round-trip through persistence. Navigation over an
  *in-memory* recordset supplied through the provider interface works.
* **Remove the Jet/DAO engine.** `Connect`, `DatabaseName`, `RecordSource`,
  `Options`, `Exclusive`, `ReadOnly` and `Refresh` against an `.mdb`/ISAM
  source raise VB/DAO error **3170** ("Could not find installable ISAM")
  with the WFC unsupported-feature text. `Database`/`Recordset` object
  accessors return `Nothing` until a provider is attached.

**Alternatives weighed.**

* *Implement DAO compatibility.* Requires an `.mdb` file engine (or a faithful
  reimplementation of the Jet engine and its SQL dialect), and ties 1.0 to a
  provider the maintainer has just removed the help/parser surface for.
  Largest cost of any item here; no corpus requirement identified.
* *Remove the control entirely.* Orphans `DataSource` bindings on every other
  intrinsic control and breaks form loading for any `.frm` that contains a
  `VB.Data` item.

**Affected requirements:** `REQ-0062`, `REQ-0128`, `REQ-0131`; binding
members listed in `REQ-0038`, `REQ-0039`, `REQ-0042`, `REQ-0044`, `REQ-0045`,
`REQ-0061`, `REQ-0063`, `REQ-0064`.

**On acceptance:** split `REQ-0128` into a retained shell/navigation part and
a removed engine part (status "Excluded from 1.0 — DAO/Jet"), keep `REQ-0131`
in scope, define the provider interface in the architecture documents, and add
tests for the 3170 stub and for binding over an in-memory provider.

**Acceptance record:** _decision: **Deferred to 2.0** — the intrinsic `Data`
control (`REQ-0062`, `REQ-0128`) and the DAO/Jet engine are not in 1.0. 2.0 is
expected to supply the database engine through SQLite or a WFC database engine
developed as a separate project, not a Jet reimplementation. For 1.0, a form
containing a `VB.Data` item, or code using the control, fails through one
documented unsupported-feature diagnostic. Generic data binding (`REQ-0131`) is
not covered by this decision and remains open. date: 2026-10-09  by:
maintainer_

## 3. OLE1 conversion and `SaveToOle1File`

**Decision question:** Remove only OLE1 conversion while retaining OLE2
hosting?

**What exists.** `REQ-0063` (the `OLE` control class) lists the single
`SaveToOle1File` method; `REQ-0129` ("OLE control") covers linked and embedded
OLE (OLE2) object hosting, activation, verbs, persistence and COM identity.

**Recommendation — remove only OLE1 conversion; keep OLE2 hosting.**

* `SaveToOle1File` stays declared (signature unchanged) and raises VB error
  **445** ("Object doesn't support this action") with the WFC
  unsupported-feature text.
* OLE1 *loading* (reading a persisted OLE1 object from an old form/stream) is
  excluded: the object loads as an opaque, preserved stream, is not
  activatable, and produces one load-time diagnostic.
* `REQ-0129` OLE2 behavior is unchanged and remains an MP-0004 obligation.

**Alternatives weighed.** Keeping OLE1 means a second, older storage and
activation protocol with no modern producers; removing OLE2 as well would
contradict the profile's COM-identity goals.

**Affected requirements:** `REQ-0063` (one member), `REQ-0129` (scope note).

**On acceptance:** annotate `REQ-0129` ("OLE1 excluded"), mark the
`SaveToOle1File` row stubbed in `REQ-0063`, add a stub conformance test.

**Acceptance record:** _decision: ____  date: ____  by: ____ _

## 4. `UserDocument` / ActiveX Documents

**Decision question:** Required by the accepted application corpus or
removable legacy host?

**What exists.** `REQ-0066` (the `UserDocument` class: 57 properties, 21
methods, 32 events) and the `UserDocument=` project-file item.

**Recommendation — remove from 1.0 (deferred, not implemented).**

ActiveX Documents are hosted by a document-object host (historically
Internet Explorer / the Office Binder) and compile to `.vbd` documents plus an
`.exe`/`.dll` server. The IE host they depend on is no longer present in
supported Windows releases, so even a perfect `UserDocument` implementation
would have nothing to run in. Proposed behavior:

* The project loader rejects a `UserDocument=`/`.dob` item with one
  unsupported-feature diagnostic that names the file (never a generic parse
  failure).
* `REQ-0066` is retained as a *deferred* contract (members inventoried, not
  implemented) so a later milestone can revive it if a corpus application
  proves a need.
* No stub class is provided; code that declares `As UserDocument` fails at
  compile time with the same diagnostic.

**Alternatives weighed.** A stub class would let such projects compile but
cannot run, which hides the problem; implementing the document-object
protocols is a large investment with no remaining host.

**The open question for the maintainer:** does any application in the
accepted corpus ship an ActiveX Document? If yes, this disposition must be
revisited before MP-0004.

**Affected requirements:** `REQ-0066`; project-loading (`REQ-0249`).

**On acceptance:** mark `REQ-0066` "Deferred beyond 1.0", add the loader
diagnostic and test.

**Acceptance record:** _decision: ____  date: ____  by: ____ _

## 5. `PropertyPage` hosting

**Decision question:** Required for component authoring or replaceable with
programmatic configuration?

**What exists.** `REQ-0065` (the `PropertyPage` class: 49 properties, 16
methods, 24 events) and the `PropertyPage=` project item; `REQ-0064`
(`UserControl`) references property pages through its `PropertyPages`
metadata.

**Recommendation — accept and persist at build time; no runtime hosting UI.**

Property pages exist so that the *design-time host* (the IDE or another
control container) can show a tabbed dialog for a control's properties. The
IDE and visual designer are already out of the 1.0 claim ("Existing Product
Exclusions"), and nothing at run time shows a property page. Proposed:

* The project loader and compiler accept `PropertyPage=` items and
  `PropertyPages` metadata, compile the class so its code is valid, and write
  the metadata into the component's type information unchanged (so a *third
  party's* designer can still use the component).
* WFC provides no page-hosting UI. A run-time call that asks the host to show
  property pages (for example `ShowPropertyPages`-style requests) raises VB
  error **445** with the unsupported-feature text.
* Programmatic configuration (setting properties in code or through the
  component's own properties) is the supported replacement.

**Alternatives weighed.** Hosting pages requires an `IPropertyPage`/
`OleCreatePropertyFrame` bridge and a property-sheet UI; that is a design-time
feature of exactly the kind the profile excludes.

**The open question for the maintainer:** is component authoring (producing
OCXs with property pages for *other* hosts) a 1.0 goal that needs the pages to
be fully functional inside WFC's own host?

**Affected requirements:** `REQ-0065`, `REQ-0064`.

**On acceptance:** mark `REQ-0065` "Compile-time only", add the loader/
compiler acceptance test and the run-time 445 stub test.

**Acceptance record:** _decision: ____  date: ____  by: ____ _

## 6. Legacy WinHelp and context-help integration

**Decision question:** Preserve IDs/API behavior, bridge to current help, or
report unsupported UI?

**What exists.** `HelpContextID` (19 occurrences across the control and form
inventories), `WhatsThisHelp`, `App.HelpFile`, and the `HelpFile`/`Context`
arguments of `MsgBox` and `InputBox` (`REQ-0076`, already accepted by the
evaluator as of `REQ-0262`). No requirement describes the WinHelp engine
itself.

**Recommendation — preserve IDs and API values; bridge to HTML Help; report
`.hlp` as unsupported.**

* **Preserve** every help property and argument: values round-trip through
  persistence, `App.HelpFile` is settable, and `MsgBox`/`InputBox` accept the
  `HelpFile`/`Context` pair (the evaluator already does).
* **Bridge** `.chm` help files to the current HTML Help API (`HtmlHelp`),
  mapping `HelpContextID` to a topic ID, for F1, the `MsgBox` Help button and
  `WhatsThisHelp` mode.
* **Report unsupported** for `.hlp` (WinHelp 32-bit) files with one
  unsupported-feature diagnostic, because `winhlp32.exe` is not shipped by
  supported Windows releases. The call fails visibly; it does not pretend help
  opened.

**Alternatives weighed.** Preserving values only (no bridge) is cheaper but
leaves F1 inert for applications that already ship `.chm` files; embedding a
WinHelp engine is not licensable or practical.

**The open question for the maintainer:** is `.chm` bridging a 1.0 deliverable
or deferred to a later milestone (the preservation part is cheap and can land
earlier regardless)?

**Affected requirements:** the `HelpContextID`/`WhatsThisHelp` rows in the
form and control contracts; `REQ-0076`.

**On acceptance:** add a help-integration requirement (property round-trip,
`.chm` mapping, `.hlp` diagnostic), and record the `.hlp` variance in the
release notes.

**Acceptance record:** _decision: ____  date: ____  by: ____ _

## What happens when these are accepted

1. Update the six rows of the profile's disposition table from "Proposed; not
   yet accepted" to the accepted disposition, citing this document and the
   decision date (change control: the profile requires the affected
   requirements and milestone allocations to change in the *same* controlled
   change).
2. Create the `WFC9000`-series diagnostic registry entries and the loader
   behavior described above.
3. Set the affected requirements' statuses as listed per section, add their
   stub/diagnostic conformance tests, and list every approved variance in the
   release notes ("Compatibility Acceptance", item 6).
4. Close the MP-0002 exit-gate item "no legacy candidate needed by MP-0003 or
   MP-0004 lacks a controlled disposition".

## Evidence and method

Counts and member names above come from the repository's own discovery
inventory (`requirements/vb/classes/`, `requirements/vb/behavior/`,
`requirements/vba/`). The recommendations are engineering judgment applied to
that inventory; they have **not** been checked against the accepted
application corpus (it is not in the repository), which is why sections 1, 2,
4 and 5 each end in an explicit question for the maintainer.
