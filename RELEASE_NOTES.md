# THYFOS 1.0.0

Initial experimental amd64 release, based on LiberNT commit `346eba3d855d70214105670f859e9dc0bb1c8615`.

## Included

- THYFOS branding, liquid-metal wallpaper and animated UEFI boot loader.
- Glass sign-in and setup, mac-style standard window controls, rounded compatible windows and menus, and 15% default transparency.
- Native desktop, unpinned dock, launcher with keyboard navigation, and File Explorer.
- THYFOS Settings with a Home page, appearance/widgets, wallpaper, network, sound, display, devices and accounts. Open it from the launcher or with Win+I inside the guest.
- CPU/RAM/GPU resource and system-playback audio-spectrum widgets. Unsupported counters or audio endpoints report unavailable.
- First-use account/password setup, driver setup and an official Chrome download/install step.
- Offline Intel Wi-Fi 24.80.0 installer, hash-pinned and automatically attempted once during first-boot setup when Intel PCI wireless hardware is present. VirtualBox's wired adapter is excluded. The full Intel installer remains under `Windows\DriverSetup`.

## Validation and known issues

The amd64 ISO compiles. QEMU/OVMF/KVM and VirtualBox tests reached the desktop and setup; the Settings Home page rendered in QEMU. Targeted native tests checked launch paths, window opacity, corner policy, launcher layouts, wireless-device filtering and the spectrum algorithm. Package hashes were verified.

This is an initial release, not a claim of production readiness or universal hardware support:

- Chrome has been observed opening a blank window; browser compatibility remains unresolved.
- Completed account creation/sign-in and file create/copy/delete operations have not been fully exercised end to end.
- Intel installer execution, Wi-Fi driver binding and physical-hardware compatibility remain unverified. Intel's package targets Windows 10/11; including it does not establish LiberNT compatibility.
- VirtualBox audio is disabled in the supplied local VM. Real audio loopback and hardware-dependent Settings controls are not fully verified.
- BIOS desktop boot is unverified. The moving loader is for UEFI; BIOS retains its existing palette fade.
- App-managed transparency, custom regions and rendering can override desktop styling. Maximized windows keep native screen-edge behavior.

See `VALIDATION.md` for the detailed test history. The release tag identifies the wrapper/overlay source; the base OS revision is pinned above.

## Files

- `THYFOS-1.0.0-amd64.iso` — bootable installation ISO for x86-64 virtual machines and compatible PCs.
- `THYFOS-1.0.0-source.zip` — build scripts, source overlays, branding assets, license and validation notes. The scripts fetch the pinned base source and toolchain; the Intel EXE is fetched separately and verified against pinned hashes.
- `SHA256SUMS` — SHA-256 checksums for the ISO and source package.
- `VALIDATION.md` — detailed checks and remaining limits.

The ISO opens setup; booting it does not automatically upgrade an existing installation. In a VM, use its virtual Ethernet adapter for internet access. Physical Windows drivers on the host do not become guest drivers.

THYFOS/LiberNT source licensing is documented in `COPYING`; the unmodified Intel package retains its own license and notices.
