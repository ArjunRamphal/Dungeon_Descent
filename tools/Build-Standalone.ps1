[CmdletBinding()]
param(
    [switch]$Clean,
    [switch]$Rebuild
)

$ErrorActionPreference = 'Stop'
$sourceRoot = Split-Path -Parent $PSScriptRoot
$buildRoot = Join-Path $sourceRoot 'build\standalone'
$destination = Join-Path $sourceRoot 'dist\Dungeon_Descent.exe'

function Find-Application([string]$Name, [string]$Fallback) {
    $command = Get-Command $Name -CommandType Application -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($command) { return $command.Source }
    if ($Fallback -and (Test-Path -LiteralPath $Fallback)) { return $Fallback }
    return $null
}

try {
    $vsRoot = $env:VSINSTALLDIR
    if (-not $vsRoot -or -not (Test-Path -LiteralPath (Join-Path $vsRoot 'Common7\Tools\VsDevCmd.bat'))) {
        $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
        if (-not (Test-Path -LiteralPath $vswhere)) {
            throw 'Install Visual Studio 2022 with Desktop development with C++ and C++ CMake tools for Windows.'
        }
        $vsRoot = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
        if ($LASTEXITCODE -ne 0 -or -not $vsRoot) { throw 'An MSVC x64 toolchain was not found.' }
        $vsRoot = $vsRoot.Trim()
    }

    # Import the x64 compiler environment even when launched from regular Visual Studio or PowerShell.
    $devCommand = Join-Path $vsRoot 'Common7\Tools\VsDevCmd.bat'
    $compilerEnvironment = & $env:ComSpec /d /c "call `"$devCommand`" -arch=x64 -host_arch=x64 >nul && set"
    if ($LASTEXITCODE -ne 0) { throw 'Visual Studio could not initialise its x64 build environment.' }
    foreach ($line in $compilerEnvironment) {
        $separator = $line.IndexOf('=')
        if ($separator -gt 0) {
            [Environment]::SetEnvironmentVariable($line.Substring(0, $separator), $line.Substring($separator + 1), 'Process')
        }
    }

    $cmake = Find-Application 'cmake.exe' (Join-Path $vsRoot 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe')
    if (-not $cmake) { throw 'Install CMake 3.24 or newer, or add C++ CMake tools for Windows in Visual Studio Installer.' }

    if ($Clean) {
        if (Test-Path -LiteralPath (Join-Path $buildRoot 'CMakeCache.txt')) {
            & $cmake --build $buildRoot --config Release --target clean
            if ($LASTEXITCODE -ne 0) { throw 'Standalone clean failed.' }
        }
        if (Test-Path -LiteralPath $destination) { Remove-Item -LiteralPath $destination }
        exit 0
    }

    if (-not (Find-Application 'git.exe' '')) { throw 'Install Git for Windows and make git.exe available on PATH for the first dependency build.' }
    $ninja = Find-Application 'ninja.exe' (Join-Path $vsRoot 'Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe')
    $generator = if ($ninja) { 'Ninja' } else { 'NMake Makefiles' }
    $cache = Join-Path $buildRoot 'CMakeCache.txt'
    if (Test-Path -LiteralPath $cache) {
        $existing = Select-String -LiteralPath $cache -Pattern '^CMAKE_GENERATOR:INTERNAL=(.+)$'
        if ($existing) { $generator = $existing.Matches[0].Groups[1].Value }
        if ($generator -notin @('Ninja', 'NMake Makefiles')) {
            throw "The standalone build directory uses $generator. Use a separate directory for manual CMake builds."
        }
    }
    if ($generator -eq 'Ninja' -and -not $ninja) { throw 'The existing standalone build requires Ninja. Install C++ CMake tools for Windows.' }

    $configure = @('-S', $sourceRoot, '-B', $buildRoot, '-G', $generator,
        '-DCMAKE_BUILD_TYPE=Release', '-DDD_STANDALONE=ON', '-DBUILD_TESTING=OFF')
    if ($generator -eq 'Ninja') { $configure += "-DCMAKE_MAKE_PROGRAM=$ninja" }
    Write-Host 'Building the standalone executable with embedded assets and static libraries.'
    Write-Host 'The first build downloads and compiles SFML and its audio/font dependencies.'
    & $cmake @configure
    if ($LASTEXITCODE -ne 0) { throw 'Standalone configuration failed. Check the first CMake error above.' }
    $build = @('--build', $buildRoot, '--config', 'Release', '--target', 'Dungeon_Descent', '--parallel')
    if ($Rebuild) { $build += '--clean-first' }
    & $cmake @build
    if ($LASTEXITCODE -ne 0) { throw 'Standalone build failed. Check the first compiler or linker error above.' }

    $executable = Join-Path $buildRoot 'Dungeon_Descent.exe'
    if (-not (Test-Path -LiteralPath $executable)) { throw 'The build did not produce Dungeon_Descent.exe.' }
    Write-Host 'Checking the executable alone, embedded assets and cache repair.'
    $verificationRoot = Join-Path $buildRoot 'verify'
    & $cmake "-DGAME=$executable" "-DWORK=$verificationRoot" -P (Join-Path $sourceRoot 'tests\StandaloneSmoke.cmake')
    if ($LASTEXITCODE -ne 0) { throw 'The standalone executable failed its isolated launch check.' }
    New-Item -ItemType Directory -Path (Split-Path -Parent $destination) -Force | Out-Null
    Copy-Item -LiteralPath $executable -Destination $destination -Force
    Write-Host "Ready to share: $destination"
} catch {
    Write-Error $_ -ErrorAction Continue
    exit 1
}
