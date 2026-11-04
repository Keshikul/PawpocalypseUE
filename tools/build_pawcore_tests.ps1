# Builds and runs the PawCore L1 unit tests (docs/09 §2) with plain MSVC: no engine, no UBT, seconds.
# PawCore's sources plus Source/PawCore/Tests/*.cpp go into one exe; Private/PawCoreModule.cpp (the Unreal module glue,
# the only file with an engine include) is left out. Exit code = the test exe's (non-zero on any failure).
#
#   powershell -ExecutionPolicy Bypass -File tools\build_pawcore_tests.ps1            # build + run
#   powershell -ExecutionPolicy Bypass -File tools\build_pawcore_tests.ps1 -NoRun     # build only
#   powershell -ExecutionPolicy Bypass -File tools\build_pawcore_tests.ps1 -Long      # ONLY the long suite (~30 min; PAWCORE_LONG_SCALE)
#   ... -TestArgs '-tc=RNG*'                                                          # doctest options (a filter here)
# Also builds build\pawcore_tests\pawcore_sim.exe (the mirror CLI, Tests/PcSimCli.cpp); -SimOnly builds just that one
# (e.g. while a long test run holds pawcore_tests.exe open), -TestsOnly skips it (tools/sabotage_pawcore.py: a sabotage
# never runs the CLI, and building it doubled every sabotage's time).
# /MP compiles the sources in parallel (2026-10-31: a sabotage's rebuild went from ~4 min to well under one).
param(
	[switch]$NoRun,
	[switch]$Long,
	[switch]$SimOnly,
	[switch]$TestsOnly,
	[Parameter(ValueFromRemainingArguments = $true)][string[]]$TestArgs
)
$ErrorActionPreference = 'Stop'

$Root = Split-Path -Parent $PSScriptRoot
$Core = Join-Path $Root 'Source\PawCore'
$Out = Join-Path $Root 'build\pawcore_tests'
New-Item -ItemType Directory -Force $Out | Out-Null

$VsWhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path $VsWhere)) { throw "vswhere.exe not found: install Visual Studio 2022 with the C++ workload (KB U/01 §2)." }
$VsPath = & $VsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $VsPath) { throw 'No Visual Studio with the x64 C++ tools found.' }
$VcVars = Join-Path $VsPath 'VC\Auxiliary\Build\vcvars64.bat'

$Sources = @(Get-ChildItem (Join-Path $Core 'Private') -Filter *.cpp -Recurse | Where-Object { $_.Name -ne 'PawCoreModule.cpp' })
$Sources += @(Get-ChildItem (Join-Path $Core 'Tests') -Filter *.cpp)
$SourceArgs = ($Sources | ForEach-Object { '"' + $_.FullName + '"' }) -join ' '

# C++17 to keep PawCore portable (docs/03 §1); /W4 /WX so a warning the engine build would reject fails here first.
$Exe = Join-Path $Out 'pawcore_tests.exe'
$Cl = "cl /nologo /MP /std:c++17 /EHsc /W4 /WX /permissive- /utf-8 /O2 /DPAWCORE_STANDALONE_TESTS=1 /D_CRT_SECURE_NO_WARNINGS " +
	"/I`"$Core\Public`" /I`"$Core\Private`" /Fo`"$Out\\`" /Fe`"$Exe`" $SourceArgs"

$Log = Join-Path $Out 'build.log'
if (-not $SimOnly) {
# Redirect inside cmd: vcvars64 writes a harmless "vswhere.exe is not recognized" to stderr, and Windows PowerShell
# turns any native stderr line into an error record that $ErrorActionPreference='Stop' would make fatal.
cmd /c "`"$VcVars`" >nul 2>&1 && $Cl > `"$Log`" 2>&1"
if ($LASTEXITCODE -ne 0) {
	Get-Content $Log
	Write-Host "PawCore test build FAILED (log: $Log)"
	exit 1
}
Write-Host "Built $Exe from $($Sources.Count) source files"
}

if (-not $TestsOnly) {

# pawcore_sim.exe, PawCore's half of the mirror gate (tools/check_mirror.py): the rule sources plus the CLI, no tests.
$SimSources = @(Get-ChildItem (Join-Path $Core 'Private') -Filter *.cpp -Recurse | Where-Object { $_.Name -ne 'PawCoreModule.cpp' })
$SimSources += Get-Item (Join-Path $Core 'Tests\PcSimCli.cpp')
$SimArgs = ($SimSources | ForEach-Object { '"' + $_.FullName + '"' }) -join ' '
$SimOut = Join-Path $Out 'sim'
New-Item -ItemType Directory -Force $SimOut | Out-Null
$SimExe = Join-Path $Out 'pawcore_sim.exe'
$SimCl = "cl /nologo /MP /std:c++17 /EHsc /W4 /WX /permissive- /utf-8 /O2 /DPAWCORE_SIM_CLI=1 /D_CRT_SECURE_NO_WARNINGS " +
	"/I`"$Core\Public`" /I`"$Core\Private`" /Fo`"$SimOut\\`" /Fe`"$SimExe`" $SimArgs"
cmd /c "`"$VcVars`" >nul 2>&1 && $SimCl > `"$Log`" 2>&1"
if ($LASTEXITCODE -ne 0) {
	Get-Content $Log
	Write-Host "pawcore_sim build FAILED (log: $Log)"
	exit 1
}
Write-Host "Built $SimExe"
if ($SimOnly) { exit 0 }
}

if ($NoRun) { exit 0 }
# The data tests read the repo's CSVs (override to point them at a copy, e.g. a deliberately broken one).
if (-not $env:PAWCORE_DATA_DIR) { $env:PAWCORE_DATA_DIR = Join-Path $Root 'Data' }
# The long suite (Tests/PcLongTests.cpp and the full Godot fixtures) is skipped by default; -Long runs it alone.
if ($Long) { $TestArgs = @('--no-skip', '--test-suite=long') + @($TestArgs | Where-Object { $_ }) }
& $Exe @TestArgs
exit $LASTEXITCODE
