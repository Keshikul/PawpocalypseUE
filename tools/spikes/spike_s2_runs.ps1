# SPIKE S2 (docs/04 §8): the crowd under each renderer, in the PACKAGED Development build (build/package_s2). Throwaway.
# Windowed runs (a game window per run). -graphicsadapter=1 = the integrated AMD Radeon on the dev PC: a GPU weaker than
# the Steam Deck's, so its 1280x800 numbers are a worst case for the Deck-class budget (docs/04 §9).
# Results: build/package_s2/Windows/Pawpocalypse/Saved/Spikes/S2/<run>.txt
param([string[]]$Only)
$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$Exe = "$Root\build\package_s2\Windows\Pawpocalypse.exe"
$Out = "$Root\build\package_s2\Windows\Pawpocalypse\Saved\Spikes\S2"
if (-not (Test-Path $Exe)) { throw "Package first: $Exe missing" }
if (Get-Process Pawpocalypse* -ErrorAction SilentlyContinue) { throw 'A packaged Pawpocalypse is running: close it first.' }

$Renderers = [ordered]@{ ism = '-S2Renderer=ism'; paper = '-S2Renderer=paper'; slate = '-S2Renderer=slate'; slatestrict = '-S2Renderer=slate -S2SlateStrict' }
$Setups = [ordered]@{ '1080_dgpu' = '-ResX=1920 -ResY=1080'; '800_dgpu' = '-ResX=1280 -ResY=800'; '800_igpu' = '-ResX=1280 -ResY=800 -graphicsadapter=1' }
foreach ($s in $Setups.Keys) {
	foreach ($r in $Renderers.Keys) {
		$name = "${r}_$s"
		if ($Only -and $name -notin $Only) { continue }
		$args = "/Game/Pawpocalypse/Dev/Spikes/S2/L_SpikeCrowd -windowed -WinX=0 -WinY=0 -ForceRes -unattended -nosound -nosplash $($Setups[$s]) $($Renderers[$r]) -S2Run=$name -S2Shot"
		$t = Get-Date
		$p = Start-Process -FilePath $Exe -ArgumentList $args -PassThru -Wait
		Write-Host "== $name (exit $($p.ExitCode), $([int]((Get-Date) - $t).TotalSeconds)s)"
		if (Test-Path "$Out\$name.txt") { Get-Content "$Out\$name.txt" | ForEach-Object { Write-Host "   $_" } } else { Write-Host '   NO RESULT' }
	}
}
