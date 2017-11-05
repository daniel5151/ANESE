set -e # stop the script when anything fails

mkdir build_linux || true
cd build_linux
cmake .. -DNESTEST=OFF -DCMAKE_BUILD_TYPE=Debug
make
./anese ../roms/tests/cpu/nestest/nestest.nes
