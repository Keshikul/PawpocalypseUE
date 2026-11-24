# THE ONE COMMAND that runs every gate (CLAUDE.md). Each gate's output goes to build\check_all\<gate>.log; the summary
# table and the exit code (0 = all green) are the verdict.
#
#   powershell -ExecutionPolicy Bypass -File tools\check_all.ps1          # the everyday set, a few minutes
#   powershell -ExecutionPolicy Bypass -File tools\check_all.ps1 -Full    # the nightly: + every self-test, the long
#                                                                        # suites, the realm sabotages
#                                                                        # (~10 h, 2026-11-06; the Godot gates retired 2026-11-23)
#
# Exit 0 = every gate RAN and passed; 1 = a gate failed; 2 = nothing failed but a gate was SKIPPED (an Unreal editor was
# open): not proven, so not a pass (found by review 2026-11-04: a skipped L3 used to exit 0).
#
# Every gate here has been seen to fail (docs/09): the -Full set runs each gate's own negative test (self-test /
# sabotage) beside it, so a gate that silently stopped testing anything fails the night it happens.
#
# The engine gates run on the editor target built FIRST, by the 'UBT editor build' gate: the headless editor loads the DLLs
# on disk, so an engine test without that build tests the code of the last build. A failed build fails every engine gate
# after it instead of running them on stale binaries.
#
# Nothing here touches the designer's game: nothing reads the Godot project (D22, 2026-11-23: the game is its own source;
# Godot was a first try), nothing is committed, packaged or exported.
param(
	[switch]$Full
)
$ErrorActionPreference = 'Stop'

$Root = Split-Path -Parent $PSScriptRoot
$Logs = Join-Path $Root 'build\check_all'
New-Item -ItemType Directory -Force $Logs | Out-Null
$Results = New-Object System.Collections.ArrayList

function Invoke-Gate([string]$Name, [string]$Command) {
	$Log = Join-Path $Logs "$Name.log"
	$Watch = [Diagnostics.Stopwatch]::StartNew()
	Write-Host ("{0,-28} ..." -f $Name) -NoNewline
	# cmd /c: native stderr must not become a PowerShell error record (Windows PowerShell 5.1).
	cmd /c "cd /d `"$Root`" && $Command > `"$Log`" 2>&1"
	$Code = $LASTEXITCODE
	$Watch.Stop()
	$Verdict = if ($Code -eq 0) { 'PASS' } else { 'FAIL' }
	Write-Host (" {0}  ({1:N0} s)" -f $Verdict, $Watch.Elapsed.TotalSeconds)
	[void]$Results.Add([pscustomobject]@{ Gate = $Name; Verdict = $Verdict; Seconds = [int]$Watch.Elapsed.TotalSeconds; Log = $Log })
}


