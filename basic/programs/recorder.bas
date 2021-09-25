#
# Note recorder. Play some notes then press F1 and it'll replay them, press
# F2 and it'll replay them in a loop. When you play some new notes it'll clear 
# the old notes and record these.
#
section init
	var RELEASE = 666
	var notes
	var play
	var played

	reset
	set("SAWTOOTH",1)
	set("SYS:PLAY",0)
	yield 50  # Better for timing


section play
	# If we're playing then delete the stored notes
	if played then
		play = 0
		played = 0
		clear notes
		mesg "New Tune!"
	fi
	notes[tostr($time)] = $note


section release
	notes[tostr($time)] = RELEASE


#
# To play the notes press F1
#
section funckey
	if $function_key < 3 then
		mesg "Playing!"
		set("SYS:PLAY",1)
		play = $function_key
	fi


#
# Plays notes once F1 or F2 is pressed 
#
section main
	var i
	var tm
	var next_tm
	var n

	while 1
		if not play or keycnt(notes) < 2 then continue fi

		played = 1

		for i = 0 to keycnt(notes) - 2
			if not play then break fi

			# Get timing key and set the note. Need to protect
			# againt play section deleting notes so block other
			# sections until we're done
			block
				if play then 
					tm = getkey(notes,i)
					next_tm = tonum(getkey(notes,i+1))
					n = notes[tm]
					tm = tonum(tm)
					#print intstr(i),": Setting note to ",intstr(n)
				fi
			unblock
			if n = RELEASE then
				set("SYS:PLAY",0)
			else
				set("SYS:PLAY",1)
				set("SYS:NOTE",n)
			fi

			#print "Sleeping ",next_tm - tm
			sleep next_tm - tm
		next i

		# 1 means just play the once, 2 = loop until new notes
		if play = 1 then
			play = 0
			set("SYS:PLAY",0)
		fi
	wend

