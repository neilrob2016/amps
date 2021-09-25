#
# 2 ways to set ring mode - number or name
#
section init
	set("RI","RANGE 1")
	set("IF",1)
	set("IL",255)
	set("HOLD NOTE",1)
	set("SYS:NOTE","3C")
	set("SYS:FREQ",480)
	set("SYS:PLAY",1)
	var mode

section main
	#exit
@waves
	# By name
	data "SINE","SQUARE","TRIANGLE","SAWTOOTH"
	autorestore "waves"

	while 1
		read mode
		printmesg "Setting ring mode to: ",mode
		set("RM",mode)
		sleep 2
	wend

section main1
	exit
	# By number
	mode = 1
	while 1
		printmesg "Setting ring mode to: ",intstr(mode)
		set("RM",mode)
		mode = (mode + 1) % $num_ring_modes
		sleep 2
	wend
		
	
