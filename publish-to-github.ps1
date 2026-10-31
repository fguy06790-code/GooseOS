param([string]$Branch = 'main')
$ErrorActionPreference = 'Stop'
Push-Location $PSScriptRoot
try {
    & git diff --quiet
    if ($LASTEXITCODE -ne 0) { throw 'Commit reviewed changes before publishing.' }
    & git diff --cached --quiet
    if ($LASTEXITCODE -ne 0) { throw 'Commit staged changes before publishing.' }
    & git push origin "HEAD:refs/heads/$Branch"
    if ($LASTEXITCODE -ne 0) { throw 'Git push failed.' }
} finally { Pop-Location }
