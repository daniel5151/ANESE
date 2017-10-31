mkdir build;
cd build;
cmake .. -DNESTEST=OFF;
make;
./anese ../roms/tests/cpu/nestest/nestest.nes;
