# SPIKE S3 (docs/04 §8): capture every raster variant x compression x resolution, offscreen, then compare. Throwaway.
# Results: Saved/Spikes/S3/<variant>_<comp>_<res>.png; logs Saved/Logs/S3_*.log; the comparison table on stdout.
#
# Guard: another Unreal PROJECT may be cooking on this machine (it happened: a different project's package_win64.bat), which
# does not touch this project's files. What must not run at the same time is an editor on THIS project: a GUI editor shows
# the project name in its window title. Command lines of UnrealEditor-Cmd processes are not readable from here.
$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$Editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
function Test-OwnEditor { [bool](Get-Process UnrealEditor -ErrorAction SilentlyContinue | Where-Object { $_.MainWindowTitle -match 'Pawpocalypse' }) }
foreach ($v in 'r1x', 'r2x', 'godot') {
	foreach ($c in 'bc7', 'ui') {
		foreach ($r in 1080, 2160) {
			if (Test-OwnEditor) { throw 'An editor is open on Pawpocalypse: close it first.' }
			$name = "${v}_${c}_$r"
			$args = "`"$Root\Pawpocalypse.uproject`" /Game/Pawpocalypse/Dev/Spikes/S3/L_SpikeArt -game -RenderOffscreen -ResX=1280 -ResY=720 -ForceRes -windowed -unattended -nosound -nosplash -S3Variant=$v -S3Comp=$c -S3Res=$r -log=S3_$name.log"
			$p = Start-Process -FilePath $Editor -ArgumentList $args -PassThru -Wait
			$lines = Select-String -Path "$Root\Saved\Logs\S3_$name.log" -Pattern 'S3 run|S3 wrote|S3: ' | ForEach-Object { ($_.Line -split 'Display: |Error: ')[-1] }
			Write-Host "== $name exit=$($p.ExitCode) $($lines -join ' | ')"
		}
	}
}
py "$Root\tools\spikes\spike_s3_compare.py"
