#
# Demonstrates key-value pair dictionary functionality
#
section init
	var gui

section dial
	print "Dial ",$dial," set to ",$dial_value
	if haskey(gui,$dial) then
		print "Dial pressed before. Previous value = ",gui[$dial], \
			", diff = ",$dial_value - gui[$dial]
	else
		print "New dial turned"
	fi
	gui[$dial] = $dial_value



section button
	print "Button ",$button," set to ",$button_value, \
		" (",$button_value_name,")"

	# Test key check
	if haskey(gui,$button) then
		print "Button pressed before. Previous value = ",gui[$button],\
			 " diff = ",$button_value - gui[$button]
	else
		print "New button ",$button," pressed"
	fi

	# Test key deletion. Forget resonance pressed if we press phaser
	if $button = "PH" and delkey(gui,"RE") then
		print "*** Resonance deleted! ***"
	fi
		
	gui[$button] = $button_value
