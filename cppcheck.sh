echo "[cppcheck] starting"
cppcheck \
  --enable=unusedFunction,missingInclude,warning,style,performance,portability \
  --suppress=missingIncludeSystem \
  --std=c++11 \
  -I ./src \
  ./src \
  2> cppcheck.txt && sort cppcheck.txt | sed -e "/constructor with 1 argument/d"
rm cppcheck.txt
