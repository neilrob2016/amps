# Shows histogram data from spectrum analyser
section main
	var i
	var j
	var freq
	var bar

	reset
	set("SPECTRUM ANALYSER",1)
	set("SAR",500)

	# Because creating the bar string is resource intensive. Also prevents
	# user changing the analyser range while we're accessing $span[]. Sleep
	# will yield control back to main thread.
	yield 0

	while 1
		if get("SPECTRUM ANALYSER") then
			print intstr($span_max_index + 1)," values: "
			for i = 0 to $span_max_index
				bar = ""
				freq = intstr((i+1) * 20)
				for j = 1 to $span[i] / 50000
					bar = bar + "#"
				next j
				print intstr(i)," (",freq,"Hz): ",bar
			next i
		else
			print "Analyser OFF"
		fi
		print "----------"
		
		sleep 0.5
	wend
