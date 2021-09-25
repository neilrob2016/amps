# Notes playing up and down when the mouse is released
section init
	var note
	var add
	var released = 0
	var max_vol = 255
	var vol_dec = 10
	var vol
	reset

section play
	vol = max_vol
	set("VOL",vol)
	released = 0

section release
	released = 1
	add = 1
	note = $note
	set("SYS:PLAY",1)

section timer 0.02
	if not released then yield fi
	set("SYS:NOTE",note+add)
	if add > 8 then
		released = 0
		set("SYS:PLAY",0)
	else if add > 0 then
		add = -add
	else
		add = -(add - 1)
	fiall
	if vol < vol_dec then
		vol = 0
	else
		vol = vol - vol_dec
	fi
	set("VOL",vol)
