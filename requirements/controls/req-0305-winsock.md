# REQ-0305 — Winsock control (`MSWinsockLib.Winsock`)

**Content type:** Project requirement

**Status:** Proposed (no VB6 reference probe; behavior follows the documented
control)

## Statement

- **Class.** `Begin MSWinsockLib.Winsock` forms (and `As MSWinsockLib.Winsock`)
  resolve to a native non-visual control over WinSock 2; no OCX is needed. The
  `Object=` line of `mswinsck.ocx` is accepted.
- **Members.** Properties `BytesReceived`, `LocalHostName`, `LocalIP`,
  `LocalPort`, `Protocol` (0 TCP, 1 UDP), `RemoteHost`, `RemoteHostIP`,
  `RemotePort`, `SocketHandle`, `State`; methods `Accept`, `Bind`, `Close`,
  `Connect`, `GetData`, `Listen`, `PeekData`, `SendData`; events `Close`,
  `Connect`, `ConnectionRequest`, `DataArrival`, `Error`, `SendComplete`,
  `SendProgress`; constants `sck*`.
- **States.** 0 closed, 1 open (bound), 2 listening, 6 connecting, 7 connected,
  8 closing, 9 error. `Connect` and `Listen` and `Bind` from a state other than
  closed raise 40020; `SendData` on an unconnected TCP socket raises 40006;
  `Protocol` cannot change while open (40020).
- **TCP.** `Connect` starts a non-blocking connection (state 6) and raises
  `Connect` when it completes; failure raises `Error` with the Winsock number and
  text and sets state 9. A listening control raises `ConnectionRequest` with a
  request id; `Accept` on a closed control adopts it (state 7); a request not
  accepted by the end of the handler is refused. Incoming bytes are buffered and
  `DataArrival(bytesTotal)` reports the buffered total; the peer closing raises
  `Close` (state 8). `SendData` raises `SendComplete`.
- **UDP.** `Bind` opens the socket (state 1); `SendData` sends a datagram to
  `RemoteHost` and `RemotePort`; an arriving datagram sets `RemoteHostIP` and
  `RemotePort` and raises `DataArrival`.
- **GetData.** `GetData(var [, type [, maxLen]])` removes bytes from the buffer
  into the variable as a String by default or as the Integer, Long, Single,
  Double or Byte `type` given; `PeekData` leaves them.
- **When events run.** On a real host a timer wakes the loop to poll sockets. A
  scripted headless run polls only in `pump <ms>`, `wait` and DoEvents so that
  event order is deterministic.

## Known limits

IPv4 only; no asynchronous name resolution (states 4 and 5 are not entered);
no `SendProgress`; no Byte-array `GetData` or `SendData`; no broadcast or
multicast options.

## Verification

Corpus `196-ocx-winsock-dialog` (TCP and UDP over loopback).
