#
# Demonstrates the asc() and chr() functions which deal with ASCII characters
#
section init
@letters
	data "A","B","C","D","X","Y","Z"
	restore "letters"
	var letter,val,i

	#
	# Translate some ascii chars
	#
	for i = 1 to 7
		read letter
		val = asc(letter)
		print "Letter ",letter," = ",intstr(val)
		print "And back = ",chr(val)
	next i

	#
	# Print out the printable ASCII charset 
	#
	val = ""
	for i = 32 to 127
		val = val + chr(i)
	next i
	print val

