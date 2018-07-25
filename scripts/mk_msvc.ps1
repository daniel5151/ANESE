param (
    [switch]$run = $false,
    [string]$rom = ""
)

if (-not (Test-Path build_msvc)) {
    New-Item -ItemType directory -Path build_msvc
}
Set-Location -Path build_msvc;
$cmake_status = $(cmake -DDEBUG_PPU=ON -DNESTEST=OFF .. | Out-Host;$?;)
$build_status = $(MSBuild.exe /p:Configuration=Release .\anese.sln | Out-Host;$?;)
if ($cmake_status -and $build_status -and $run) {
    if ($rom -eq "") {
        invoke-expression "cmd /c start powershell -Command { .\Release\anese.exe --widenes }"
    } else {
        invoke-expression "cmd /c start powershell -Command { .\Release\anese.exe --widenes '..\$rom' }"
    }
}
Set-Location -Path ..;
