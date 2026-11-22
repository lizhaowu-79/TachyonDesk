# Automatic Reverse Connection & TachyonDesk Repeater

TachyonDesk Server supports automatic reverse-repeater connections alongside the clipboard, file manager, GPU video, single-window, and remote audio features. Automatic reverse connection is disabled by default.

## Server Configuration (`tachyon.ini`)

Add the following section to the server's `tachyon.ini`:

```ini
[auto_reverse]
AutoReverseEnabled=1
AutoReverseRepeaterHost=your-repeater-host
AutoReverseRepeaterPort=5500
AutoReverseId=ID:123456
AutoReverseReconnectInterval=5
```

The server maintains one waiting/connected reverse client. When that client disconnects, it reconnects after the configured interval. The existing reverse-authentication setting still applies.

## Command-Line Overrides (Application Mode)

```text
TachyonServer.exe -autoreverse -repeater your-repeater-host:5500 -id:123456 -reconnect 5 -run
```

Command-line values override `tachyon.ini` for that run. For service mode (`TachyonDeskService`), configure the `tachyon.ini` used by the service. Hostnames and IPv4 addresses can use `host:port` on the command line; for IPv6, use the separate host/port fields in `tachyon.ini` and enable the server's IPv6 setting.

## Implementation Notes

- Uses a zero-padded 250-byte repeater ID registration record.
- Connection ownership is guarded by an atomic lease.
- The reconnect worker thread is joined cleanly before server shutdown.

## TachyonDesk Repeater

`repeater/TachyonRepeater.sln` builds `TachyonRepeater.exe` (also included in `server/TachyonServer.sln` and `build.ps1`). Its original GPL notices are retained.

Build standalone x64 Release via `build.ps1`, or directly with MSBuild:

```powershell
powershell -ExecutionPolicy Bypass -File .\build.ps1
```
