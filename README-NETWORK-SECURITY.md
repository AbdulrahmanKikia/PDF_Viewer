## Network & Remote-Feature Security Guidelines

The current PDFViewer application is a **purely local desktop app**:

- No HTTP/HTTPS clients or servers.
- No sockets, IPC listeners, or web views.
- No automatic update checks, telemetry, or cloud features.

Any future networked or remote-capable feature must follow these requirements.

### General Principles

- Treat **all remote input as untrusted**.
- Minimize the number of network endpoints and protocols.
- Prefer **pull-based** flows from the UI (user-initiated) rather than background daemons.

### Transport Security

- Use **HTTPS only** for all remote HTTP(S) interactions.
- Enable and enforce **certificate validation**:
  - Use the platform’s trusted CA store by default.
  - Do not ship your own broad CA bundle unless absolutely necessary.
  - Pin only to narrow, well-controlled certificates when justified.
- Do **not** disable certificate or hostname verification, even behind feature flags.

### Authentication & Secrets

- Do not hardcode API keys, tokens, or passwords in the source code or resources.
- If authentication is required:
  - Use short-lived access tokens where possible.
  - Store refresh tokens or long-lived secrets in platform-appropriate secure storage
    (e.g. Windows Credential Manager, macOS Keychain, Linux keyrings) – not in `QSettings`,
    JSON files, or plain text.
- Never log credentials or full tokens; at most log a short, obfuscated prefix for debugging.

### Request Handling & Input Validation

- Validate all remote-derived data before use:
  - Enforce strict schemas for JSON payloads.
  - Treat filenames, paths, and URLs from remote sources as untrusted and normalize them,
    rejecting values that escape allowed directories or expected formats.
- Implement conservative size and rate limits:
  - Maximum payload sizes.
  - Timeouts on network operations.
  - Simple client-side rate limiting for user-triggerable actions (e.g. update checks).

### Update Checks and Telemetry

- Make any update-checking or telemetry feature **opt-in** or clearly disclosed in the UI.
- For update checks:
  - Fetch only minimal metadata (version strings, release channels).
  - Verify downloaded artifacts using signatures or checksums from a trusted channel.
- Telemetry data, if ever added, must avoid PII and file-content leakage; prefer coarse,
  aggregated metrics.

### Error Handling & Logging

- Avoid logging raw server responses that may contain PII or secrets.
- Surface network errors to users in a clear but **non-verbose** way (no stack traces or
  internal URLs).
- When in doubt, fail closed: on ambiguous TLS or parsing errors, abort the operation.

### Examples of Non-Compliant Patterns (Disallowed)

- Disabling SSL verification, e.g. “accept all certificates”.
- Sending full file paths or document content to third-party endpoints without explicit,
  informed user consent.
- Background network tasks that run without any visible indication or user control.

