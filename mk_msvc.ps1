param (
    [switch]$run = $false
)

if (-not (Test-Path build_msvc)) {
    New-Item -ItemType directory -Path build_msvc
}
Set-Location -Path build_msvc;
$cmake_status = $(cmake -DDEBUG_PPU=ON -DNESTEST=OFF .. | Out-Host;$?;)
$build_status = $(MSBuild.exe .\anese.sln | Out-Host;$?;)
if ($cmake_status -and $build_status -and $run) {
    invoke-expression 'cmd /c start powershell -Command { .\Debug\anese.exe "..\roms\retail\Donkey Kong (GC).nes" }'
}
Set-Location -Path ..;
