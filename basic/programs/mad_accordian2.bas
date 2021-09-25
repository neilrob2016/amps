# Vary ring mod params depending on note being played and filter value.
# Update internal values using function keys. May 2016
section init
	reset
	set("VOL",150)
	set("SQUARE",1)
	set("SB1","SQUARE")
	set("SB2","SAWTOOTH")
	set("CH","1-3-5")
	set("DE",20)
	set("EL",5)

	set("RI","RANGE 1")
	set("RM","SINE")
	set("IF",1)
	set("IL",255)

	var ang0
	var ang1
	var val
	var frq
	var mult = 1
	var add = 255
	var note_div = 2
	var filter_div = 20


section funckey
	if $function_key = 1 then
		if note_div > 1 then
			note_div = note_div - 1
			mesg "Note div = ",tointstr(note_div)
		fi
	else if $function_key = 2 then
		note_div = note_div + 1
		mesg "Note div = ",tointstr(note_div)
	else if $function_key = 3 then
		if filter_div > 1 then
			filter_div = filter_div - 1
			mesg "Filter div = ",tointstr(filter_div)
		fi
	else if $function_key = 4 then
		filter_div = filter_div + 1
		mesg "Filter div = ",tointstr(filter_div)
	fiall
	gosub "update_mult"
	gosub "update_add"
		

section play
	gosub "update_mult"


section filter
	gosub "update_add"


section timer0 0.02
	set("IF",abs(sin(ang0) * mult) + 3)
	ang0 = (ang0 + add) % 360


section timer1 0.1
	val = abs(sin(ang1) * 5)
	set("SO1",val)
	set("SO2",-val)
	ang1 = (ang1 + 10) % 360
	# So we don't fall through into subroutines
	yield


#
# Subroutines
#
@update_mult
	# If note not available use frequency
	if $note = -1 then
		if $freq < 32 then
			frq = 32
		else
			frq = $freq
		fi
		mult = (log2(frq) - 5) * 3.5 + 1
	else
		mult = $note / note_div + 1
	fi
	return


@update_add
	add = (255 - $filter) / filter_div
	if add < 0 then 	
		add = 0.1
	fi
	return
