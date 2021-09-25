# Demonstrate fields in strings.
#
# field() & fieldcnt() allow an unlimited number of seperator characters 
# between fields. 
#
# field1() & fieldcnt1() allows only 1 seperator character between fields
#
section init
	var a
	a["one"] = " hello   cruel world  out   there  "
	a["two"] = "0,1,,,4,5,,7,,,,11,"

	var cnt1 = fieldcnt(a["one"]," ")
	var cnt2 = fieldcnt1(a["two"],",")

	var i
	var j
	for i = 0 to 1
		print
		if i then
			print "FIELD1()" 
			cnt1 = fieldcnt1(a["one"]," ")
			cnt2 = fieldcnt1(a["two"],",")
		else
			print "FIELD()"
			cnt1 = fieldcnt(a["one"]," ")
			cnt2 = fieldcnt(a["two"],",")
		fi
		print "--------"
		print "One cnt = ",cnt1
		print "Two cnt = ",cnt2
		print

		for j = 0 to cnt1-1 
			if i then
				print j," = ",field1(a["one"],j," ")
			else
				print j," = ",field(a["one"],j," ")
			fi
		next j

		print "----"

		for j = 0 to cnt2-1
			if i then
				print j," = ",field1(a["two"],j,",")
			else
				print j," = ",field(a["two"],j,",")
			fi
		next j
	next i
