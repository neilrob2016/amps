#
# Store all the frequencies of all possible notes
#
section init
	var note_dict
	var note
	var scale
	var name
	var freq
	var i

	# C scale will give us the white keys
	scale = getscalebyname("C")
	loop 2
		for note = 0 to $num_notes - 1
			name = getnotename(note,scale)
			freq = getnotefreq(note,scale)
			note_dict[name] = freq
		next note
		# B scale will give us the black keys
		scale = getscalebyname("B")
	lend

	for i = 0 to keycnt(note_dict) - 1
		name = getkey(note_dict,i)
		print "Note ",name," = ",tointstr(note_dict[name])," Hz"
	next i
