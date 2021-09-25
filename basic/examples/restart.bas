section init
	print "Restart num = ",intstr($restart_num)
	set("SYS:MAIN_OSC",1 + ($restart_num % ($num_snd_types - 2)))

section main
	sleep 2
	restart
