param (
    [switch]$run = $false
)

if (-not (Test-Path build_clang)) {
    New-Item -ItemType directory -Path build_clang
}
Set-Location -Path build_clang;
$cmake_status = $(cmake -TLLVM-vs2014 -G"Visual Studio 15 2017" -DDEBUG_PPU=ON -DNESTEST=OFF .. | Out-Host;$?;)
$build_status = $(MSBuild.exe .\anese.sln | Out-Host;$?;)
if ($cmake_status -and $build_status -and $run) {
    invoke-expression 'cmd /c start powershell -Command { .\Debug\anese.exe "..\roms\retail\Donkey Kong (GC).nes" }'
}
Set-Location -Path ..;
