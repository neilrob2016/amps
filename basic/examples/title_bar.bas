#
# Demonstrates getting and setting the window title
#
section init
	printmesg "Title = ",get("SYS:TITLE")

section main
	var title = "Long title that scrolls on and on ...   "
	var len = strlen(title)
	var pos = 0
	var new_title 

	loop 100
		new_title = substr(title,pos,len-pos) + substr(title,0,pos)
		printmesg "Title = '",set("SYS:TITLE",new_title),"'"
		pos = pos + 1
		if pos = len then
			pos = 0
		fi
		sleep 0.1
	lend

	# Set back to default
	printmesg "Back to default..."
	set("SYS:TITLE","")
