# Flip 2 effects in the sequence to get various permutations
section init
	var e1
	var e2
	var s1
	var s2
	var seq
	var i
	var pos1 = 0
	var pos2 = 3
	var len = strlen($effects_seq)


section timer 0.2
	seq = $effects_seq
	e1 = substr(seq,pos1,2)
	e2 = substr(seq,pos2,2)

	if not pos1 then
		seq = e2 + "-" + e1 + substr(seq,5,len-5)
	else if not pos2 then
		seq = e1 + substr(seq,2,len-4) + e2
	else
		seq = substr(seq,0,pos1) + e2 + "-" + e1 + \
		      substr(seq,pos2+2,len-pos2-2)
	fiall
	set("SYS:EFFECTS_SEQ",seq)
	print $effects_seq
	pos1 = (pos1 + 3) % (len+1)
	pos2 = (pos2 + 3) % (len+1)
