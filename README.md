# Custom Super-Lightweight LiberNT OS (Cairo Desktop + Custom Boot)

This repository contains everything needed to compile an ultra-lightweight Windows NT 10.0 operating system with **Cairo Desktop** and a **Custom Boot Screen**.

## What is Configured:
1. **Operating System:** Pure Windows NT 10.0 architecture (LiberNT, AMD64 64-bit).
2. **Default Shell:** Cairo Desktop (super-lightweight mode with zero blur/shadows/animations/polling).
3. **Boot Screen:** Custom boot logo (`custom-bootlogo.bmp`) patched into `ntoskrnl.exe`.
4. **Stripped Daemons:** Print spooler, remote diagnostics, and telemetry disabled in `hivesys.inf`.
5. **Compiler Flags:** `MinSizeRel` with `strip --strip-all` on all binaries.

---

## 🚀 1-Click: Create GitHub Repo & Build ISO

To automatically install any missing tools (Git / GitHub CLI), create a new GitHub repository, push this code, and start the ISO build:

1. Open PowerShell.
2. Run:
```powershell
Set-Location "C:\Users\Owner\.gemini\antigravity\scratch\LiberNT-Custom"
.\publish-to-github.ps1
```

* Optional parameters:
```powershell
.\publish-to-github.ps1 -RepoName "my-custom-os" -Visibility private
```

The script will open your browser directly to your new repository's **Actions** tab where you can watch the ISO compile and download the finished **`LiberNT-SuperLight-Cairo-ISO`** as soon as it completes!
