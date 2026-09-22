#Requires -Version 5.1
<#
pyCircuit 6 Windows build wrapper.

PowerShell counterpart of flows/scripts/pyc plus the toolchain discovery in
flows/scripts/lib.sh. It provides the `build` subcommand with the same
semantics as the bash wrapper:

  * configure the repository root with the Ninja generator, Release build,
    LLVM_DIR/MLIR_DIR, and the PYC_BUILD_* options;
  * build pycc and the pyc6 runtime (and everything when the Agentic Circuit
    build is enabled);
  * install into <root>/.pycircuit_out/toolchain/install and report the
    installed pycc.exe path.

Environment parity with the bash wrapper:
  LLVM_DIR, MLIR_DIR         Optional explicit LLVM/MLIR CMake package dirs.
  LLVM_ROOT                  Optional LLVM install root (Windows archive
                             layout: <root>/lib/cmake/{llvm,mlir}).
  LLVM_CONFIG                Optional explicit llvm-config executable.
  PYC_BUILD_DIR              Optional build directory override.
  PYC_INSTALL_PREFIX         Optional install prefix override.
  PYC_BUILD_AGENTIC_CIRCUIT_TESTS  Build integrated tests (OFF).
  PYC_PYTHON_EXECUTABLE      Optional exact Python interpreter for the SDK.
  PYCC                       Path to pycc (used by unsupported subcommands).

The bash wrapper supports `build` and `smoke`. Only `build` has a supported
Windows implementation here; every other subcommand fails explicitly instead
of pretending to work.
#>

[CmdletBinding()]
param(
    [Parameter(Position = 0)]
    [string]$Command = "help",

    [Parameter(Position = 1, ValueFromRemainingArguments = $true)]
    [string[]]$Arguments = @()
)

Set-StrictMode -Version 2.0
$ErrorActionPreference = "Stop"

$script:PycircuitRequiredLlvmMajor = 22
$script:ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$script:RootDir = (Resolve-Path (Join-Path $script:ScriptDir "..\..")).Path

function Write-PycLog {
    param([string]$Message)
    Write-Host "[pyc] $Message"
}

function Write-PycWarn {
    param([string]$Message)
    Write-Host "[pyc][warn] $Message" -ForegroundColor Yellow
}

function Exit-PycDie {
    param([string]$Message)
    Write-Host "[pyc][error] $Message" -ForegroundColor Red
    exit 1
}

function Get-PycUsage {
    @"
Usage: flows/scripts/pyc.ps1 <command>

Commands:
  build        Configure+build+install pycc/pyc-opt into a staged toolchain

Env:
  LLVM_DIR, MLIR_DIR        Optional. If unset, they are inferred from
                            LLVM_CONFIG, LLVM_ROOT, or llvm-config.exe.
  LLVM_ROOT                 Optional LLVM install root on Windows.
  LLVM_CONFIG               Optional explicit llvm-config executable.
  PYC_BUILD_DIR             Optional build directory override.
  PYC_INSTALL_PREFIX        Optional install prefix override.
  PYC_BUILD_AGENTIC_CIRCUIT_TESTS Build integrated ACIR tests (OFF).
  PYC_PYTHON_EXECUTABLE     Optional exact Python interpreter for the SDK.
  PYCC                      Path to pycc (overrides auto-detect).

Unsupported on Windows:
  smoke                     The bash-only simulation and example flows are not
                            ported; smoke fails with an explicit message.
"@
}

function Get-EnvironmentValue {
    param([string]$Name)
    $value = [Environment]::GetEnvironmentVariable($Name)
    if ([string]::IsNullOrWhiteSpace($value)) {
        return $null
    }
    return $value.Trim()
}

function Get-EnvironmentValueOrDefault {
    param(
        [string]$Name,
        [string]$Default
    )
    $value = Get-EnvironmentValue $Name
    if ($null -eq $value) {
        return $Default
    }
    return $value
}

function Get-ConfiguredPath {
    param([string]$Path)
    if ([string]::IsNullOrWhiteSpace($Path)) {
        return $null
    }
    $expanded = [Environment]::ExpandEnvironmentVariables($Path)
    if ([System.IO.Path]::IsPathRooted($expanded)) {
        return [System.IO.Path]::GetFullPath($expanded)
    }
    return [System.IO.Path]::GetFullPath((Join-Path (Get-Location).Path $expanded))
}

function Quote-NativeValue {
    param([string]$Value)
    # The wrapper runs native tools through a single command string so that
    # values containing whitespace survive both Windows PowerShell 5.1 (which
    # concatenates array elements without quoting) and PowerShell 7.
    return '"' + $Value + '"'
}

