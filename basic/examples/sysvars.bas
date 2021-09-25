#
# Prints out all non-event system variables
#
section init
	var i
	print "Version = ",$version
	print "Build date = ",$build_date
	print "Time = ",$time
	print "Pi = ",$pi
	print "E = ",$e
	print "Num sounds = ",$num_snd_types
	print "Num chords = ",$num_chords
	print "Num ARP seqs = ",$num_arp_seqs
	print "Num phasing modes = ",$num_phasing_modes
	print "Num resonance modes = ",$num_res_modes
	print "Num freq modes = ",$num_freq_modes
	print "Num ring ranges = ",$num_ring_ranges
	print "Num ring modes = ",$num_ring_modes
	print "Num timers = ",$num_timers
	for i = 0 to $num_timers-1
		print "  Timer ",intstr(i)," interval: ",$timer_interval[i]
	next i
	print "This section = ",$section
	print "$arg = ",$arg
	print "$true = ",$true
	print "$false = ",$false

section timer0 0.1
	rem

section timer1 0.5
	rem