# --- the everyday set ---------------------------------------------------------------------------------------------
# First: no planted fault left in the tree by a negative test that died (2026-11-13: a power cut mid-sabotage left a source
# all zeros). tools/safe_mutate.py puts every journaled original back, or FAILS when that needs a person: a file changed
# since, a damaged entry, or a run still holding one (one test run at a time).
Invoke-Gate 'no mutant left in place' 'py tools\safe_mutate.py'
Invoke-Gate 'L0 static' 'py tools\check_pawcore_static.py'
Invoke-Gate 'L1 PawCore tests' 'powershell -ExecutionPolicy Bypass -File tools\build_pawcore_tests.ps1'
Invoke-Gate 'mirror (PawCore vs pawsim)' 'py tools\check_mirror.py'
Invoke-Gate 'validator fuzz 3k' 'py tools\fuzz_validator.py --trials 3000'
# Sticker Box (D18, 2026-11-18): tools/art/sticker_cast.py is what the approved Bestiary (tools/art/concept_bestiary.html) draws,
# recorded by the concept's own code; a hand edit, or a changed concept that was not recorded again, fails here.
Invoke-Gate 'Sticker cast is the concept' 'node tools\art\sticker_record.js --check'
Invoke-Gate 'art generators reproduce' 'py tools\art\build_art.py --check'
Invoke-Gate 'art rasters reproduce' 'py tools\raster_svg.py --check'
Invoke-Gate 'field atlas reproduces' 'py tools\pack_atlas.py --check'
Invoke-Gate 'UI atlas reproduces' 'py tools\pack_ui_atlas.py --check'
# The restyle's look spec on the field's drawings (R5, 2026-11-09): tokens only, one ink, flat, clear of every floor, eyes
# that read at 56 px; KNOWN only shrinks. Its self-test plants one fault per rule (an off-palette fill must FAIL).
Invoke-Gate 'look lint' 'py tools\art\look_lint.py'
Invoke-Gate 'look lint self-test' 'py tools\art\look_lint.py --self-test'
Invoke-Gate 'font coverage' 'py tools\check_font_coverage.py'
# The text rule's own test (2026-11-08): its budgets and banned words flag planted faults. `--banned`: no table says
# "grudge", "unimpressed" or "prestige" (gated since 2026-11-09, when the designer's text pass renamed Grudge Ledger).
# The port's own English over Godot's (Data/TextOverrides.csv): its checks bite on planted faults (the generators run
# them for real in L0's `generated`).
Invoke-Gate 'text audit self-test' 'py tools\text_audit.py --self-test'
Invoke-Gate 'no banned words' 'py tools\text_audit.py --banned'
Invoke-Gate 'text overrides self-test' 'py tools\text_overrides.py --self-test'
Invoke-Gate 'text overrides agree' 'py tools\text_overrides.py --check'
# Phase 7 (2026-11-19): the translation files are in their written form (no row whose key is gone); the status tool
# tells stale from missing. No Godot file is read (D22).
Invoke-Gate 'translation files in form' 'py tools\port_po.py --check'
Invoke-Gate 'loc status self-test' 'py tools\loc_status.py --self-test'
# The wording review (2026-11-20): numbers kept, line breaks, nothing left in English, no mice for enemies (F55), the fixes
# to Godot's translations (F53, F54), one percent style per language; its self-test plants each fault.
Invoke-Gate 'translations lint' 'py tools\loc_lint.py'
Invoke-Gate 'translations lint self-test' 'py tools\loc_lint.py --self-test'
# Phase 8 (2026-11-23): the package audit's rules (tools/package.py: a Saved folder, a .pdb, content outside
# /Game/Pawpocalypse, a map, a string table or translation, a cursor, the ICU data) each fail on a planted listing. The
# packaging itself is the designer's word, never run here.
Invoke-Gate 'package audit self-test' 'py tools\package.py --self-test'
Invoke-Gate 'steam delta self-test' 'py tools\steam_delta.py --self-test'
# The packaged bench makes up to three runs (2026-11-24): the game thread on the best (one run's p99 moved 2.8-6.8 ms on the
# same package; interference only slows a run), every other check in every run; the self-test plants each case and the
# game's reason wording.
Invoke-Gate 'bench verdict self-test' 'py tools\packaged_bench.py --self-test'
# The PSO check's reading of a validation log (2026-11-24): a miss of one of this game's materials fails, the engine's
# materials and global shaders are noted; each event placed in time. The run itself needs a package (package.py's gates).
Invoke-Gate 'pso check self-test' 'py tools\packaged_pso.py --self-test'
# The game target's plugin trim is current (2026-11-23): an engine update added FabLauncher (editor-only, enabled by
# default), which pulled USD, Boost and TBB into the game until the trim ran again. Fails when a run would change the list.
Invoke-Gate 'game plugins trimmed' 'py tools\trim_game_plugins.py --check'
# Every sabotage's target still in its source (seconds, nothing built): a target a later slice rewrote otherwise stays unseen
# until the nightly's sabotage gate refuses to start (2026-11-06: one stale since slice 3 cost a whole night's realm gate).
Invoke-Gate 'sabotage targets current' 'py tools\sabotage_pawcore.py --list && py tools\sabotage_realm.py --list'
# --- the engine's own gates: a headless editor, so SKIP (not FAIL) while any Unreal editor is open -----------------
# An Unreal process of this project (or naming none: it may be the designer's editor) blocks the engine gates; another
# project's run does not (tools/unreal_procs.py). --wait, never one look: each gate starts the moment the last returns, and a
# look can catch a process on its way out (one look refused L3 UI in 1 s, 2026-11-13); the SKIP names what blocked.
function Test-UnrealBlocking {
	$script:UnrealBlockers = @(cmd /c "cd /d `"$Root`" && py tools\unreal_procs.py --wait 2>nul")
	return ($LASTEXITCODE -ne 0)
}
$script:EngineBuilt = $false
function Invoke-EngineGate([string]$Name, [string]$Command, [switch]$NeedsBuild) {
	if (Test-UnrealBlocking) {
		Write-Host ("{0,-28} SKIP  (an Unreal editor is running)" -f $Name)
		$script:UnrealBlockers | ForEach-Object { Write-Host $_ }
		[void]$Results.Add([pscustomobject]@{ Gate = $Name; Verdict = 'SKIP'; Seconds = 0; Log = '' })
	}
	elseif ($NeedsBuild -and -not $script:EngineBuilt) {
		# Never on the DLLs of an earlier build: what they would test is not this source.
		Write-Host ("{0,-28} FAIL  (not run: the editor build did not pass)" -f $Name)
		[void]$Results.Add([pscustomobject]@{ Gate = $Name; Verdict = 'FAIL'; Seconds = 0; Log = '' })
	}
	else {
		Invoke-Gate $Name $Command
	}
}
# The editor target (game module + PawCore), built before anything loads it.
Invoke-EngineGate 'UBT editor build' ('"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" PawpocalypseEditor Win64 Development ' +
	"-Project=`"$Root\Pawpocalypse.uproject`" -WaitMutex")
$script:EngineBuilt = @($Results | Where-Object { $_.Gate -eq 'UBT editor build' -and $_.Verdict -eq 'PASS' }).Count -gt 0
Invoke-EngineGate 'L3 DataTable parity' 'py tools\import_data.py --check' -NeedsBuild
Invoke-EngineGate 'L3 Pawpocalypse.Data' 'py tools\run_automation.py --no-build Pawpocalypse.Data' -NeedsBuild
Invoke-EngineGate 'L3 Pawpocalypse.Realm' 'py tools\run_automation.py --no-build Pawpocalypse.Realm' -NeedsBuild
Invoke-EngineGate 'L3 Pawpocalypse.UI' 'py tools\run_automation.py --no-build Pawpocalypse.UI' -NeedsBuild
Invoke-EngineGate 'L3 Pawpocalypse.Save' 'py tools\run_automation.py --no-build Pawpocalypse.Save' -NeedsBuild
Invoke-EngineGate 'L3 Pawpocalypse.Input' 'py tools\run_automation.py --no-build Pawpocalypse.Input' -NeedsBuild
Invoke-EngineGate 'L3 Pawpocalypse.Audio' 'py tools\run_automation.py --no-build Pawpocalypse.Audio' -NeedsBuild
# The GAME target too (what a package compiles): editor-only API (WITH_EDITOR / WITH_EDITORONLY_DATA) in game-module code
# builds in the editor and breaks only here (HANDOFF-LOG (41): two sessions of it went unseen until the packaged gate).
Invoke-EngineGate 'UBT game build' ('"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" Pawpocalypse Win64 Development ' +
	"-Project=`"$Root\Pawpocalypse.uproject`" -WaitMutex")
# And in Shipping (2026-11-23, first compiled then): Shipping strips the console, check() and dev code, so a dev-only call
# outside `#if !UE_BUILD_SHIPPING` breaks only here (KB U/19 §2). Incremental after the first build (~3 min cold).
Invoke-EngineGate 'UBT game build (Shipping)' ('"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" Pawpocalypse Win64 Shipping ' +
	"-Project=`"$Root\Pawpocalypse.uproject`" -WaitMutex")

if ($Full) {
	# --- every gate's own negative test ---------------------------------------------------------------------------
	Invoke-Gate 'L0 self-test' 'py tools\check_pawcore_static.py --self-test'
	Invoke-Gate 'mirror self-test' 'py tools\check_mirror.py --self-test'
	Invoke-Gate 'font coverage self-test' 'py tools\check_font_coverage.py --self-test'
	Invoke-Gate 'Unreal guard self-test' 'py tools\unreal_procs.py --self-test'
	Invoke-Gate 'safe-mutate self-test' 'py tools\safe_mutate.py --self-test'
	Invoke-Gate 'PawCore sabotage' 'py tools\sabotage_pawcore.py'
	Invoke-EngineGate 'DataTable parity self-test' 'py tools\import_data.py --self-test' -NeedsBuild
	# --- the long runs --------------------------------------------------------------------------------------------
	Invoke-Gate 'L1 long suite' 'powershell -ExecutionPolicy Bypass -File tools\build_pawcore_tests.ps1 -Long'
	Invoke-Gate 'mirror 200k states' 'py tools\check_mirror.py --random 200000'
	Invoke-Gate 'validator fuzz 30k' 'py tools\fuzz_validator.py --trials 30000 --seed 7'
	# L4: the pad-only playthrough and the focus tour, in a real window (-game, ~2 min; sandboxed saves).
	Invoke-EngineGate 'L4 pad-only playthrough' 'py tools\padplay_tour.py' -NeedsBuild
	# The realm's engine tests, each sabotaged in the game module and seen to fail (UBT per sabotage: ~1 min each; the
	# editor build itself is an everyday gate now, before the L3 gates).
	Invoke-EngineGate 'Realm sabotage' 'py tools\sabotage_realm.py'
}


Write-Host ''
$Results | Format-Table Gate, Verdict, Seconds, Log -AutoSize | Out-String -Width 200 | Write-Host
$Failed = @($Results | Where-Object { $_.Verdict -eq 'FAIL' })
if ($Failed.Count -gt 0) {
	Write-Host "check_all: $($Failed.Count) gate(s) FAILED"
	exit 1
}
$Skipped = @($Results | Where-Object { $_.Verdict -eq 'SKIP' })
if ($Skipped.Count -gt 0) {
	# Not a pass: a skipped gate proved nothing (close the editor and rerun, or run those gates alone).
	Write-Host "check_all: NOT PROVEN - every gate that ran passed, but $($Skipped.Count) SKIPPED: $(($Skipped | ForEach-Object { $_.Gate }) -join ', ')"
	exit 2
}
Write-Host 'check_all: every gate passed'
exit 0
