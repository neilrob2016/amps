section init
	var val
	val = sin(60)
	print "Sin = ",val
	print "Arcsin = ",arcsin(val)
	val = cos(60)
	print "Cos = ",val
	print "Arccos = ",arccos(val)
	val = tan(60)
	print "Tan = ",val
	print "Arctan = ",arctan(val)
	# Cause out of range error
	print arccos(-2)
