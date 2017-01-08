set -e

cd ~/dev/p2/app
rm -rf scripts/*
au build
rm scripts/*.map
gzip -r scripts
cp index.html ~/dev/p2/arduino/xy/data
cp -a scripts/* ~/dev/p2/arduino/xy/data/scripts

cd ~/dev/p2/arduino/xy
pio run -t buildfs

# curl http://192.168.1.235/upload --data-binary -d data/eridien-logo.jpg
