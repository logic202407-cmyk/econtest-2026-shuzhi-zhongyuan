param(
    [string]$BuildDirectory = "build/host-tests"
)

$ErrorActionPreference = "Stop"

$root = Resolve-Path (Join-Path $PSScriptRoot "..")
$output = Join-Path $root $BuildDirectory
$executable = Join-Path $output "test_application_logic.exe"
$lockExecutable = Join-Path $output "test_gimbal_motion_lock.exe"
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

New-Item -ItemType Directory -Force -Path $output | Out-Null

if (Get-Command cl.exe -ErrorAction SilentlyContinue) {
    & cl.exe /nologo /std:c11 /W4 /DGIMBAL_MOTION_ENABLED=1 /I$root /Fe:$executable $sources
    if ($LASTEXITCODE -eq 0) {
        & cl.exe /nologo /std:c11 /W4 /I$root /Fe:$lockExecutable $lockSources
    }
} elseif (Get-Command gcc.exe -ErrorAction SilentlyContinue) {
    & gcc.exe -std=c11 -Wall -Wextra -Werror -DGIMBAL_MOTION_ENABLED=1 -I$root -o $executable $sources
    if ($LASTEXITCODE -eq 0) {
        & gcc.exe -std=c11 -Wall -Wextra -Werror -I$root -o $lockExecutable $lockSources
    }
} else {
    throw "No host C compiler found. Install Visual Studio Build Tools or a GCC toolchain."
}

if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

& $executable
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

& $lockExecutable
exit $LASTEXITCODE
