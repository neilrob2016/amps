# Print out frequencies and the nearest note for the given scale
section init
	var scale
	var freq
	var note
	var name
	var notefreq

	for scale = 0 to $num_scales - 1
		print "Scale: ",getlongscalename(scale)

		for freq = 20 to 300 step 5
			note = getnotebyfreq(freq,scale)
			name = getnotename(note,scale)
			notefreq = tointstr(getnotefreq(note,scale))

			print "    Freq = ",tointstr(freq), \
			      " Hz, nearest note = ",name, \
			      ", actual freq = ",notefreq," Hz"
		next freq

	next scale
