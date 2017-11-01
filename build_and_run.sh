mkdir build_linux;
cd build_linux;
cmake .. -DNESTEST=OFF;
make;
cppcheck \
  --enable=unusedFunction,warning,style,performance,portability \
  --std=posix \
  -I ../src \
  ../src \
  2> cppcheck && cat cppcheck | sed -e "/\(Checking\|files checked\)/d" | sort \
  & # run this in the background
./anese ../roms/tests/cpu/nestest/nestest.nes;
