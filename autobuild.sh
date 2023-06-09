set -e

rm -rf `pwd`/build/*
cd `pwd`/build &&
	cmake .. &&
	make

cd ..
cp -r `pwd`/src/include `pwd`/lib
cp -f `pwd`/build/src/libjhrpc.a `pwd`/lib   #很奇怪，写的cmake无法导向，只能手动复制了

