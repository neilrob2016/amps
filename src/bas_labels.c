/*** BASIC label support. eg: @mylabel ***/

#include "globals.h"

void addLabel(char *name, int loc)
{
	st_label *label;

	label = (st_label *)malloc(sizeof(st_label));

	/* Don't need to strdup as name points to token->str and this won't
	   be deleted */
	assert(name[0] == '@');
	label->name = name+1;
	label->next = NULL;
	label->loc = loc;

	if (first_label)
		last_label->next = label;
	else
		first_label = label;

	last_label = label;
}




int getLabelLocation(char *name)
{
	st_label *label;

	for(label=first_label;label;label=label->next)
		if (!strcmp(name,label->name)) return label->loc;
	return 0;
}
