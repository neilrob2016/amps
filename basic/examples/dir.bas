#
# Set working dir to patches and load a patch
#
section init
	var dir1,dir2
	dir1 = get("SYS:DIR")
	dir2 = set("SYS:DIR","patches")

	print "Before = ",dir1
	print "After = ",dir2
	if dir1 = dir2 then
		print "ERROR: Couldn't change to new directory"
	else
		print "Loading patch..."
		load("numan")
	fi
