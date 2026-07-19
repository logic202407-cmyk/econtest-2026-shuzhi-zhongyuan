param(
    [string]$BuildDirectory = "build/host-tests",
    [string]$ArmclangPath = ""
)

$ErrorActionPreference = "Stop"

$root = Resolve-Path (Join-Path $PSScriptRoot "..")
if ([System.IO.Path]::IsPathRooted($BuildDirectory)) {
    $output = $BuildDirectory
} else {
    $output = Join-Path $root $BuildDirectory
}
$executable = Join-Path $output "test_application_logic.exe"
$lockExecutable = Join-Path $output "test_gimbal_motion_lock.exe"
$stm32VisionExecutable = Join-Path $output "test_stm32_vision_ascii.exe"
$sources = @(
    "tests/test_application_logic.c",
    "application/vision/maixcam_protocol.c",
    "application/motor/x42s_rs485/x42s_rs485.c",
    "application/gimbal/gimbal_control.c"
) | ForEach-Object { Join-Path $root $_ }
$lockSources = @(
    "tests/test_gimbal_motion_lock.c",
    "application/vision/maixcam_protocol.c",
    "application/motor/x42s_rs485/x42s_rs485.c",
    "application/gimbal/gimbal_control.c"
) | ForEach-Object { Join-Path $root $_ }
$stm32VisionSources = @(
    "tests/test_stm32_vision_ascii.c",
    "firmware/stm32_f407/skystar_stdperiph_project/app/vision_ascii_protocol.c"
) | ForEach-Object { Join-Path $root $_ }

New-Item -ItemType Directory -Force -Path $output | Out-Null

function Resolve-Armclang {
    param([string]$RequestedPath)

    if ($RequestedPath -ne "") {
        if (Test-Path -LiteralPath $RequestedPath) {
            return (Resolve-Path -LiteralPath $RequestedPath).Path
        }
        throw "ArmclangPath does not exist: $RequestedPath"
    }

    $command = Get-Command armclang.exe -ErrorAction SilentlyContinue
    if ($null -ne $command) {
        return $command.Source
    }

    return ""
}

function Invoke-ArmclangCompileSet {
    param(
        [string]$Compiler,
        [string[]]$CompileSources,
        [string]$ObjectSubdir,
        [string[]]$Defines
    )

    $objectDir = Join-Path $output $ObjectSubdir
    New-Item -ItemType Directory -Force -Path $objectDir | Out-Null

    $index = 0
    foreach ($source in $CompileSources) {
        $base = [System.IO.Path]::GetFileNameWithoutExtension($source)
        $object = Join-Path $objectDir ("{0:D2}_{1}.o" -f $index, $base)
        $index += 1
        $args = @(
            "-c",
            "--target=arm-arm-none-eabi",
            "-mcpu=cortex-m0plus",
            "-std=c11",
            "-Wall",
            "-Werror",
            "-I$root"
        ) + $Defines + @(
            $source,
            "-o",
            $object
        )

        & $Compiler @args
        if ($LASTEXITCODE -ne 0) {
            exit $LASTEXITCODE
        }
    }
}

if (Get-Command cl.exe -ErrorAction SilentlyContinue) {
    & cl.exe /nologo /std:c11 /W4 /DGIMBAL_MOTION_ENABLED=1 /I$root /Fe:$executable $sources
    if ($LASTEXITCODE -eq 0) {
        & cl.exe /nologo /std:c11 /W4 /I$root /Fe:$lockExecutable $lockSources
    }
    if ($LASTEXITCODE -eq 0) {
        & cl.exe /nologo /std:c11 /W4 /I$root /Fe:$stm32VisionExecutable $stm32VisionSources
    }
} elseif (Get-Command gcc.exe -ErrorAction SilentlyContinue) {
    & gcc.exe -std=c11 -Wall -Wextra -Werror -DGIMBAL_MOTION_ENABLED=1 -I$root -o $executable $sources
    if ($LASTEXITCODE -eq 0) {
        & gcc.exe -std=c11 -Wall -Wextra -Werror -I$root -o $lockExecutable $lockSources
    }
    if ($LASTEXITCODE -eq 0) {
        & gcc.exe -std=c11 -Wall -Wextra -Werror -I$root -o $stm32VisionExecutable $stm32VisionSources
    }
} else {
    $armclang = Resolve-Armclang -RequestedPath $ArmclangPath
    if ($armclang -eq "") {
        throw "No host C compiler found. Install Visual Studio Build Tools/GCC, or rerun with -ArmclangPath <path-to-armclang.exe> for compile-only checks."
    }

    Write-Host "No runnable host C compiler found; using ARMCLANG for compile-only checks."
    Invoke-ArmclangCompileSet -Compiler $armclang -CompileSources $sources `
        -ObjectSubdir "armclang-enabled" -Defines @("-DGIMBAL_MOTION_ENABLED=1")
    Invoke-ArmclangCompileSet -Compiler $armclang -CompileSources $lockSources `
        -ObjectSubdir "armclang-locked" -Defines @()
    Invoke-ArmclangCompileSet -Compiler $armclang -CompileSources $stm32VisionSources `
        -ObjectSubdir "armclang-stm32-vision" -Defines @()
    Write-Host "ARMCLANG compile-only checks passed. Host executables were not run."
    exit 0
}

if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

& $executable
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

& $lockExecutable
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

& $stm32VisionExecutable
exit $LASTEXITCODE
