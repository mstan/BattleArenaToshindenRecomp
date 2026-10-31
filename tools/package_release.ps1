# Official Windows releases use the pinned shared builder and title recipe.
# Run with shared options, e.g. --channel public --platform windows --jobs 2.
$ErrorActionPreference = 'Stop'
$titleRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
Push-Location -LiteralPath $titleRoot
try {
    & (Join-Path $titleRoot 'psxrecomp/tools/package_title_release.ps1') @args
    $packageExitCode = $LASTEXITCODE
}
finally { Pop-Location }
exit $packageExitCode
