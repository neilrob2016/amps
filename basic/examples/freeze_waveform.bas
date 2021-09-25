# Shows switching freeze waveform on and off
section main
	var frz = 0

	set("SYS:MESG",1)

	# Set up an interesting waveform
	set("SAWTOOTH",1)
	set("SB1","SAWTOOTH")
	set("SO1",-1)
	set("SB2","SAWTOOTH")
	set("SO2",1)
	set("CH","1-3-5")
	set("RE",1)
	set("RL",10)
	set("RF",5)

	set("VOL",150)
	set("SYS:PLAY","ON")
	set("HOLD NOTE",1)
	set("SYS:NOTE","2C")

	while 1
		print "Freeze: ",set("SYS:FREEZE",frz)
		sleep 1
		frz = not frz
	wend
