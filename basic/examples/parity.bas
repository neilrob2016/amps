#
# Calculate odd parity for values. Note that bitwise ops treat all numbers
# as 32 bit only.
#
section main
	var val
	var shift
	var parity
	var i

@values
	data 1,3,5,8,0xF,0x1F,0x3F,0xFF,0
	restore "values"

	# The parity calculation uses a lot of cycles
	yield 100

	read val
	while val
		gosub "parity"
		print "Odd parity of ",val," = ",parity
		read val
	wend
	exit

@parity
	parity = 0
	for shift = 0 to 31
		parity = parity ^ ((val & (1 << shift)) >> shift)
	next shift
	return
