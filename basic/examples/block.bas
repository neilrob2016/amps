#
# Shows the effect of blocking and unblocking sections
#
section play
	print "play: ",$note

section release
	print "release: ",$note

section main
	var i
	sleep 1
	print "START MAIN"
	block
		for i = 1 to 5
			print "MAIN: i = ",i
			sleep 0.2
		next i
	unblock
	print "STOP MAIN"
	sleep 2
	print "RESTARTING"
	restart


section main1
	var j
	print "START MAIN1"
	for j = 1 to 20
		print "MAIN1: j = ",j
		sleep 0.1
	next j
