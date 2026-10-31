param([string]$Distribution = 'Debian', [switch]$InstallPrereqs)
$ErrorActionPreference = 'Stop'
if ($InstallPrereqs) {
    & wsl -d $Distribution -u root -- sh -lc 'apt-get update && apt-get install -y build-essential git cmake ninja-build flex bison curl python3 xz-utils'
    if ($LASTEXITCODE -ne 0) { throw 'Prerequisite installation failed' }
}
$script = (& wsl -d $Distribution -- wslpath -a (Join-Path $PSScriptRoot 'build-linux.sh')).Trim()
if ($LASTEXITCODE -ne 0) { throw 'Could not resolve WSL path' }
& wsl -d $Distribution -u root -- sh $script
if ($LASTEXITCODE -ne 0) { throw "Build failed with exit code $LASTEXITCODE" }
