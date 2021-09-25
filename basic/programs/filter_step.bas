#
# Step the filter. Change the amount using the F1 and F2 keys.
#
section init
	var f
	var fstep = 10


section funckey
	if $function_key = 1 then
		if fstep > 1 then fstep = fstep - 1 fi
	else if $function_key = 2 then
		fstep = fstep + 1
	else exit
	fiall
	printmesg "Filter step = ",intstr(fstep)


section filter
	f = int($filter / fstep) * fstep
	print "Filter ",intstr($filter)," -> ",intstr(set("SYS:FILTER",f))
