mkdir build;
cd build;
cmake .. -DNESTEST=ON;
make;
./anese ../roms/tests/cpu/nestest/nestest.nes > out ;
head -n $(wc -l out | sed -e 's/out//g') ../roms/tests/cpu/nestest/nestest.log > in ;
colordiff in out -y --width=200;
