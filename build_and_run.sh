set -e # stop the script when anything fails

mkdir build_linux || true
cd build_linux
cmake .. -DNESTEST=OFF
make
echo "[cppcheck] running in the background"
cppcheck \
  --quiet \
  --enable=unusedFunction,missingInclude,warning,style,performance,portability \
  --suppress=missingIncludeSystem \
  --std=c++11 \
  -I ../src \
  ../src \
  2> cppcheck.txt && sort cppcheck.txt | sed -e "/constructor with 1 argument/d"\
  || true & # run this in the background
./anese ../roms/tests/cpu/nestest/nestest.nes