function Get-LlvmConfigPath {
    $configured = Get-EnvironmentValue "LLVM_CONFIG"
    if ($configured -and (Test-Path -LiteralPath $configured -PathType Leaf)) {
        return (Get-ConfiguredPath $configured)
    }

    $command = Get-Command "llvm-config.exe" -ErrorAction SilentlyContinue
    if ($null -eq $command) {
        $command = Get-Command "llvm-config" -ErrorAction SilentlyContinue
    }
    if ($null -ne $command) {
        return $command.Source
    }

    # Candidate locations are only meaningful on Windows; guard them so the
    # script stays loadable on hosts without the Windows environment.
    $programFiles = [Environment]::GetEnvironmentVariable("ProgramFiles")
    $candidates = @("C:\LLVM\bin\llvm-config.exe")
    if (-not [string]::IsNullOrWhiteSpace($programFiles)) {
        $candidates = @((Join-Path $programFiles "LLVM\bin\llvm-config.exe")) + $candidates
    }
    foreach ($candidate in $candidates) {
        if (Test-Path -LiteralPath $candidate -PathType Leaf) {
            return $candidate
        }
    }
    return $null
}

function Get-LlvmMajorVersion {
    param([string]$LlvmConfig)
    $reported = & $LlvmConfig --version 2>$null
    if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($reported)) {
        return $null
    }
    return ($reported.Trim() -split "\.")[0]
}

function Resolve-LlvmDirectories {
    param([string]$LlvmConfig)

    $llvmDir = Get-EnvironmentValue "LLVM_DIR"
    $mlirDir = Get-EnvironmentValue "MLIR_DIR"
    if ($llvmDir -and $mlirDir) {
        return @{
            LlvmDir    = (Get-ConfiguredPath $llvmDir)
            MlirDir    = (Get-ConfiguredPath $mlirDir)
            LlvmConfig = $LlvmConfig
        }
    }

    $root = Get-EnvironmentValue "LLVM_ROOT"
    if ($root) {
        $resolvedRoot = Get-ConfiguredPath $root
        $rootLlvm = Join-Path $resolvedRoot "lib\cmake\llvm"
        $rootMlir = Join-Path $resolvedRoot "lib\cmake\mlir"
        if ((Test-Path -LiteralPath $rootLlvm -PathType Container) -and
            (Test-Path -LiteralPath $rootMlir -PathType Container)) {
            return @{
                LlvmDir    = $rootLlvm
                MlirDir    = $rootMlir
                LlvmConfig = $LlvmConfig
            }
        }
        Exit-PycDie "LLVM_ROOT=$resolvedRoot does not contain lib\cmake\llvm and lib\cmake\mlir"
    }

    if (-not $LlvmConfig) {
        $LlvmConfig = Get-LlvmConfigPath
    }
    if ($LlvmConfig) {
        $major = Get-LlvmMajorVersion $LlvmConfig
        if ($major -ne "$script:PycircuitRequiredLlvmMajor") {
            $reported = if ($major) { $major } else { "unknown" }
            Exit-PycDie "LLVM $script:PycircuitRequiredLlvmMajor is required, but $LlvmConfig reports version $reported"
        }
        Write-PycLog "inferring LLVM_DIR/MLIR_DIR via $LlvmConfig"
        $cmakeDir = (& $LlvmConfig --cmakedir 2>$null | Select-Object -First 1)
        if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($cmakeDir)) {
            Exit-PycDie "$LlvmConfig --cmakedir did not report an LLVM CMake package directory"
        }
        $resolvedLlvmDir = Get-ConfiguredPath $cmakeDir.Trim()
        $resolvedMlirDir = Join-Path (Split-Path -Parent $resolvedLlvmDir) "mlir"
        if (-not (Test-Path -LiteralPath $resolvedLlvmDir -PathType Container)) {
            Exit-PycDie "inferred LLVM_DIR does not exist: $resolvedLlvmDir"
        }
        if (-not (Test-Path -LiteralPath $resolvedMlirDir -PathType Container)) {
            Exit-PycDie "inferred MLIR_DIR does not exist: $resolvedMlirDir"
        }
        return @{
            LlvmDir    = $resolvedLlvmDir
            MlirDir    = $resolvedMlirDir
            LlvmConfig = $LlvmConfig
        }
    }

    Exit-PycDie "set LLVM_DIR and MLIR_DIR (or provide LLVM_ROOT / llvm-config.exe, or pass --llvm-config) before building"
}

