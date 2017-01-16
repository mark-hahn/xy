
set -e

cd ../../app
npm run build
rm -rf             ../arduino/xy/data/*
cp favicon.ico     ../arduino/xy/data
cp index.html      ../arduino/xy/data

mkdir -p           ../arduino/xy/data/dist
cp -a dist/*       ../arduino/xy/data/dist
gzip               ../arduino/xy/data/dist/*

mkdir -p           ../arduino/xy/data/src/assets
cp -a src/assets/* ../arduino/xy/data/src/assets

cd ../arduino/xy
