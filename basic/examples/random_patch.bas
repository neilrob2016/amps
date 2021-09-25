#
# Create a random patch each time F1 is pressed
#
section init
	set("SYS:MESG",1)

section funckey
	if $function_key = 1 then
		set("RANDOMISE",1)
	fi
