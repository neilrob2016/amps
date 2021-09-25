/*** Deals with X win mouse input ***/

#include "globals.h"

#define RADIUS (DIAM / 2)

static int send_play_off = 0;


/*** Mouse button pressed ***/
void mousePressed(int mouse_button, short x, short y)
{
	/* Remember first mouse button pressed (from when none were) */
	if (!first_mouse_button) first_mouse_button = mouse_button;

	mouseData(mouse_button,x,y,0);
}




/*** Mouse button released ***/
void mouseReleased(int mouse_button)
{
	if (mouse_button == first_mouse_button)
	{
		first_mouse_button = 0;
		if (evr_state == RECORDER_RECORD)
			evrAddMouseReleaseEvent(mouse_button);

		/* playOff() calls BASIC so we only want to call it if the 
		   release is for a note, not a button */
		if (send_play_off && !params.hold_note)
		{
			playOff(1); 
			send_play_off = 0;
		}
	}
}




/*** Button clicked or mouse moved with button clicked ***/
void mouseData(int mouse_button, short x, short y, char motion)
{
	double wx;
	double wy;

	wx = (double)x / g_x_scale;
	wy = (double)y / g_y_scale;

	if (evr_state == RECORDER_RECORD)
	{
		if (motion)
			evrAddMotionEvent(mouse_button,x,y);
		else
			evrAddMousePressEvent(mouse_button,x,y);
	}

	if (buttonPressed(mouse_button,motion,wx,wy)) return;

	/* If wheel scrolled and mouse is above filter cutoff line then 
	   cycle through the scales */
	if (y <= g_y_scale * FILTER_Y &&
	    mouse_button > RIGHT_MBUTTON 
	    && !motion && shm->freq_mode != FREQ_CONT)
	{
		cycleScale(mouse_button);
		return;
	}
	if (wy > BOT_Y1) return;

	/* No buttons pressed so set draw tail and do sound/filter */
	tail[tail_next].x = (double)x - RADIUS * g_x_scale;
	tail[tail_next].y = (double)y - RADIUS * g_y_scale;
	tail[tail_next].colour = 0;
	tail_next = (tail_next + 1) % TAIL_LEN;

	send_play_off = 1;

	/* If right mouse button pressed then only do filter so we
	   can play using the keyboard but still change filter. */
	if (mouse_button != RIGHT_MBUTTON) setFreqByX(wx,mouse_button);
	setFilterValueByY(wy);
}
