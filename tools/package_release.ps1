param(
    # Empty means "read VERSION" from the repository root. That is the shared
    # release version source for this title; packaging/release/VERSION is copied
    # into the staged payload only when root creates it explicitly.
    [string]$Version = "",
    [string]$BuildDir = "build-publish",
    [string]$ToolchainBin = (Join-Path $env:USERPROFILE ".local\share\retcomm\toolchains\cmake-clang-v1\latest\bin"),
    [switch]$SkipBuild,
    [int]$Jobs = 0,
    [switch]$NormalPriority
)

$ErrorActionPreference = "Stop"

$Root = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot "..")).Path
$FrameworkRoot = Join-Path $Root "psxrecomp"
$PackagingRelease = Join-Path $Root "packaging\release"
$BuildPath = Join-Path $Root $BuildDir
$StageRoot = Join-Path $Root "release-stage"
$StageName = "BattleArenaToshindenRecomp-windows-x64"
$Stage = Join-Path $StageRoot $StageName
$ExeStem = "BattleArenaToshinden_Recompiled"
$ZipPath = Join-Path $Root ("BattleArenaToshindenRecomp-{0}-windows-x64.zip" -f $Version)
$RuntimeTarget = "psx-runtime"

function Resolve-NativeTool {
    param(
        [Parameter(Mandatory)][string]$Name,
        [string]$PreferredPath = ""
    )
    if ($PreferredPath -and (Test-Path -LiteralPath $PreferredPath -PathType Leaf)) {
        return (Resolve-Path -LiteralPath $PreferredPath).Path
    }
    $cmd = @(Get-Command $Name -CommandType Application -ErrorAction SilentlyContinue) | Select-Object -First 1
    if (-not $cmd) { throw "Required tool not found on PATH: $Name" }
    return $cmd.Source
}

