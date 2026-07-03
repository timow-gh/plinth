param(
    [string]$ClangFormat = "clang-format",
    [switch]$Check
)

$ErrorActionPreference = "Stop"

$repoRoot = git rev-parse --show-toplevel
if ($LASTEXITCODE -ne 0) {
    throw "format_cpp.ps1 must be run from inside a git repository."
}

Set-Location $repoRoot

$extensions = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
@(
    ".c",
    ".cc",
    ".cpp",
    ".cxx",
    ".h",
    ".hpp",
    ".hxx"
) | ForEach-Object {
    [void]$extensions.Add($_)
}

function Test-IsCMakeFile {
    param([string]$Path)

    $fileName = [System.IO.Path]::GetFileName($Path)
    $extension = [System.IO.Path]::GetExtension($Path)

    return $fileName -ieq "CMakeLists.txt" -or
        $extension -ieq ".cmake" -or
        $extension -ieq ".in" -or
        $fileName.EndsWith(".cmake.in", [System.StringComparison]::OrdinalIgnoreCase)
}

$candidateFiles = git ls-files -- "src/**/*" "test/**/*"
if ($LASTEXITCODE -ne 0) {
    throw "Failed to enumerate tracked files under src and test."
}

$files = @(
    $candidateFiles | Where-Object {
        -not (Test-IsCMakeFile $_) -and $extensions.Contains([System.IO.Path]::GetExtension($_))
    }
)

if ($files.Count -eq 0) {
    Write-Host "No tracked C/C++ files found under src or test."
    exit 0
}

$clangFormatCommand = Get-Command $ClangFormat -ErrorAction Stop
Write-Host "Using clang-format: $($clangFormatCommand.Source)"

if ($Check) {
    $failedFiles = New-Object System.Collections.Generic.List[string]

    foreach ($file in $files) {
        & $ClangFormat --dry-run --Werror $file
        if ($LASTEXITCODE -ne 0) {
            $failedFiles.Add($file)
        }
    }

    if ($failedFiles.Count -ne 0) {
        Write-Host "Formatting check failed for:"
        foreach ($file in $failedFiles) {
            Write-Host "  $file"
        }
        exit 1
    }

    Write-Host "Formatting check passed for $($files.Count) files."
    exit 0
}

foreach ($file in $files) {
    & $ClangFormat -i $file
    if ($LASTEXITCODE -ne 0) {
        throw "clang-format failed for $file."
    }
}

Write-Host "Formatted $($files.Count) files under src and test."
