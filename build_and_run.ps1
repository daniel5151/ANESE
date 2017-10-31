Set-Location -Path build;
$cmake_status = $(cmake .. | Out-Host;$?;)
$build_status = $(MSBuild.exe .\anese.sln | Out-Host;$?;)
if ($cmake_status -and $build_status) {
    invoke-expression 'cmd /c start powershell -Command { .\Debug\anese.exe ..\roms\tests\cpu\nestest\nestest.nes }'
}
Set-Location -Path ..;