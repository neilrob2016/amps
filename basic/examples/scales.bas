#
# Set scales by name
#
section init
@scales
	# Can use short or long names
	data "C","C minor / D#","C#","C# minor / E","D","Dm","D#m",
	data "Em","Fm","F#m","G#","G#m","A","B"
	var scale
	var done

section timer 1
	if not done then
		autorestore "scales"
		done = 1
	fi
	read scale
	print "Scale = ",set("SYS:SCALE",scale)

