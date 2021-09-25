#
# Loops through randomised patches every few seconds and lets user
# save them with F1 key
#
section init
	var patch_num = 1
	var patch
	var saved
	reset
	mesg "Press F1 to save patch"
	set("SYS:NOTE","4C")


section funckey
	if $function_key = 1 and not saved then
		patch = "random_" + intstr(patch_num)	
		mesg "Saving patch: ",patch
		if not save(patch) then
			mesg "ERROR: Patch save failed"
		else
			patch_num = patch_num + 1
			saved = 1
		fi
	else if $function_key = 2 then
		mesg "Program exiting..."
		exit program
	fiall


section timer 3
	saved = 0
	mesg "New patch..."
	set("RANDOMISE",1)
	set("SYS:PLAY",1)
