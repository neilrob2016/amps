#
# Set ARP by name
#
section main
@seqdata
	data "OFF", "2 NOTES", "3 NOTES UP A", "3 NOTES DOWN A",
        data "3 NOTES UP B", "3 NOTES DOWN B", "3 NOTES UP DOWN A",
        data "3 NOTES UP DOWN B", "4 NOTES UP", "4 NOTES DOWN",
	data "4 NOTES UP DOWN", "4 NOTES UP INTERLEAVED",
        data "4 NOTES DOWN INTERLEAVED", "OCTAVE UP", "OCTAVE DOWN",
        data "OCTAVE UP DOWN"
	var seq
	
	reset
	autorestore "seqdata"
	set("SAWTOOTH",1)
	set("SYS:NOTE","3C")
	set("SYS:PLAY",1)

	while 1
		read seq
		printmesg "ARP = ",seq
		set("AR",seq)
		sleep 2
	wend

