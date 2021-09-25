# Print out the prime numbers
section main
	var num = 3
	var cnt = 1
	var str
	var ordstr
	var c
	var i

	while 1
		if num <> 3 then
			for i = 3 to sqrt(num)+2 step 2
				if (num % i) = 0 then
					goto "notprime"
				fi
			next i
		fi
		str = intstr(cnt)
		c = substr(str,strlen(str)-1,1)

		if cnt = 11 or cnt = 12 or cnt = 13 then ordstr = "th"
		else if c = "1" then ordstr = "st"
		else if c = "2" then ordstr = "nd"
		else if c = "3" then ordstr = "rd"
		else ordstr = "th"
		fiall

		str = intstr(cnt) + ordstr + " prime: " + intstr(num)
		print str
		mesg str
		sleep 0.5
		cnt = cnt + 1
@notprime
		num = num + 2
	wend
