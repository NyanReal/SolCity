param(
    [switch]$Once,
    [switch]$Import,
    [int]$PollSeconds = 10,
    [string]$EngineRoot = "C:\Program Files\Epic Games\UE_5.7"
)

$ErrorActionPreference = "Stop"
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$QueuePath = Join-Path $ProjectRoot "ExternalImageRequests.md"
$ManifestPath = Join-Path $ProjectRoot "Config\GeneratedAssetManifest.json"
$ProjectFile = Join-Path $ProjectRoot "SolCity.uproject"
$PythonScript = Join-Path $ProjectRoot "Content\Python\sync_generated_assets.py"
$EditorCmd = Join-Path $EngineRoot "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"

function Update-CompletionStates {
    $Manifest = Get-Content -LiteralPath $ManifestPath -Raw -Encoding UTF8 | ConvertFrom-Json
    $Content = Get-Content -LiteralPath $QueuePath -Raw -Encoding UTF8
    $Changed = $false
    $NeedsImport = $false

    foreach ($Asset in $Manifest.assets) {
        $SourcePath = Join-Path $ProjectRoot $Asset.source
        if (Test-Path -LiteralPath $SourcePath) {
            $Id = [Regex]::Escape($Asset.id)
            $SectionPattern = "(?ms)##\s+$Id\b.*?(?=^##\s+REQ-|\z)"
            $SectionMatch = [Regex]::Match($Content, $SectionPattern)
            if (-not $SectionMatch.Success) {
                continue
            }

            # Each request has four ordered checkbox rows: request, assignment,
            # generation, and UE import. Matching by position keeps this script
            # ASCII-only so Windows PowerShell 5 does not depend on a UTF-8 BOM.
            $Section = $SectionMatch.Value
            $StatusMatches = [Regex]::Matches($Section, "(?m)^-\s+[^:\r\n]+:\s*\[[ xX]\]")
            if ($StatusMatches.Count -lt 4) {
                continue
            }

            $GenerationStatus = $StatusMatches[2]
            if ($GenerationStatus.Value -match "\[\s\]") {
                $CheckedStatus = [Regex]::Replace($GenerationStatus.Value, "\[\s\]", "[x]", 1)
                $Section = $Section.Remove($GenerationStatus.Index, $GenerationStatus.Length).Insert($GenerationStatus.Index, $CheckedStatus)
                $Content = $Content.Remove($SectionMatch.Index, $SectionMatch.Length).Insert($SectionMatch.Index, $Section)
                $Changed = $true
                Write-Host "Detected completed image: $($Asset.id)"
            }

            if ($StatusMatches[3].Value -match "\[\s\]") {
                $NeedsImport = $true
            }
        }
    }

    if ($Changed) {
        Set-Content -LiteralPath $QueuePath -Value $Content -Encoding UTF8 -NoNewline
    }
    return [PSCustomObject]@{
        Changed = $Changed
        NeedsImport = $NeedsImport
    }
}

function Invoke-UnrealImport {
    if (-not (Test-Path -LiteralPath $EditorCmd)) {
        throw "UnrealEditor-Cmd.exe was not found under $EngineRoot"
    }
    & $EditorCmd $ProjectFile "-ExecutePythonScript=$PythonScript" -unattended -nop4 -nosplash
    if ($LASTEXITCODE -ne 0) {
        throw "Unreal asset import exited with code $LASTEXITCODE"
    }
}

do {
    $State = Update-CompletionStates
    if ($Import -and $State.NeedsImport) {
        Invoke-UnrealImport
    }
    if (-not $Once) {
        Start-Sleep -Seconds ([Math]::Max(2, $PollSeconds))
    }
} while (-not $Once)
