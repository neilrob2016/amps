# Demonstrates some of the get-name and get-by-name functions
section init
	var name
	var i

	for i = 0 to $num_chords - 1
		name = getchordname(i)
		print intstr(i),": Chord '",name,"' = ",intstr(getchordbyname(name))
	next i

	for i = 0 to $num_snd_types - 1
		name = getsoundname(i)
		print intstr(i),": Snd '",name,"' = ",intstr(getsoundbyname(name))
	next i

	for i = 0 to $num_arp_seqs - 1
		name = getarpseqname(i)
		print intstr(i),": ARP seq '",name,"' = ",intstr(getarpseqbyname(name))
	next i

	for i = 0 to $num_res_modes - 1
		name = getresmodename(i)
		print intstr(i),": Res mode '",name,"' = ",intstr(getresmodebyname(name))
	next i

	for i = 0 to $num_phasing_modes - 1
		name = getphasingmodename(i)
		print intstr(i),": Phasing mode '",name,"' = ",intstr(getphasingmodebyname(name))
	next i

	for i = 0 to $num_ring_ranges - 1
		name = getringrangename(i)
		print intstr(i),": Ring range '",name,"' = ",intstr(getringrangebyname(name))
	next i

	for i = 0 to $num_ring_modes - 1
		name = getringmodename(i)
		print intstr(i),": Ring mode '",name,"' = ",intstr(getringmodebyname(name))
	next i
