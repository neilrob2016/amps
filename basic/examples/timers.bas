#
# Shows the use of timer sections and the effect of blocking on them with the 
# catchup flag switched on and off
#
section init
	var t[$num_timers]
	var tmr 
	var gap
	var i
	print "Catchup = ",get("SYS:TIMER_CATCHUP")
	for i = 0 to $num_timers-1
		print "Timer ",intstr(i)," interval = ",$timer_interval[i]
	next i
	exit



@print_timer
	print "  " * tmr,"Timer ",intstr(tmr),": ",$time - t[tmr]
	t[tmr] = $time
	return

section timer0 0.25
	tmr = 0	
	gosub "print_timer"

section timer1 0.5
	tmr = 1	
	gosub "print_timer"

section timer2 0.75
	tmr = 2	
	gosub "print_timer"

section timer3 1
	tmr = 3
	gosub "print_timer"

section timer4 2
	tmr = 4
	gosub "print_timer"



section main
	while 1
		sleep 2
		print "Catchup now = ",intstr(set("SYS:TIMER_CATCHUP",not get("SYS:TIMER_CATCHUP")))
		print "Blocking"
		block
			sleep 3
			print "unblocking"
		unblock
	wend

