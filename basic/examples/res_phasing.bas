#
# Set resonance and phaser buttons by string values
#
section main
@res
	data "OFF","INDEPENDENT FREQ","FOLLOW MAIN FREQ"

@phase
	data "OFF","ADDITIVE FLANGER","SUBTRACTIVE FLANGER",
	data "ADDITIVE DIRTY FLANGER","SUBTRACTIVE DIRTY FLANGER",
	data "ADDITIVE PHASER","SUBTRACTIVE PHASER",

	autorestore "res"
	var val

	set("SYS:MESG","ON")

	loop 10
		read val
		set("RE",val)
		sleep 0.5
	lend

	autorestore "phase"
	loop 20
		read val
		set("PH",val)
		sleep 0.5
	lend

