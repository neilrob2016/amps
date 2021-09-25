#
# Uses the function keys to load patches quickly
#

# Set up patches to load
section init
	var pat
	var curr_pat = ""
	var num_patches 
	var patch[13]
@patches
	data "accordion","bassphase","bells","ethereal_bells","laugh",
	data "numan","organ1","organ2","phaser","wall-e","spooky","END"
	restore "patches"

	reset
	print 
	print "*** PATCH LOADER ***"
	print
	read pat
	while pat <> "END" and num_patches < 13
		num_patches = num_patches + 1
		patch[num_patches] = pat
		print "F",intstr(num_patches)," = '",patch[num_patches],"'"
		read pat
	wend


# This event section is called when a function key is pressed
section funckey
	if $function_key <= num_patches then
		pat = "patches/" + patch[$function_key]
		if pat <> curr_pat then
			mesg "Loading patch '",pat,"'..."
			if not load(pat) then
				mesg "*** Patch load failed ***"
			fi
			curr_pat = pat
		fi
	else
		mesg "No patch assigned to key F",intstr($function_key)
	fi
