<#
.SYNOPSIS
    Automated Local Builder for Super-Lightweight LiberNT + Cairo Desktop + Custom Boot
#>

param(
    [string]$SourceDir = "C:\LiberNT-Build",
    [switch]$InstallPrereqs
)

$ErrorActionPreference = "Stop"

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host " Super-Lightweight LiberNT Builder (Cairo + Custom Boot)" -ForegroundColor Cyan
Write-Host "==========================================================" -ForegroundColor Cyan

if ($InstallPrereqs) {
    Write-Host "[1/5] Installing Build Prerequisites..." -ForegroundColor Yellow
    winget install --id Git.Git -e --silent
    winget install --id Kitware.CMake -e --silent
    winget install --id Ninja-build.Ninja -e --silent
    
    # Download RosBE
    Write-Host "Downloading ReactOS Build Environment (RosBE)..."
    Invoke-WebRequest -Uri "https://downloads.sourceforge.net/project/reactos/RosBE-Windows/2.2.1/RosBE-2.2.1.exe" -OutFile "$env:TEMP\RosBE-Setup.exe"
    Start-Process -FilePath "$env:TEMP\RosBE-Setup.exe" -ArgumentList "/S", "/D=C:\RosBE" -Wait
}

Write-Host "[2/5] Preparing LiberNT source..." -ForegroundColor Yellow
if (-not (Test-Path "$SourceDir")) {
    git clone --depth 1 https://github.com/LiberNT/LiberNT.git "$SourceDir"
}

Write-Host "[3/5] Injecting Cairo Desktop & Super-Lightweight Tweaks..." -ForegroundColor Yellow
$cairoDir = "$SourceDir\modules\cairo\files"
New-Item -ItemType Directory -Path $cairoDir -Force | Out-Null

$cairoUrl = (Invoke-RestMethod -Uri "https://api.github.com/repos/cairoshell/cairoshell/releases/latest").assets | Where-Object { $_.name -like "*64*.zip" } | Select-Object -First 1 -ExpandProperty browser_download_url
Invoke-WebRequest -Uri $cairoUrl -OutFile "$env:TEMP\cairo.zip"
Expand-Archive -Path "$env:TEMP\cairo.zip" -DestinationPath $cairoDir -Force

# Strip non-English language packs from Cairo
Get-ChildItem -Path $cairoDir -Directory | Where-Object { $_.Name.Length -eq 2 -or $_.Name -like "*-*" } | Remove-Item -Recurse -Force
Get-ChildItem -Path $cairoDir -Include *.pdb, *.xml -Recurse | Remove-Item -Force

Copy-Item ".\CairoSettings.xml" -Destination "$cairoDir\CairoSettings.xml" -Force
Copy-Item ".\custom-bootlogo.bmp" -Destination "$SourceDir\ntoskrnl\inbv\logo\1.bmp" -Force

# Patch hivesft.inf (Default shell -> Cairo)
$hivesft = Get-ChildItem -Path "$SourceDir" -Filter "hivesft.inf" -Recurse | Select-Object -First 1
(Get-Content $hivesft.FullName) -replace '"explorer.exe"', '"%SystemRoot%\CairoDesktop\CairoDesktop.exe"' | Set-Content $hivesft.FullName

# Patch hivesys.inf (Aggressive service stripping)
$hivesys = Get-ChildItem -Path "$SourceDir" -Filter "hivesys.inf" -Recurse | Select-Object -First 1
(Get-Content $hivesys.FullName) -replace '("Spooler","Start",0x00010001,)0x00000002', '$10x00000004' | Set-Content $hivesys.FullName

Write-Host "[4/5] Ready to compile super-lightweight ISO!" -ForegroundColor Green
Write-Host "Run: cd $SourceDir; C:\RosBE\RosBE.cmd; cmake -G Ninja -DARCH=amd64 -DCMAKE_BUILD_TYPE=MinSizeRel -DOPTIMIZE=1 -B build; ninja -C build bootcd"
