# Tests multitasking the main sections
section main
	var cnt,prev_time
	while 1
		print "MAIN cnt = ",cnt,", diff = ",$time - prev_time
		cnt = cnt + 1
		prev_time = $time
		sleep 1
	wend


section main1
	var cnt1,prev_time1
	while 1
		print "   MAIN 1 cnt = ",cnt1,", diff = ",$time - prev_time1
		cnt1 = cnt1 + 1
		prev_time1 = $time
		sleep 0.5
	wend


section main2
	var cnt2,prev_time2
	while 1
		print "      MAIN 2 cnt = ",cnt2,", diff = ",$time - prev_time2
		cnt2 = cnt2 + 1
		prev_time2 = $time
		sleep 0.2
	wend


section main3
	var cnt3,prev_time3
	while 1
		print "         MAIN 3 cnt = ",cnt3,", diff = ",$time - prev_time3
		cnt3 = cnt3 + 1
		prev_time3 = $time
		sleep 0.1
	wend
