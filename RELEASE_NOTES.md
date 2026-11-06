# Goose language port — unreleased

- The desktop spectrum algorithm and Store retry policy are implemented in `.goose` source and linked through generated C interfaces.
- Builds require checksum-pinned Goose v2026.3 and initialize its runtime on the calling UI thread.
- Added Goose source tests and native ABI tests; retained earlier native bug fixes below.
- This is a partial user-mode port. The kernel, drivers and other native components remain C/C++; no Goose-port ISO has been released or boot-validated.

# Unreleased fixes

- Wait through WLAN service start/stop transitions and show WLAN operation errors in Settings.
- Grow miniport scan-result buffers when the driver reports more data than fits in the initial 16 KiB allocation.
- Prevent Store catalog refresh and package installation from overlapping, clear deselected packages, and ignore empty-background double-clicks.
- Report installer restart requirements without claiming the application has been verified working.
- Bound setup downloads by actual byte progress, cancel failed jobs, reject hash checks after read errors, and clear metadata between component entries.
- Add isolated Windows regressions and a sanitized portable scan-buffer test; require them before CI ISO builds.

These changes have not been released as an ISO. Chrome rendering, physical Wi-Fi connections, and wider hardware/application compatibility remain unresolved. See VALIDATION.md for the checks performed.

# THYFOS 1.1.0

- Open Terminal from the desktop launcher. Run `lntget rice` to download hash-verified Windows builds of Fastfetch, CAVA, and CMatrix.
- The five-peak THYFOS star appears in the boot artwork, Terminal icon, and `neofetch` text art. The setup wizard still displays the older T artwork.
- THYFOS Store can retry a failed catalog load. Settings includes installed-app management, and first-run setup checks network connectivity before downloading Chrome.
- The amd64 ISO includes optional Wi-Fi driver packages. VirtualBox NAT provides network access through the host. Physical Wi-Fi passthrough remains unverified for a successful connection.

Download the amd64 ISO and check it against SHA256SUMS. GitHub attaches source archives for the release tag.
