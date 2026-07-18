param(
    [Parameter(Mandatory = $true)]
    [string]$SeekFreeSourceDir,

    [string]$OutputDir = "build/MSPM0G3507_Library-TianMengXing-V3.3.4",
    [string]$PythonPath = "",
    [string]$KeilUv4Path = "",
    [switch]$Force,
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"

function Resolve-RepoRoot {
    return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

function Resolve-Executable {
    param(
        [string]$ExplicitPath,
        [string[]]$CommandNames,
        [string]$DisplayName
    )

    if ($ExplicitPath -ne "") {
        if (-not (Test-Path -LiteralPath $ExplicitPath)) {
            throw "$DisplayName was not found: $ExplicitPath"
        }
        return (Resolve-Path -LiteralPath $ExplicitPath).Path
    }

    foreach ($name in $CommandNames) {
        $command = Get-Command $name -ErrorAction SilentlyContinue
        if ($null -ne $command) {
            return $command.Source
        }
    }

    throw "$DisplayName was not found. Pass its full path with the matching parameter."
}

function Resolve-OutputPath {
    param(
        [string]$RepoRoot,
        [string]$PathText
    )

    if ([System.IO.Path]::IsPathRooted($PathText)) {
        return $PathText
    }

    return (Join-Path $RepoRoot $PathText)
}

function Assert-PathExists {
    param(
        [string]$PathText,
        [string]$Label
    )

    if (-not (Test-Path -LiteralPath $PathText)) {
        throw "$Label is missing: $PathText"
    }
}

function Assert-ProjectReferencesExist {
    param([string]$ProjectPath)

    [xml]$xml = Get-Content -LiteralPath $ProjectPath -Encoding UTF8
    $projectDir = Split-Path $ProjectPath
    $missing = @()

    foreach ($node in $xml.SelectNodes("//FilePath")) {
        $candidate = Join-Path $projectDir $node.InnerText
        if (-not (Test-Path -LiteralPath $candidate)) {
            $missing += $node.InnerText
        }
    }

    if ($missing.Count -ne 0) {
        throw "Keil project contains missing FilePath entries:`n$($missing -join "`n")"
    }
}

function Assert-ProjectContainsApplication {
    param([string]$ProjectPath)

    $projectText = Get-Content -LiteralPath $ProjectPath -Encoding UTF8 -Raw
    $requiredPatterns = @(
        "<GroupName>application</GroupName>",
        "..\application\app_main.c",
        "..\application\platform\platform_uart.c",
        "..\application\platform\platform_gpio.c",
        "..\application\vision\maixcam_protocol.c",
        "..\application\motor\x42s_rs485\x42s_rs485.c",
        "..\application\gimbal\gimbal_control.c",
        "..\application;..\application\config;..\application\platform"
    )

    foreach ($pattern in $requiredPatterns) {
        if (-not $projectText.Contains($pattern)) {
            throw "Keil project is missing expected application entry: $pattern"
        }
    }
}

function Assert-SysConfigContainsFrozenMap {
    param([string]$SyscfgPath)

    $text = Get-Content -LiteralPath $SyscfgPath -Encoding UTF8 -Raw
    $requiredPatterns = @(
        'GPIO1.$name                              = "LED_PB22";',
        'GPIO3.$name                              = "RS485_DE";',
        'GPIO3.associatedPins[0].pin.$assign      = "PB17";',
        'UART1.$name                    = "UART_MAIXCAM";',
        'UART1.peripheral.rxPin.$assign = "PA9";',
        'UART1.peripheral.txPin.$assign = "PA8";',
        'UART2.$name                    = "UART_X42S";',
        'UART2.peripheral.rxPin.$assign = "PB16";',
        'UART2.peripheral.txPin.$assign = "PB15";'
    )

    foreach ($pattern in $requiredPatterns) {
        if (-not $text.Contains($pattern)) {
            throw "SysConfig is missing expected frozen mapping: $pattern"
        }
    }
}

function Invoke-KeilBuild {
    param(
        [string]$Uv4Path,
        [string]$ProjectPath
    )

    $projectDir = Split-Path $ProjectPath
    & $Uv4Path -b $ProjectPath -j0
    if ($LASTEXITCODE -ne 0) {
        throw "Keil UV4 rebuild failed with exit code $LASTEXITCODE"
    }

    $objectsDir = Join-Path $projectDir "Objects"
    $log = $null
    for ($attempt = 0; $attempt -lt 60; $attempt += 1) {
        if (Test-Path -LiteralPath $objectsDir) {
            $log = Get-ChildItem -LiteralPath $objectsDir -Filter "*.build_log.htm" |
                Sort-Object LastWriteTime -Descending |
                Select-Object -First 1
        }

        if ($null -ne $log) {
            break
        }

        Start-Sleep -Milliseconds 500
    }

    if ($null -eq $log) {
        throw "Keil build log was not found under Objects/"
    }

    $logText = ""
    for ($attempt = 0; $attempt -lt 60; $attempt += 1) {
        $logText = Get-Content -LiteralPath $log.FullName -Encoding Default -Raw
        if ($logText -match 'Error\(s\),') {
            break
        }
        Start-Sleep -Milliseconds 500
    }

    if ($logText -notmatch 'Error\(s\),') {
        throw "Keil build log did not reach its final summary: $($log.FullName)"
    }

    if ($logText -notmatch '0 Error\(s\), 0 Warning\(s\)') {
        $summary = ($logText -split "`r?`n" | Select-String -Pattern 'Error\(s\)|Warning\(s\)|error:|warning:|Target not created' | ForEach-Object { $_.Line }) -join "`n"
        throw "Keil rebuild did not finish cleanly:`n$summary"
    }

    return $log.FullName
}

$repoRoot = Resolve-RepoRoot
$source = (Resolve-Path -LiteralPath $SeekFreeSourceDir).Path
$output = Resolve-OutputPath -RepoRoot $repoRoot -PathText $OutputDir
$materializer = Join-Path $repoRoot "firmware/mspm0g3507/tianmengxing/scripts/materialize.py"

Assert-PathExists -PathText $materializer -Label "Materializer"
Assert-PathExists -PathText (Join-Path $source "SeekFree_MSPM0G3507_Opensource_Library") -Label "SeekFree library"
Assert-PathExists -PathText (Join-Path $source "Example/Coreboard_Demo") -Label "SeekFree Coreboard demo"

$python = Resolve-Executable -ExplicitPath $PythonPath -CommandNames @("python.exe", "py.exe") -DisplayName "Python"
$uv4 = ""
if (-not $SkipBuild) {
    if ($KeilUv4Path -eq "" -and $env:KEIL_UV4_PATH -ne $null) {
        $KeilUv4Path = $env:KEIL_UV4_PATH
    }
    $uv4 = Resolve-Executable -ExplicitPath $KeilUv4Path -CommandNames @("UV4.exe") -DisplayName "Keil UV4"
}

if ((Test-Path -LiteralPath $output) -and -not $Force) {
    throw "Output directory already exists: $output. Re-run with -Force or choose a new -OutputDir."
}

$materializeArgs = @($materializer, $source, "--output", $output)
if ($Force) {
    $materializeArgs += "--force"
}

Write-Host "Generating TianMengXing workspace..."
& $python @materializeArgs
if ($LASTEXITCODE -ne 0) {
    throw "materialize.py failed with exit code $LASTEXITCODE"
}

$libraryRoot = Join-Path $output "SeekFree_MSPM0G3507_Opensource_Library"
$demoRoot = Join-Path $output "Example/TianMengXing_Coreboard_Demo"
$projectPath = Join-Path $libraryRoot "project/mdk/SeekFree_MSPM0G3507_Device_Library.uvprojx"
$syscfgPath = Join-Path $libraryRoot "libraries/sdk/ti_config/SeekFree_MSPM0G3507_Device_Library.syscfg"

Assert-PathExists -PathText $libraryRoot -Label "Generated SeekFree library"
Assert-PathExists -PathText $demoRoot -Label "Generated TianMengXing demo"
Assert-PathExists -PathText $projectPath -Label "Keil project"
Assert-PathExists -PathText $syscfgPath -Label "SysConfig source"

Assert-ProjectContainsApplication -ProjectPath $projectPath
Assert-ProjectReferencesExist -ProjectPath $projectPath
Assert-SysConfigContainsFrozenMap -SyscfgPath $syscfgPath

$buildLog = ""
if (-not $SkipBuild) {
    Write-Host "Running Keil rebuild..."
    $buildLog = Invoke-KeilBuild -Uv4Path $uv4 -ProjectPath $projectPath
}

Write-Host ""
Write-Host "TianMengXing Keil workspace is ready."
Write-Host "Output: $output"
Write-Host "Project: $projectPath"
if ($buildLog -ne "") {
    Write-Host "Build log: $buildLog"
    Write-Host "Keil result: 0 Error(s), 0 Warning(s)"
} else {
    Write-Host "Keil rebuild skipped."
}
