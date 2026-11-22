# Security Policy & Deployment Guidelines

## Transport Encryption (Public Internet)

TachyonDesk transmits all extended channels (video, audio, single-window frames, file clipboard, and file manager) inside the single RFB TCP connection.

- **Always enable transport encryption** (TLS, RSA-AES, or a secure DSM plugin) or run TachyonDesk over a VPN (WireGuard, Tailscale, etc.) when connecting across untrusted networks or the public internet.
- Do not expose an unencrypted `TachyonServer.exe` port directly to the internet.

## File Clipboard Security

Like Windows Remote Desktop (RDP), when file clipboard sharing is enabled:
- Copying files in Explorer on either side sends the file manifest (`Offer`) to the connected peer, which can then read those offered files on demand.
- Only connect to peers you trust, or disable clipboard/file transfer in settings when not needed.
- On the server side, file clipboard is only active when File Transfer is enabled and the client has input permission.

## Service-Mode File Manager & Unattended Access

When `TachyonServer.exe` runs as a Windows service (`TachyonDeskService`, LocalSystem):
- Normally, all file operations impersonate the active logged-on desktop user's token.
- If **no user is logged on** (Windows lock/sign-in screen):
  - With **User impersonation (for Service only)** enabled (default): file access is denied.
  - With **User impersonation** disabled (opt-in): the service accesses files as `LocalSystem`. Anyone who knows the VNC password can read/write files on the machine without logging into Windows. Only disable impersonation if you explicitly accept this risk and enforce strong authentication + encryption.

## Reporting a Vulnerability

If you discover a security vulnerability in TachyonDesk, please report it privately to the repository maintainers via GitHub Security Advisories rather than opening a public issue.
