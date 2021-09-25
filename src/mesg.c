/*** Runs the Star Wars type messages that appear in the GUI ***/

#include "globals.h"

#define MESG_INTERVAL 0.1

/*** The actual message function. Add the message string to the list of
     messages ***/
void _message(int to_stdout, char *fmt, va_list args)
{
	static double mesg_time = 0;
	struct st_mesg *msg;
	double now;
	int plen;
	int nm;

	msg = &mesg[next_mesg];

	/* If message hasn't yet been cleared then we've wrapped around the
	   ring buffer. Replace the most recent message */
	if (msg->text)
	{
		nm = (next_mesg ? next_mesg : MAX_MESGS) - 1;
		msg = &mesg[nm];
	}
	else nm = -1;

	/* vasprintf allocates the memory and sets the text pointer */
	free(msg->text);
	vasprintf(&msg->text,fmt,args);

	if (!x_mode)
	{
		if (to_stdout) puts(msg->text);
		return;
	}

	msg->len = strlen(msg->text);

	/* If we're replaced a message then do nothing more */
	if (nm != -1) return;

	/* Space messages out in time so they don't overlap */
	now = getUsecTimeAsDouble();
	if (mesg_time + MESG_INTERVAL < now)
		mesg_time = now;
	else
		mesg_time += MESG_INTERVAL;

	msg->mesg_time = mesg_time;
	msg->y = MESG_START_Y;
	msg->col = YELLOW;
	msg->y_scale = 2;
	msg->x_scale = 2;

	/* Make sure full message is always on screen */
	if ((plen = getTextPixelLen(msg->len,2)) > WIN_WIDTH)
		msg->x_scale *= ((double)WIN_WIDTH / plen);

	next_mesg = (next_mesg + 1) % MAX_MESGS;
}




/*** Output to stdout and window message text ***/
void dualMessage(char *fmt, ...)
{
	va_list args;
	va_list pargs;

	/* vprintf seems to corrupt the args so need seperate one */
	va_start(pargs,fmt);
	vprintf(fmt,pargs);
	putchar('\n');
	va_end(pargs);

	/* Call message function */
	va_start(args,fmt);
	_message(0,fmt,args);
	va_end(args);
}




/*** Calling function for _message() ***/
void message(char *fmt, ...)
{
	va_list args;

	/* Set by BASIC */
	if (do_messages) 
	{
		va_start(args,fmt);
		_message(1,fmt,args);
		va_end(args);
	}
}





/*** Update position and colour of messages. Can't do this in draw.c
     since it'll be affected by screen refresh rate ***/
void updateMessages()
{
	double now;
	int i;

	now = getUsecTimeAsDouble();

	for(i=0;i < MAX_MESGS;++i)
	{
		if (mesg[i].text && mesg[i].mesg_time <= now)
		{
			mesg[i].y -= 5;
			mesg[i].x_scale *= 0.995;
			mesg[i].y_scale *= 0.995;
			if ((mesg[i].col += 0.2) >= BLACK)
			{
				free(mesg[i].text);
				mesg[i].text = NULL;
			}
		}
	}
}
