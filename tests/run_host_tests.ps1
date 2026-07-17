param(
    [string]$BuildDirectory = "build/host-tests"
)

$ErrorActionPreference = "Stop"

$root = Resolve-Path (Join-Path $PSScriptRoot "..")
$output = Join-Path $root $BuildDirectory
$executable = Join-Path $output "test_application_logic.exe"
$sources = @(
    "tests/test_application_logic.c",
    "application/vision/maixcam_protocol.c",
    "application/motor/x42s_rs485/x42s_rs485.c",
    "application/gimbal/gimbal_control.c"
) | ForEach-Object { Join-Path $root $_ }

New-Item -ItemType Directory -Force -Path $output | Out-Null

if (Get-Command cl.exe -ErrorAction SilentlyContinue) {
    & cl.exe /nologo /std:c11 /W4 /I$root /Fe:$executable $sources
} elseif (Get-Command gcc.exe -ErrorAction SilentlyContinue) {
    & gcc.exe -std=c11 -Wall -Wextra -Werror -I$root -o $executable $sources
} else {
    throw "No host C compiler found. Install Visual Studio Build Tools or a GCC toolchain."
}

if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

& $executable
exit $LASTEXITCODE
