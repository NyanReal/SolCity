param(
    [ValidateSet("Debug", "DebugGame", "Development", "Shipping")]
    [string]$Configuration = "Development",
    [string]$EngineRoot = "C:\Program Files\Epic Games\UE_5.7"
)

$ErrorActionPreference = "Stop"
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$ProjectFile = Join-Path $ProjectRoot "SolCity.uproject"
$BuildScript = Join-Path $EngineRoot "Engine\Build\BatchFiles\Build.bat"

if (-not (Test-Path -LiteralPath $BuildScript)) {
    throw "UE 5.7 Build.bat was not found under $EngineRoot"
}

& $BuildScript SolCityEditor Win64 $Configuration $ProjectFile -WaitMutex -NoHotReloadFromIDE
if ($LASTEXITCODE -ne 0) {
    throw "SolCityEditor build failed with exit code $LASTEXITCODE"
}
