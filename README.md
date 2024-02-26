# xasm16
A specialized 6502 assembler for the Commander X16 computer

Designed by me to fit my basic needs while designing my own X16 multitasking OS (shelved atm as I am a mid programmer), but still a fun exercise in assembler development.

#### Options:
`-o`: signify file for output

`-v`: verbose

`-addr`: specify an addr to start assembling from (will drop basic header if modified) 

Accepts numbers in hex (0x or \$ on the command line) or in decimal with no prefix

`-fix_caps` / `-fc`: make capital letters line up on PETSCII charset
