set -e

cd ~/dev/p2/app
rm scripts/*.gz
cp index.html scripts
au build
rm scripts/*.map
gzip -r scripts
cp scripts/* ~/dev/p2/arduino/xy/data

cd ~/dev/p2/arduino/xy
pio run -t buildfs
echo "******** AU build done *********"

curl http://192.168.1.235/upload --data-binary -d data/eridien-logo.jpg
