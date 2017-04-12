
fs = require("fs");

fs.appendFileSync("invtable1.asm", "\
PSECT invtblsect,class=CODE,local,delta=2\n\
GLOBAL _invtable1\n\
_invtable1:\n\
");

for(pps = 64; pps < 96; pps+=(1/64)) {
  fs.appendFileSync( "invtable1.asm",
    "DW 0x" + Math.min(0x3fff, Math.round(1e6/pps)).toString(16) + '  ; 1e6/' + pps + '\n'
  );
}
