#
# Reverses all the bits in a variable (32 bits) without looping.
#
section main
	var n 
	var i
	var str

	if $arg = "" then
		print "ERROR: You need to provide a number with -arg"
		exit
	fi
	n = tonum($arg)

	yield 0
	print
	str = "BEFORE: 0x" + hexstr(n) + ", 0b"
	gosub "getbinary"
	mesg str
	if $mode = "GUI" then
		print str
	fi

	n = ((n >>  1) & 0x55555555) | ((n <<  1) & 0xaaaaaaaa)
	n = ((n >>  2) & 0x33333333) | ((n <<  2) & 0xcccccccc)
	n = ((n >>  4) & 0x0f0f0f0f) | ((n <<  4) & 0xf0f0f0f0)
	n = ((n >>  8) & 0x00ff00ff) | ((n <<  8) & 0xff00ff00)
	n = ((n >> 16) & 0x0000ffff) | ((n << 16) & 0xffff0000)

	str = "AFTER : 0x" + hexstr(n) + ", 0b"
	gosub "getbinary"

	mesg str
	if $mode = "GUI" then
		print str
	fi
	print
	exit

@getbinary
	for i = 31 to 0 step -1
		if n & (1 << i) then
			str = str + "1"
		else
			str = str + "0"
		fi
	next i
	return
