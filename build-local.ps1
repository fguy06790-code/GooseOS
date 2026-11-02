param([string]$Distribution = 'Debian', [switch]$InstallPrereqs, [string]$WifiInstaller, [string]$AsusWifiDriverDirectory)
$ErrorActionPreference = 'Stop'
if ($InstallPrereqs) {
    & wsl -d $Distribution -u root -- sh -lc 'apt-get update && apt-get install -y build-essential git cmake ninja-build flex bison curl python3 xz-utils'
    if ($LASTEXITCODE -ne 0) { throw 'Prerequisite installation failed' }
}
$script = (& wsl -d $Distribution -- wslpath -a (Join-Path $PSScriptRoot 'build-linux.sh')).Trim()
if ($LASTEXITCODE -ne 0) { throw 'Could not resolve WSL path' }
$buildEnvArgs = @()
if ($WifiInstaller) {
    $file = Get-Item -LiteralPath $WifiInstaller -ErrorAction Stop
    if ($file.PSIsContainer) { throw 'WifiInstaller must name an installer file.' }
    $wifiPath = (& wsl -d $Distribution -- wslpath -a $file.FullName).Trim()
    if ($LASTEXITCODE -ne 0) { throw 'Could not resolve the Wi-Fi installer path.' }
    $buildEnvArgs += "WIFI_INSTALLER=$wifiPath"
}
if ($AsusWifiDriverDirectory) {
    $folder = Get-Item -LiteralPath $AsusWifiDriverDirectory -ErrorAction Stop
    if (-not $folder.PSIsContainer) { throw 'AsusWifiDriverDirectory must name the exported driver directory.' }
    $usbWifiPath = (& wsl -d $Distribution -- wslpath -a $folder.FullName).Trim()
    if ($LASTEXITCODE -ne 0) { throw 'Could not resolve the ASUS Wi-Fi driver path.' }
    $buildEnvArgs += "ASUS_WIFI_DRIVER_DIR=$usbWifiPath"
}
& wsl -d $Distribution -u root -- env @buildEnvArgs sh $script
if ($LASTEXITCODE -ne 0) { throw "Build failed with exit code $LASTEXITCODE" }
