section init
	var a[10] 
	var b = "hello"
	var c
	c["blah"] = "bih"
	a[1] = 123
	a[2] = "hello"
	a["test"] = "toast"
	clear a,b,c
	print a[1],",",a[2],",",haskey(a,"test")
	print b
	print haskey(c,"blah")
