# List all the scales and the notes in those scales
section init
	var scale
	var note

	for scale = 0 to $num_scales - 1
		print "Scale: ",getlongscalename(scale), \
		      " (",getshortscalename(scale),",",tointstr(scale),")"
		for note = 0 to $num_notes - 1
			print "   Note ",tointstr(note),": ", \
			      getnotename(note,scale),"  (", \
			      tointstr(getnotefreq(note,scale))," Hz)"
		next note
	next scale