function Invoke-BuildCommand {
    param([string[]]$BuildArguments)

    $buildDir = $null
    $installPrefix = $null
    $llvmConfig = $null

    $envBuildDir = Get-EnvironmentValue "PYC_BUILD_DIR"
    if ($envBuildDir) { $buildDir = Get-ConfiguredPath $envBuildDir }
    $envInstallPrefix = Get-EnvironmentValue "PYC_INSTALL_PREFIX"
    if ($envInstallPrefix) { $installPrefix = Get-ConfiguredPath $envInstallPrefix }

    $index = 0
    while ($index -lt $BuildArguments.Count) {
        $argument = $BuildArguments[$index]
        switch ($argument) {
            "--build-dir" {
                if ($index + 1 -ge $BuildArguments.Count) {
                    Exit-PycDie "missing value for --build-dir"
                }
                $index++
                $buildDir = Get-ConfiguredPath $BuildArguments[$index]
            }
            "--install-prefix" {
                if ($index + 1 -ge $BuildArguments.Count) {
                    Exit-PycDie "missing value for --install-prefix"
                }
                $index++
                $installPrefix = Get-ConfiguredPath $BuildArguments[$index]
            }
            "--llvm-config" {
                if ($index + 1 -ge $BuildArguments.Count) {
                    Exit-PycDie "missing value for --llvm-config"
                }
                $index++
                $llvmConfig = Get-ConfiguredPath $BuildArguments[$index]
                if (-not (Test-Path -LiteralPath $llvmConfig -PathType Leaf)) {
                    Exit-PycDie "--llvm-config is not an existing file: $llvmConfig"
                }
            }
            default {
                Exit-PycDie "unknown build option: $argument"
            }
        }
        $index++
    }

    if (-not $buildDir) {
        $buildDir = Join-Path $script:RootDir ".pycircuit_out\toolchain\build"
    }
    if (-not $installPrefix) {
        $installPrefix = Join-Path $script:RootDir ".pycircuit_out\toolchain\install"
    }

    New-Item -ItemType Directory -Force -Path $buildDir | Out-Null
    New-Item -ItemType Directory -Force -Path $installPrefix | Out-Null

    $llvmDirectories = Resolve-LlvmDirectories -LlvmConfig $llvmConfig

    if (-not (Get-Command "cmake" -ErrorAction SilentlyContinue)) {
        Exit-PycDie "cmake is required (install CMake and ensure it is on PATH)"
    }
    if (-not (Get-Command "ninja" -ErrorAction SilentlyContinue)) {
        Exit-PycDie "ninja is required (install Ninja and ensure it is on PATH)"
    }

    $agenticCircuitTests = Get-EnvironmentValueOrDefault "PYC_BUILD_AGENTIC_CIRCUIT_TESTS" "OFF"

    Write-PycLog "configure ($buildDir)"
    $cmakeCommand = "cmake -G Ninja -S " + (Quote-NativeValue $script:RootDir) +
        " -B " + (Quote-NativeValue $buildDir) +
        " -DCMAKE_BUILD_TYPE=Release" +
        " -DCMAKE_INSTALL_PREFIX=" + (Quote-NativeValue $installPrefix) +
        " -DLLVM_DIR=" + (Quote-NativeValue $llvmDirectories.LlvmDir) +
        " -DMLIR_DIR=" + (Quote-NativeValue $llvmDirectories.MlirDir) +
        " -DPYC_BUILD_AGENTIC_CIRCUIT_TESTS=" + (Quote-NativeValue $agenticCircuitTests)
    $pythonExecutable = Get-EnvironmentValue "PYC_PYTHON_EXECUTABLE"
    if ($pythonExecutable) {
        $cmakeCommand += " -DPython3_EXECUTABLE=" + (Quote-NativeValue $pythonExecutable)
    }

    Invoke-Expression $cmakeCommand
    if ($LASTEXITCODE -ne 0) {
        Exit-PycDie "cmake configure failed"
    }

    Write-PycLog "build pycc + runtime ($buildDir)"
    Invoke-Expression ("ninja -C " + (Quote-NativeValue $buildDir) + " pycc pyc6_runtime")
    if ($LASTEXITCODE -ne 0) {
        Exit-PycDie "ninja failed for pycc and pyc6_runtime"
    }

    Write-PycLog "build integrated ACIR/ACC/gfsim toolchain"
    Invoke-Expression ("ninja -C " + (Quote-NativeValue $buildDir) + " all")
    if ($LASTEXITCODE -ne 0) {
        Exit-PycDie "ninja failed for the integrated toolchain"
    }

    # pyc-opt is optional in the bash wrapper and may be declared only on some
    # configurations; do not fail the build when the target is absent.
    Invoke-Expression ("ninja -C " + (Quote-NativeValue $buildDir) + " pyc-opt 2>`$null")
    if ($LASTEXITCODE -ne 0) {
        Write-PycWarn "pyc-opt target was not built"
    }

    Invoke-Expression ("cmake --install " + (Quote-NativeValue $buildDir) + " --prefix " + (Quote-NativeValue $installPrefix))
    if ($LASTEXITCODE -ne 0) {
        Exit-PycDie "cmake --install failed"
    }

    $env:PYC_TOOLCHAIN_ROOT = $installPrefix
    $env:PYCC = Join-Path $installPrefix "bin\pycc.exe"
    Write-PycLog "ok: $($env:PYCC)"
    Write-PycLog "PYC_TOOLCHAIN_ROOT=$installPrefix"
}

switch ($Command) {
    "build" {
        Invoke-BuildCommand -BuildArguments $Arguments
    }
    "smoke" {
        Exit-PycDie "smoke is not supported on Windows: flows/scripts/run_examples.sh and run_sims.sh are bash-only flows. Run them on the Linux or macOS lane."
    }
    { $_ -in @("-h", "--help", "help") } {
        Get-PycUsage | Write-Host
    }
    default {
        Exit-PycDie "unknown command: $Command (run: flows/scripts/pyc.ps1 help)"
    }
}
