
set -e

cd ~/dev/p2/app
rm -rf scripts/*
au build
gzip -r scripts
cp index.html ~/dev/p2/arduino/xy/data
cp -a scripts/* ~/dev/p2/arduino/xy/data/scripts

cd ~/dev/p2/arduino/xy
pio run -t buildfs

ud index.html
ud scripts/app-bundle.js.gz
ud scripts/app-bundle.js.map.gz
ud scripts/vendor-bundle.js.gz
