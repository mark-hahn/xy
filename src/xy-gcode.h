
#ifndef GCODE
#define GCODE

// on error, returns -1 and  line starts with ERROR:
// return of 0 means to try again
// return of 1 means to move on to next line
int8_t execGCodeLine(char *lineIn, char *lineOut);

/*
http://www.reprap.org/wiki/G-code

max 256 chars/line

comment chars: ; * ( % /
N3 T0*57 ; This is a comment
N4 G92 E0*67  ; * is cksum
; So is this
N5 G28*22

M commands execute after buffer empty

M0: Stop or Unconditional stop (time delay)
M1 - Sleep or Conditional stop
M2: Program End  (M30?)
M17: Enable/Power all stepper motors
M18: Disable all stepper motors
M20: List SD card
M23-M24-M32: Select SD file and start
M25: Pause SD print
M26: Set SD position
M27: Report SD print status
M28: Begin write to SD card
M29: Stop writing to SD card
M30: Delete a file on the SD card
M31: Output time since last M109 or SD card start to serial
M33: Get the long name for an SD card file or folder
M85: Set inactivity shutdown timer
M92: Set axis_steps_per_unit
M106: Fan On & speed
M107: Fan Off
M112: Emergency Stop
M114: Get Current Position
M115: Get Firmware Version and Capabilities
M119: Get Endstop Status
M201: Set max printing acceleration
M202: Set max travel acceleration
M203: Set maximum feedrate
M204: Set default acceleration
M206: Offset axes
M220: Set speed factor override percentage
M221: Set extrude factor override percentage
M300: Play beep sound
M350: Set microstepping mode
M400: Wait for current moves to finish (same as G4 P0)
M502: Revert to the default "factory settings."
M999: Restart after being stopped by error


Typically, the following moving commands are buffered: G0-G3 and G28-G32.

G0 - Rapid Motion
G1 - Coordinated Motion
G2 - Arc - Clockwise
G3 - Arc - Counter Clockwise
G28 - Home (X, Y and Z can be seperate)
G29-G32 - Z-Probe

G4 - Dwell  (pause in P: ms, S: secs)
G10: Retract (Tool Offset)
G11: Unretract
G20 - Inches as units
G21 - Millimeters as units
G53 - Set absolute coordinate system
G90 - Absolute Positioning
G91 - Relative Positioning
G92 - Set zero position
G130 - Set stepper current

X absolute position
Y absolute position
Z absolute position
A position (rotary around X)
B position (rotary around Y)
C position (rotary around Z)
U Relative axis parallel to X
V Relative axis parallel to Y
W Relative axis parallel to Z
M code (another action register or Machine code(*)) (otherwise referred to as a Miscellaneous function)
F feed rate
S spindle speed
N line number
R Arc radius or optional word passed to a subprogram/canned cycle
P Dwell time or optional word passed to a subprogram/canned cycle
T Tool selection
I Arc data X axis
J Arc data Y axis.
K Arc data Z axis, or optional word passed to a subprogram/canned cycle
D Cutter diameter/radius offset
H Tool length offset

linuxCNC language
  number  +- ddd . ffff  -- seventeen significant digits !!!!
  within 0.0001 of an integer => integer

Examples

G1 X5 Y-5 Z6 F3300.0 (Move to postion <x,y,z>=<5,-5,6> at speed 3300.0)
G21 (set units to mm)
G90 (set positioning to absolute)
G92 X0 Y0 Z0 (set current position to <x,y,z>=<0,0,0>)

Some older machines, CNC or otherwise, used to move faster if they did not move in a straight line.














*/



#endif
