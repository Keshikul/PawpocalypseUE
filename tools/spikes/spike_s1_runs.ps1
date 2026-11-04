# SPIKE S1 (docs/04 §8): run the AA options one after another. Throwaway.
#   default     offscreen, scene-capture readback (valid for colour, NOT for AA: doc 04 §10)
#   -Windowed   a real game window, back-buffer readback (what the player sees); opens a window per run
# Results: Saved/Spikes/S1/<run>_<capture|screen>.png/.txt, logs Saved/Logs/S1_<run>.log.
# Then: py tools/spikes/spike_s1_compare.py <run>_screen ...
param([switch]$Windowed, [string[]]$Only)
$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$Editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$Fwd = '-ini:Engine:[/Script/Engine.RendererSettings]:r.ForwardShading=True'
if ($Windowed) {
	$Runs = [ordered]@{
		'W1080_N_None'  = '-S1AA=0 -ResX=1920 -ResY=1080'
		'W1080_C_FXAA'  = '-S1AA=1 -ResX=1920 -ResY=1080'
		'W1080_B2_TAA'  = '-S1AA=2 -ResX=1920 -ResY=1080'
		'W1080_B_TSR'   = '-S1AA=4 -ResX=1920 -ResY=1080'
		'W1080_A0_MSAA' = "-S1AA=3 $Fwd -ResX=1920 -ResY=1080"
		'W800_C_FXAA'   = '-S1AA=1 -ResX=1280 -ResY=800'
		'W800_B2_TAA'   = '-S1AA=2 -ResX=1280 -ResY=800'
		'W800_B_TSR'    = '-S1AA=4 -ResX=1280 -ResY=800'
		'W800_A0_MSAA'  = "-S1AA=3 $Fwd -ResX=1280 -ResY=800"
	}
	$Mode = '-windowed -WinX=0 -WinY=0'
} else {
	$Runs = [ordered]@{
		'N_None'     = '-S1AA=0'
		'C_FXAA'     = '-S1AA=1'
		'B2_TAA'     = '-S1AA=2'
		'B_TSR'      = '-S1AA=4'
		'A0_MSAA'    = "-S1AA=3 $Fwd"
		'A_MSAA_A2C' = "-S1AA=3 -S1A2C $Fwd"
	}
	$Mode = '-RenderOffscreen -ResX=1920 -ResY=1080 -windowed'
}
if (Get-Process UnrealEditor*, UnrealEditor-Cmd -ErrorAction SilentlyContinue) { throw 'An editor is running: close it first.' }
foreach ($name in $Runs.Keys) {
	if ($Only -and $name -notin $Only) { continue }
	$t = Get-Date
	$args = "`"$Root\Pawpocalypse.uproject`" /Game/Pawpocalypse/Dev/Spikes/S1/L_SpikeLook -game $Mode -ForceRes -unattended -nosound -nosplash -S1Run=$name $($Runs[$name]) -log=S1_$name.log"
	$p = Start-Process -FilePath $Editor -ArgumentList $args -PassThru -Wait
	$secs = [int]((Get-Date) - $t).TotalSeconds
	$line = Select-String -Path "$Root\Saved\Logs\S1_$name.log" -Pattern 'S1 run ''|S1 \[(capture|screen)\] (run=|colour)|ensure' | ForEach-Object { ($_.Line -split 'Display: |Error: ')[-1] }
	Write-Host "== $name (exit $($p.ExitCode), ${secs}s)"
	$line | ForEach-Object { Write-Host "   $_" }
}
