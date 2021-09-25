#
# Demonstrates use of arrsize() function. Most of the time you'll know the
# size of the array anyway but in some odd occasions it might prove useful.
#
section init
	var size
@mydata
	data 3,8
	data "hello","cruel","world"
	data "some",123,"more","stuff",456,"blah","de","blah"
	restore "mydata"

	read size
	var a[size]
	read size
	var b[size]
	
	# Set up arrays
	var i
	for i = 0 to arrsize(a) - 1
		read a[i]
	next i
	for i = 0 to arrsize(b) - 1
		read b[i]
	next i

	# Print out their data
	for i = 0 to arrsize(a) - 1
		print "a[",intstr(i),"] = ",a[i]
	next i
	print "-----"
	for i = 0 to arrsize(b) - 1
		print "b[",intstr(i),"] = ",b[i]
	next i
