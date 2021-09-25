#
# Recursion demonstration
#
section main
	print "*** Start ***"
	var i
@rec
	i = i + 1
	if i < 100 then
		print "Gosub: ",intstr(i)
		gosub "rec"
	fi
	i = i - 1
	if i > 0 then
		print "Return: ",intstr(i)
		return
	fi
	print "*** End ***"
