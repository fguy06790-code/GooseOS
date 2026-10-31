# Validation

Validated locally on 2026-10-31:

- amd64 MinSizeRel boot ISO and 8 GiB preinstalled disk image compiled successfully with LiberNT RosBE GCC 16.2, including WoW64 components.
- Headless QEMU/OVMF with KVM, two virtual CPUs and 2 GiB RAM reached the actual THYFOS desktop, displayed the supplied boot artwork and wallpaper, and opened first-use region/keyboard setup automatically.
- CPU, RAM and GPU widgets updated in the guest. The VM had no audio endpoint, and the audio widget reported unavailable.
- File Explorer launched through the native launcher and displayed its Documents folder, navigation pane and file controls. File create/copy/delete operations were not exercised.
- Native Windows rendering checked login layout at five resolutions from 800×600 to 2560×1440 and rendered the desktop/launcher.
- Synthetic FFT checks passed silence, a 1 kHz opposite-phase stereo signal, zero sample-rate handling, and bounded output. Live CPU/RAM counter sampling passed on Windows; no host audio capture was started.
- Build shell syntax, Python compilation and ISO SHA-256 were checked.

The final caption text placement correction was compiled after the guest screenshots. The screenshots show the earlier overlapping title and traffic lights.

Not established: completed account/password setup, authentication after setup, Chrome installation/execution, driver download/install success, real audio loopback, BIOS desktop boot, or behavior on physical hardware. BIOS testing did not reach a desktop within the bounded test. No claim of universal hardware compatibility is made.

This ISO targets amd64/x86-64 PCs. ARM, phones, consoles, Apple Silicon and other CPU architectures cannot run it natively. Firmware, storage, networking, graphics and audio each require supported hardware and compatible drivers. A virtual machine with emulated supported hardware is the most practical way to try it on a different host architecture; that does not guarantee compatibility or performance.

File Explorer caption update: removed the application-name suffix and the initial application-name caption. Folder titles remain. The changed explorerframe component and ISO were rebuilt; this caption update has not had a separate VM check.

THYFOS logo and automatic Wi-Fi update changes: compiled into the ISO. Actual Wi-Fi package download/install and the revised setup artwork have not been verified in a guest. The already-running VirtualBox VM remains on its previous installed version.

Window opacity: rebuilt the amd64 ISO. A Windows test using an isolated test window verified alpha 179/255 at 70%, repeated application, restoration to a solid window at 100%, and preservation of existing application-owned alpha. Guest compositor rendering of this change has not been separately verified. The running VirtualBox VM is not modified by rebuilding the ISO.

Modern setup and motion validation: the amd64 ISO compiled successfully. Headless QEMU/OVMF/KVM displayed the redesigned setup and moving UEFI loader; setup advanced from language selection to the account page. The window-opacity regression test also passed after the launcher changes. Final follow-up refinements smooth the loader highlight, shorten account copy, and reduce the body font by one pixel; these were compiled after those captures. Animation timing and reduced-motion switching have not been exhaustively tested, nor have third-party app animations. BIOS retains its existing palette fade. The latest local deliverable is in outputs/THYFOS-modern/THYFOS-amd64.iso; the existing running VirtualBox VM remains on its earlier booted build.

Liquid glass / 15% transparency: the new amd64 ISO compiled and its SHA-256 was verified. The isolated Windows test passed 217/255 alpha (85% opacity), solid restoration, preservation of app-owned alpha and exemption of content-backdrop windows from double blending. VirtualBox on the ASUS host booted outputs/THYFOS-glass/THYFOS-amd64.iso and displayed the modern setup with its content-backdrop material and glass outline. No installation or partition change was performed. Desktop and login card fill opacity is also 217/255. Existing app-managed transparency remains an exception.

Settings/rounded UI validation: the amd64 ISO compiled. The THYFOS Settings Home page rendered in the headless QEMU guest when launched through its actual installed path; screenshots are in outputs/THYFOS-settings. The shell's corrected launch path, Settings row and typed aliases passed an isolated Windows regression test without launching host apps. Native tests passed the 85% opacity policy, round-corner ownership/disable behavior, app overrides, custom regions and launcher layout at five resolutions. The launcher focus correction, setup input clipping and menu-radius change were compiled after the captured Home preview; they have not all had separate guest interaction checks. Appearance-page control changes and hardware-dependent Settings categories are not fully runtime-verified. Chrome was observed with a blank window in the user's earlier VM session and remains unresolved. The user's running state was preserved in a Before Settings update snapshot before the VM restart; after the VM window was closed, it was left powered off with the latest Settings ISO attached.

Intel offline Wi-Fi package: Windows Authenticode reported a valid Intel Corporation signature for the supplied 75,760,592-byte file. Its SHA-512 matched Intel's published package hash. The amd64 ISO compiled; extracting its Windows/DriverSetup installer to a streaming hash reproduced the exact supplied SHA-256 (40d2f714830b406ccc1f9c00547ad6090f164f928748e38f27a1c7c0ef5ed5ea). Native policy tests accepted Intel PCI wireless IDs and excluded virtual Ethernet, Bluetooth, another vendor and empty IDs. No installer was executed on the Windows host. Real driver installation, restart handling and hardware binding on THYFOS remain unverified. Latest local ISO: outputs/THYFOS-wifi/THYFOS-amd64.iso.

Release 1.0.0: rebuilt the current feature set with product-version labels and an installed release.json manifest. The amd64 build passed. This versioning step does not establish fixes for the compatibility issues listed above; RELEASE_NOTES.md carries those limits into the public release.
