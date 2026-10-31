<#
.SYNOPSIS
    Automates installing Git & GitHub CLI, creating a new GitHub repository,
    pushing the custom LiberNT configuration, and launching the ISO build workflow.

.EXAMPLE
    .\publish-to-github.ps1 -RepoName "my-libernt-os" -Visibility public
#>

param(
    [string]$RepoName = "libernt-superlight",
    [ValidateSet("public", "private")]
    [string]$Visibility = "public"
)

$ErrorActionPreference = "Stop"

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host "  Create GitHub Repo & Trigger Super-Lightweight ISO Build " -ForegroundColor Cyan
Write-Host "==========================================================" -ForegroundColor Cyan

function Refresh-Path {
    $env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")
    if (Test-Path "C:\Program Files\Git\cmd") { $env:Path += ";C:\Program Files\Git\cmd" }
    if (Test-Path "C:\Program Files\GitHub CLI") { $env:Path += ";C:\Program Files\GitHub CLI" }
}

# 1. Install Git if missing
Refresh-Path
if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
    Write-Host "[1/5] Git not found. Installing via winget..." -ForegroundColor Yellow
    winget install --id Git.Git -e --source winget --accept-package-agreements --accept-source-agreements --silent
    Refresh-Path
} else {
    Write-Host "[1/5] Git is already installed." -ForegroundColor Green
}

# 2. Install GitHub CLI (gh) if missing
if (-not (Get-Command gh -ErrorAction SilentlyContinue)) {
    Write-Host "[2/5] GitHub CLI (gh) not found. Installing via winget..." -ForegroundColor Yellow
    winget install --id GitHub.cli -e --source winget --accept-package-agreements --accept-source-agreements --silent
    Refresh-Path
} else {
    Write-Host "[2/5] GitHub CLI is already installed." -ForegroundColor Green
}

# 3. Authenticate with GitHub
Write-Host "[3/5] Verifying GitHub authentication..." -ForegroundColor Yellow
gh auth status 2>&1 | Out-Null
if ($LASTEXITCODE -ne 0) {
    Write-Host "Launching web browser for GitHub authorization..." -ForegroundColor Cyan
    gh auth login --web -p https
} else {
    Write-Host "GitHub authentication confirmed." -ForegroundColor Green
}

# 4. Prepare Git repository
Write-Host "[4/5] Preparing local repository and committing files..." -ForegroundColor Yellow
Set-Location -Path $PSScriptRoot

if (-not (Test-Path ".git")) {
    git init
    git branch -M main
}

# Set default git identity if unset
$currentUserEmail = git config user.email
if (-not $currentUserEmail) {
    git config user.email "builder@libernt.local"
    git config user.name "LiberNT Builder"
}

git add .
git commit -m "Super-Lightweight LiberNT with Cairo Desktop & Custom Boot" --allow-empty

# 5. Create remote GitHub repository & push
Write-Host "[5/5] Creating GitHub repository '$RepoName' ($Visibility) and pushing..." -ForegroundColor Yellow

$existingRemote = git remote | Where-Object { $_ -eq "origin" }
if ($existingRemote) {
    git remote remove origin
}

# Create repo and push
gh repo create $RepoName "--$Visibility" --source=. --remote=origin --push

# Retrieve repo URL and launch Actions tab
$repoUrl = gh repo view --json url -q .url
$actionsUrl = "$repoUrl/actions"

Write-Host ""
Write-Host "==========================================================" -ForegroundColor Green
Write-Host "  SUCCESS! Repository created and files pushed!" -ForegroundColor Green
Write-Host "  Repository URL: $repoUrl" -ForegroundColor Cyan
Write-Host "  Build Actions:  $actionsUrl" -ForegroundColor Cyan
Write-Host "==========================================================" -ForegroundColor Green
Write-Host "Opening the GitHub Actions build page in your browser..." -ForegroundColor Yellow

Start-Process $actionsUrl