function Convert-CMakePath {
    param([Parameter(Mandatory)][string]$Path)
    return $Path.Replace('\', '/')
}

$CMakeExe = Resolve-NativeTool "cmake.exe" (Join-Path $ToolchainBin "cmake.exe")
$NinjaExe = Resolve-NativeTool "ninja.exe" (Join-Path $ToolchainBin "ninja.exe")
$ObjdumpExe = Resolve-NativeTool "objdump.exe" (Join-Path $ToolchainBin "objdump.exe")
$CcExe = Resolve-NativeTool "clang.exe" (Join-Path $ToolchainBin "clang.exe")
$CxxExe = Resolve-NativeTool "clang++.exe" (Join-Path $ToolchainBin "clang++.exe")
$RcExe = Resolve-NativeTool "windres.exe" (Join-Path $ToolchainBin "windres.exe")

$env:PATH = "$ToolchainBin;$env:PATH"

if (-not $Version) {
    $VersionFile = Join-Path $Root "VERSION"
    if (-not (Test-Path -LiteralPath $VersionFile -PathType Leaf)) {
        throw "No -Version given and $VersionFile is missing"
    }
    $Version = (Get-Content -LiteralPath $VersionFile -Raw).Trim()
    if (-not $Version) { throw "$VersionFile is empty" }
    $ZipPath = Join-Path $Root ("BattleArenaToshindenRecomp-{0}-windows-x64.zip" -f $Version)
}

if ($Jobs -le 0) {
    $Jobs = [Math]::Max(2, [int]$env:NUMBER_OF_PROCESSORS - 2)
}
if (-not $NormalPriority) {
    [System.Diagnostics.Process]::GetCurrentProcess().PriorityClass =
        [System.Diagnostics.ProcessPriorityClass]::BelowNormal
}

Write-Host ("Packaging BattleArenaToshindenRecomp {0} for Windows x64 (jobs={1}, priority={2})" -f `
    $Version, $Jobs, [System.Diagnostics.Process]::GetCurrentProcess().PriorityClass)

function New-Dir {
    param([Parameter(Mandatory)][string]$Path)
    if (Test-Path -LiteralPath $Path -PathType Leaf) {
        throw "Expected a directory but a file exists at: $Path"
    }
    if (-not (Test-Path -LiteralPath $Path)) {
        [System.IO.Directory]::CreateDirectory($Path) | Out-Null
    }
    return $Path
}

function Copy-FileTo {
    param(
        [Parameter(Mandatory)][string]$Source,
        [Parameter(Mandatory)][string]$Destination
    )
    if (-not (Test-Path -LiteralPath $Source -PathType Leaf)) {
        throw "Required release file not found: $Source"
    }
    if (Test-Path -LiteralPath $Destination -PathType Container) {
        throw "Copy-FileTo destination is a directory: $Destination"
    }
    New-Dir (Split-Path -Parent $Destination) | Out-Null
    Copy-Item -LiteralPath $Source -Destination $Destination -Force
}

function Copy-FileInto {
    param(
        [Parameter(Mandatory)][string]$Source,
        [Parameter(Mandatory)][string]$DestinationDir
    )
    New-Dir $DestinationDir | Out-Null
    Copy-FileTo $Source (Join-Path $DestinationDir (Split-Path -Leaf $Source))
}

function Copy-TreeTo {
    param(
        [Parameter(Mandatory)][string]$Source,
        [Parameter(Mandatory)][string]$Destination
    )
    if (-not (Test-Path -LiteralPath $Source -PathType Container)) {
        throw "Required release directory not found: $Source"
    }
    if (Test-Path -LiteralPath $Destination) {
        Remove-Item -LiteralPath $Destination -Recurse -Force
    }
    New-Dir (Split-Path -Parent $Destination) | Out-Null
    Copy-Item -LiteralPath $Source -Destination $Destination -Recurse -Force
}

function Invoke-Native {
    param([scriptblock]$Cmd, [string]$What)
    $old = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    & $Cmd 2>&1 | Out-Host
    $code = $LASTEXITCODE
    $ErrorActionPreference = $old
    if ($code -ne 0) { throw "$What failed (exit $code)" }
}

function Remove-TreeInsideRoot {
    param([Parameter(Mandatory)][string]$Path)
    if (-not (Test-Path -LiteralPath $Path)) { return }
    $resolvedRoot = (Resolve-Path -LiteralPath $Root).Path.TrimEnd('\')
    $resolvedPath = (Resolve-Path -LiteralPath $Path).Path.TrimEnd('\')
    if (-not $resolvedPath.StartsWith($resolvedRoot + "\", [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to delete path outside repo root: $resolvedPath"
    }
    Remove-Item -LiteralPath $resolvedPath -Recurse -Force
}

function Assert-PlayerGameToml {
    param([Parameter(Mandatory)][string]$Path)
    $text = Get-Content -LiteralPath $Path -Raw
    $forbidden = @(
        '^\s*disc\s*=\s*"[A-Za-z]:',
        '^\s*bios\s*=\s*"[A-Za-z]:',
        '^\s*retail_bios\s*=',
        'SCPH1001\.BIN'
    )
    foreach ($pat in $forbidden) {
        if ($text -match $pat) {
            throw "packaging/release/game.toml contains release-forbidden content matching: $pat"
        }
    }
}

function Assert-BuildMatchesRelease {
    param([Parameter(Mandatory)][string]$Path)
    $cache = Join-Path $Path "CMakeCache.txt"
    if (-not (Test-Path -LiteralPath $cache -PathType Leaf)) {
        throw "Build directory is not configured: $Path"
    }
    $cacheValues = @{}
    foreach ($line in (Get-Content -LiteralPath $cache)) {
        if ($line -match '^([^#/][^:=]*):[^=]*=(.*)$') {
            $cacheValues[$Matches[1]] = $Matches[2]
        }
    }
    if ($cacheValues.ContainsKey("CMAKE_BUILD_TYPE") -and
        $cacheValues["CMAKE_BUILD_TYPE"] -ne "Release") {
        throw "Build cache CMAKE_BUILD_TYPE is '$($cacheValues["CMAKE_BUILD_TYPE"])', expected 'Release'"
    }
    $builtVersion = Join-Path $Path "psx_game_version.txt"
    if (-not (Test-Path -LiteralPath $builtVersion -PathType Leaf)) {
        throw "Build did not publish psx_game_version.txt"
    }
    $haveVersion = (Get-Content -LiteralPath $builtVersion -Raw).Trim()
    if ($haveVersion -ne $Version) {
        throw "Build game version is $haveVersion, expected $Version"
    }
    $ninja = Join-Path $Path "build.ninja"
    if (-not (Test-Path -LiteralPath $ninja -PathType Leaf)) {
        throw "Build directory is missing build.ninja"
    }
    $ninjaText = Get-Content -LiteralPath $ninja -Raw
    if ($ninjaText -notmatch 'PSX_NO_DEBUG_TOOLS=1') {
        throw "Build graph does not define PSX_NO_DEBUG_TOOLS=1"
    }
    if ($ninjaText -notmatch ('PSX_GAME_VERSION=\\?"' + [regex]::Escape($Version) + '\\?"')) {
        throw "Build graph does not define PSX_GAME_VERSION=$Version"
    }
}

$PlayerGameToml = Join-Path $PackagingRelease "game.toml"
$InputIni = Join-Path $PackagingRelease "input.ini"
$StartHere = Join-Path $PackagingRelease "START_HERE.txt"
$PayloadVersionFile = Join-Path $PackagingRelease "VERSION"
if (-not (Test-Path -LiteralPath $PayloadVersionFile -PathType Leaf)) {
    $PayloadVersionFile = Join-Path $Root "VERSION"
}
Assert-PlayerGameToml $PlayerGameToml

if ($SkipBuild) {
    Write-Host "SkipBuild: packaging existing build output from $BuildPath"
} else {
    Invoke-Native {
        & $CMakeExe -S (Convert-CMakePath $Root) -B (Convert-CMakePath $BuildPath) -G Ninja `
            "-DCMAKE_MAKE_PROGRAM=$(Convert-CMakePath $NinjaExe)" `
            "-DCMAKE_C_COMPILER=$(Convert-CMakePath $CcExe)" `
            "-DCMAKE_CXX_COMPILER=$(Convert-CMakePath $CxxExe)" `
            "-DCMAKE_RC_COMPILER=$(Convert-CMakePath $RcExe)" `
            -DCMAKE_BUILD_TYPE=Release `
            -DPSX_STATIC_RUNTIME=ON `
            -DPSX_DEBUG_TOOLS=OFF `
            "-DPSX_GAME_VERSION=$Version" `
            "-DCMAKE_EXE_LINKER_FLAGS=-Wl,--no-insert-timestamp"
    } "cmake configure"
    Invoke-Native {
        & $CMakeExe --build $BuildPath --target $RuntimeTarget -j $Jobs
    } "cmake build"
}
Assert-BuildMatchesRelease $BuildPath

