# Beating notes will show the average volume going up and down in the bar
# Version 1.13+ only
section main
	set("VOL",255)
	set("VOLUME BAR",1)
	set("HOLD NOTE",1)
	set("SYS:MAIN_OSC","SQUARE")
	set("SB1","SQUARE")
	set("SO1",1)
	set("SYS:NOTE",10)
	set("SYS:PLAY",1)
	print "Number of volume bar elements = ",tointstr($vol_bar_elements)

	while 1
		print "Volume bar top = ",tointstr($vol_bar_top_value),", hold = ",tointstr($vol_bar_hold_value)
		sleep 0.2
	wend
