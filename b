
set -e

cd ~/dev/p2/app
rm -rf scripts/*
au build
#rm scripts/*.map
gzip -r scripts
cp index.html ~/dev/p2/arduino/xy/data
cp -a scripts/* ~/dev/p2/arduino/xy/data/scripts

cd ~/dev/p2/arduino/xy
pio run -t buildfs

curl -F 'filename=/index.html'                    \
     -F 'file=@data/index.html'                    http://192.168.1.235/upload

curl -F 'filename=scripts/app-bundle.js.gz'      \
     -F 'file=@data/scripts/app-bundle.js.gz'      http://192.168.1.235/upload

curl -F 'filename=scripts/app-bundle.js.map.gz'  \
     -F 'file=@data/scripts/app-bundle.js.map.gz'  http://192.168.1.235/upload

curl -F 'filename=scripts/vendor-bundle.js.gz'   \
     -F 'file=@data/scripts/vendor-bundle.js.gz'   http://192.168.1.235/upload