Remove-TreeInsideRoot $StageRoot
New-Dir $Stage | Out-Null
New-Dir (Join-Path $Stage "saves") | Out-Null
New-Dir (Join-Path $Stage "cache") | Out-Null

$DevExe = Join-Path $BuildPath "$ExeStem.exe"
if (-not (Test-Path -LiteralPath $DevExe -PathType Leaf)) {
    $DevExe = Join-Path $BuildPath "$RuntimeTarget.exe"
}
Copy-FileTo $DevExe (Join-Path $Stage "$ExeStem.exe")
Copy-FileTo $PlayerGameToml (Join-Path $Stage "game.toml")
Copy-FileTo $InputIni (Join-Path $Stage "input.ini")
Copy-FileTo $StartHere (Join-Path $Stage "START_HERE.txt")
Copy-FileTo $PayloadVersionFile (Join-Path $Stage "VERSION")
Copy-FileInto (Join-Path $Root "README.md") $Stage
if (Test-Path -LiteralPath (Join-Path $Root "LICENSE") -PathType Leaf) {
    Copy-FileInto (Join-Path $Root "LICENSE") $Stage
}
if (Test-Path -LiteralPath (Join-Path $Root "RELEASE_NOTES.md") -PathType Leaf) {
    Copy-FileInto (Join-Path $Root "RELEASE_NOTES.md") $Stage
}
New-Dir (Join-Path $Stage "licenses") | Out-Null
Copy-FileTo (Join-Path $FrameworkRoot "LICENSE") `
    (Join-Path $Stage "licenses\psxrecomp-LICENSE")
Copy-FileInto (Join-Path $FrameworkRoot "THIRD_PARTY_ATTRIBUTION.md") $Stage
Copy-FileTo (Join-Path $FrameworkRoot "runtime\licenses\libchdr-NOTICES.txt") `
    (Join-Path $Stage "licenses\libchdr-NOTICES.txt")
