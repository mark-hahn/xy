
fs = require("fs");

fs.appendFileSync("invtable.asm", "\
PSECT invtblsect,class=CODE,local,delta=2\n\
GLOBAL _invtable\n\
_invtable:\n\
");

for(pps = 2048; pps < 4096; pps++) {
  fs.appendFileSync( "invtable.asm",
    "DW 0x" + Math.min(0x3fff, Math.round(1e6/pps)) .toString(16) + '\n'
  );
}
