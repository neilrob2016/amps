#
# Linear feedback shift registers - pseudo random number generators.
#
# A demonstration of the bitwise operators and hex values.
#
section main
	var reg = 0xACE1
	var bit

	yield 1000
	print "Fibonacci LFSR:"
	sleep 1
	do
		bit = reg ^ (reg >> 2) ^ (reg >> 3) ^ (reg >> 5)
		reg = ((reg >> 1) | (bit << 15)) & 0xFFFF
		gosub "print"
	until reg = 0xACE1

	print 
	print "Galois LFSR:"
	sleep 1

	reg = 0xACE1
	do
		reg = (reg >> 1) ^ (-(reg & 1) & 0xB400)
		gosub "print"
	until reg = 0xACE1
	exit

@print
	print intstr(reg)," (0x",hexstr(reg),", 0b",binstr(reg),")"
	return
