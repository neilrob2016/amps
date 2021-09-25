# Button/dial get/set test
SECTION INIT
	var gv,sv
	var obj,objval

SECTION BUTTON
	obj = $button
	objval = $button_value
	gosub "set"
	exit

SECTION DIAL
	obj = $dial
	objval = $dial_value
	gosub "set"
	exit

@set
	print
	print "$dial/$button = ",obj,", value = ",objval
	gv = get(obj)
	sv = set(obj,gv)
	print "Get value = ",gv,", set value = ",sv
	print "Get2 value = ",get(obj)
	if gv <> sv then 
		print "ERROR!"
	fi
	return
