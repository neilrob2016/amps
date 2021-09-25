#
# Hex, octal & binary number examples
#
section main
	var code1
	var code2
	var hex
	var i

@hexchars
	data "0","1","2","3","4","5","6","7","8","9","A","B","C","D","E","F"
	autorestore "hexchars"

	print "Hex 0x0A = ",0x0A
	print "Hex 0xFF = ",0xFF
	print "Oct 0012 = ",0012
	print "Oct 0777 = ",0777
	sleep 1
	for i = 0 to 32
		print "Dec = ",intstr(i),", oct = ",octstr(i), \
		      ", hex = Ox",hexstr(i),", bin = 0b",binstr(i)
	next i
	sleep 1

	for code1 = asc("A") to asc("F")
		for i = 0 to 0xF
			read code2
			hex = "0x" + chr(code1) + code2
			print "Hex str = ",hex," val = ",tonum(hex)
		next i
	next code1
