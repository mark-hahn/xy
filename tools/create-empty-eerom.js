
// create file with empty eerom values
// run:  node tools/create-empty-eerom.js

/* eeprom map
  2  Magic = 0xedde
  33 AP password
	4 STA choices (71 each)
		 33 ssid
		 33 pwd
		 4  static ip
		 1  min quality
	319 bytes total
*/

buf = Buffer.alloc(319);
buf[0] = 0xed; // magic value
buf[1] = 0xde;
buf.write("eridien", 2, "utf-8"); // AP pwd

require("fs").writeFileSync("data/empty-eerom.dat", buf);
