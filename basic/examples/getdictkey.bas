#
# Get keys by position
#
section init
	var key
	var value
	var a
	var b = 456
	var i
@d
	data "hello",123,
	data "cruel","world",
	data "out",b,
	data "there","wibble"
	restore "d"

	for i = 0 to 3
		read key,value
		print "Key = ",key,", value = ",value
		a[key] = value
	next i

	print "-----"
	for i = 0 to keycnt(a) - 1
		key = getkey(a,i)
		print "Key = ",key,", value = ",a[key]
	next i
		

