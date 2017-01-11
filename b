
set -e

cd ~/dev/p2/app
rm -rf scripts/*

lessc src/styles.less src/styles.css
time au build
gzip -r scripts
cp index.html     ~/dev/p2/arduino/xy/data
cp images/*       ~/dev/p2/arduino/xy/data/images
cp -a scripts/*   ~/dev/p2/arduino/xy/data/scripts
cp src/styles.css ~/dev/p2/arduino/xy/data/scripts

cd ~/dev/p2/arduino/xy
pio run -t buildfs
