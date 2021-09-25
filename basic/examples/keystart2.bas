#
# Set the key start value by note name
#
section main
@notes
	data "0C","1C","2C","3C","4C"
	data "1D","3D","2D","4D"
	autorestore "notes"
	var note

	mesg "Play something using the PC keyboard ..."
	while 1
		read note
		set("SYS:KEY_START_NOTE",note)
		sleep 2
	wend
