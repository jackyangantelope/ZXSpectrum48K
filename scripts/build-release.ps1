[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$PicoSdkPath,
    [Parameter(Mandatory = $true)][string]$PicoPioUsbPath,
    [Parameter(Mandatory = $true)][string]$ToolchainPath,
    [Parameter(Mandatory = $true)][string]$NinjaPath,
    [string]$PioasmDir,
    [string]$PicotoolDir,
    [string]$BuildDir,
    [string]$CmakeExe = 'cmake'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$ProjectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
if (-not $BuildDir) { $BuildDir = Join-Path $ProjectRoot 'build/fruit-jam-release' }
$BuildDir = [IO.Path]::GetFullPath($BuildDir)
$env:PICO_SDK_PATH = [IO.Path]::GetFullPath($PicoSdkPath)
$env:PICO_PIO_USB_PATH = [IO.Path]::GetFullPath($PicoPioUsbPath)
if (-not (Test-Path -LiteralPath (Join-Path $env:PICO_SDK_PATH 'pico_sdk_init.cmake'))) {
    throw "Pico SDK not found: $env:PICO_SDK_PATH"
}
if (-not (Test-Path -LiteralPath (Join-Path $env:PICO_PIO_USB_PATH 'src/pio_usb.c'))) {
    throw "Pico-PIO-USB not found: $env:PICO_PIO_USB_PATH"
}
if (-not (Test-Path -LiteralPath $NinjaPath -PathType Leaf)) {
    throw "Ninja not found: $NinjaPath"
}
$CmakeArgs = @(
    '-S', $ProjectRoot, '-B', $BuildDir, '-G', 'Ninja',
    "-DCMAKE_MAKE_PROGRAM:FILEPATH=$NinjaPath",
    "-DPICO_SDK_PATH=$env:PICO_SDK_PATH",
    "-DPICO_PIO_USB_PATH=$env:PICO_PIO_USB_PATH",
    "-DPICO_TOOLCHAIN_PATH=$([IO.Path]::GetFullPath($ToolchainPath))",
    '-DPICO_PLATFORM=rp2350-arm-s', '-DPICO_BOARD=adafruit_fruit_jam',
    '-DCMAKE_BUILD_TYPE=Release'
)
if ($PioasmDir) { $CmakeArgs += "-Dpioasm_DIR=$([IO.Path]::GetFullPath($PioasmDir))" }
if ($PicotoolDir) { $CmakeArgs += "-Dpicotool_DIR=$([IO.Path]::GetFullPath($PicotoolDir))" }
& $CmakeExe @CmakeArgs
if ($LASTEXITCODE -ne 0) { throw "CMake configure failed: $LASTEXITCODE" }
& $CmakeExe --build $BuildDir --target picoZxSpectrum48 --parallel 4
if ($LASTEXITCODE -ne 0) { throw "Build failed: $LASTEXITCODE" }
Write-Host (Join-Path $BuildDir 'picoZxSpectrum48.uf2')
