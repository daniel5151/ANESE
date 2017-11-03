set -e # stop the script when anything fails

mkdir build_linux || true
cd build_linux
cmake .. -DNESTEST=ON -DCMAKE_BUILD_TYPE=Debug
make
./anese ../roms/tests/cpu/nestest/nestest.nes > out
head -n $(wc -l out | sed -e 's/out//g') ../roms/tests/cpu/nestest/nestest.log > in
colordiff in out -y --width=200 | grep -e ".*[|<>].*" -C 10
