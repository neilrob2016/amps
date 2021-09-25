# Check scale and note
section init
	var s,n
	set("SYS:PLAY","ON")
	set("SAWTOOTH","ON")

section scale
	print "SECTION SCALE: Scale = ",$scale_name," (",$scale,")"

section play
	print "SECTION PLAY: Note = ",$note,", name = ",$note_name

section timer 1
	print "Get scale = ",$scale_name," (",$scale,")"
	print "Get note = ",$note,", (",$note_name,")"
	set("SYS:SCALE",$scale+1)
	set("SYS:NOTE",$note+1)