if (Test-Path -LiteralPath (Join-Path $Root "recomp-ui\LICENSE") -PathType Leaf) {
    Copy-FileTo (Join-Path $Root "recomp-ui\LICENSE") `
        (Join-Path $Stage "licenses\recomp-ui-LICENSE")
}

$BundledBiosSrc = Join-Path $BuildPath "bios"
$BundledBiosDst = New-Dir (Join-Path $Stage "bios")
Copy-FileInto (Join-Path $BundledBiosSrc "openbios.bin") $BundledBiosDst
Copy-FileInto (Join-Path $BundledBiosSrc "OpenBIOS.LICENSE") $BundledBiosDst
if ((Get-Item -LiteralPath (Join-Path $BundledBiosDst "openbios.bin")).Length -ne 524288) {
    throw "Staged OpenBIOS is not 512 KiB"
}

$AssetsSrc = Join-Path $BuildPath "assets"
if (Test-Path -LiteralPath $AssetsSrc -PathType Container) {
    Copy-TreeTo $AssetsSrc (Join-Path $Stage "assets")
}

$PythonExe = Resolve-NativeTool "py.exe" (Join-Path $env:WINDIR "py.exe")
$ReleaseStagePy = Join-Path $FrameworkRoot "tools\release_stage.py"
Invoke-Native {
    & $PythonExe -3 $ReleaseStagePy stage-mods `
        --build-path $BuildPath `
        --stage $Stage `
        --runtime-target $RuntimeTarget `
        --mod-source (Join-Path $Root "mods\preloaded")
} "mod catalog staging"

$imports = & $ObjdumpExe -p (Join-Path $Stage "$ExeStem.exe") |
    Select-String "DLL Name: (.+)" | ForEach-Object { $_.Matches[0].Groups[1].Value.Trim() }
$systemDlls = @(
    "kernel32.dll","user32.dll","gdi32.dll","shell32.dll","msvcrt.dll",
    "advapi32.dll","ws2_32.dll","comdlg32.dll","dbghelp.dll","ole32.dll",
    "oleaut32.dll","winmm.dll","imm32.dll","version.dll","setupapi.dll",
    "dinput8.dll","rpcrt4.dll","hid.dll","cfgmgr32.dll","opengl32.dll",
    "bcrypt.dll","crypt32.dll","secur32.dll","iphlpapi.dll","dnsapi.dll",
    "normaliz.dll","wldap32.dll","winhttp.dll","shlwapi.dll","comctl32.dll",
    "winmm.dll","uxtheme.dll","d2d1.dll","dwrite.dll"
)
$nonSystem = $imports | Where-Object {
    $dll = $_.ToLower()
    ($systemDlls -notcontains $dll) -and ($dll -notmatch '^api-ms-win-crt-[a-z0-9-]+\.dll$')
}
if ($nonSystem) {
    throw "Release exe is not self-contained; imports non-system DLL(s): $($nonSystem -join ', ')"
}
Write-Host "Verified self-contained: imports only Windows system DLLs ($($imports.Count) total)"

$exeBytes = [System.IO.File]::ReadAllBytes((Join-Path $Stage "$ExeStem.exe"))
$exeText = [System.Text.Encoding]::ASCII.GetString($exeBytes)
$bakedBios = [regex]::Matches($exeText, '[A-Za-z]:[/\\][ -~]*?SCPH1001\.BIN') |
    ForEach-Object { $_.Value } | Select-Object -Unique
if ($bakedBios) {
    throw "Release exe contains baked absolute BIOS path(s): $($bakedBios -join '; ')"
}
Write-Host "Verified no baked absolute retail BIOS path in the exe"

$forbiddenStageFiles = @()
$forbiddenStageFiles += Get-ChildItem -LiteralPath $Stage -Recurse -File -ErrorAction SilentlyContinue |
    Where-Object {
        $rel = $_.FullName.Substring($Stage.Length).TrimStart('\','/')
        $isBundledOpenBios = $rel -ieq "bios\openbios.bin" -or $rel -ieq "bios/openbios.bin"
        $_.Name -like "SCPH*.BIN" -or
        (($_.Extension -in @(".bin",".cue",".iso",".mcd",".mcr",".img",".sub",".ccd",".chd")) -and
            -not $isBundledOpenBios) -or
        $_.Name -in @("bios.cfg","disc.cfg","settings.toml","keybinds.ini",
                      "overlay_captures.json","state.toml","state.toml.tmp") -or
        $rel -match '(^|[\\/])generated([\\/]|$)'
    }
if ($forbiddenStageFiles) {
    throw "Stage contains files that must never ship: $(($forbiddenStageFiles | ForEach-Object FullName) -join '; ')"
}
$savesFiles = Get-ChildItem -LiteralPath (Join-Path $Stage "saves") -Recurse -File -ErrorAction SilentlyContinue
if ($savesFiles) {
    throw "Stage saves/ directory must be empty, contains: $(($savesFiles | ForEach-Object FullName) -join '; ')"
}
Write-Host "Verified no retail BIOS/disc/generated-source/save/preference files in stage"

if (Test-Path -LiteralPath $ZipPath) { Remove-Item -LiteralPath $ZipPath -Force }

if ($env:SOURCE_DATE_EPOCH) {
    $epoch = [int64]$env:SOURCE_DATE_EPOCH
} else {
    $epoch = [int64](& git -C $Root log -1 --format=%ct).Trim()
}
$stamp = [System.DateTimeOffset]::FromUnixTimeSeconds($epoch).UtcDateTime
Write-Host ("Deterministic zip: SOURCE_DATE_EPOCH={0} ({1:yyyy-MM-dd HH:mm:ss}Z)" -f $epoch, $stamp)

Add-Type -AssemblyName System.IO.Compression | Out-Null
Add-Type -AssemblyName System.IO.Compression.FileSystem | Out-Null

$stageParent = Split-Path -Parent $Stage
$entries = Get-ChildItem -LiteralPath $Stage -Recurse -File |
    ForEach-Object {
        [PSCustomObject]@{
            Full = $_.FullName
            Name = $_.FullName.Substring($stageParent.Length).TrimStart('\','/').Replace('\','/')
        }
    } | Sort-Object -Property Name -CaseSensitive

$zipStream = [System.IO.File]::Open($ZipPath, [System.IO.FileMode]::CreateNew)
try {
    $archive = New-Object System.IO.Compression.ZipArchive(
        $zipStream, [System.IO.Compression.ZipArchiveMode]::Create, $true)
    try {
        foreach ($e in $entries) {
            $entry = $archive.CreateEntry($e.Name, [System.IO.Compression.CompressionLevel]::Optimal)
            $entry.LastWriteTime = [System.DateTimeOffset]::new($stamp, [TimeSpan]::Zero)
            $in = [System.IO.File]::OpenRead($e.Full)
            try {
                $out = $entry.Open()
                try { $in.CopyTo($out) } finally { $out.Dispose() }
            } finally { $in.Dispose() }
        }
    } finally { $archive.Dispose() }
} finally { $zipStream.Dispose() }

$zipMB = "{0:N1}" -f ((Get-Item -LiteralPath $ZipPath).Length / 1MB)
$zipSha = (Get-FileHash -LiteralPath $ZipPath -Algorithm SHA256).Hash.ToLower()
Set-Content -LiteralPath "$ZipPath.sha256" -Encoding ASCII -Value "$zipSha  $(Split-Path -Leaf $ZipPath)"
Write-Host "Release packaged: $ZipPath (~$zipMB MB, $($entries.Count) entries)"
Write-Host "SHA256: $zipSha"
