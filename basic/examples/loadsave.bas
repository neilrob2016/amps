# Load a patch at startup, let the user change it then resave
section init
	if load("patches/numan") then 
		print "Patch LOADED"
	else
		print "Patch load FAILED"
	fi

section main
	sleep 5
	if save("wibble") then
		print "Patch SAVED"
	else
		print "Patch save FAILED"
	fi
