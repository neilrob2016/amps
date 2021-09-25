/*** Miscallanious BASIC support functions ***/

#include "globals.h"

/*** See if the string contains a valid number ***/
int isNumber(char *str, int allow_neg)
{
	char *s;
	char *s2;
	char *e;
	char *e2;
	int dot;
	int digit;

	/* Skip whitespace front and rear */
	for(s=str;*s && isspace(*s);++s);
	for(e=str+strlen(str)-1;e >= str && isspace(*e);--e);

	if (*s == '-')
	{
		if (++s > e || !allow_neg) return NOT_NUM;
	}

	/* See if its an oct or hex value */
	if ((int)(e-s) > 1 && *s == '0')
	{
		s2 = s+1;
		if (*s2 == 'x')
		{
			strtoll(s,&e2,16);
			return (e2 != e+1 || errno == EINVAL) ? NOT_NUM : NUM_HEX;
		}
		else if (*s2 != '.')
		{
			strtoll(s,&e2,8);
			return (e2 != e+1 || errno == EINVAL) ? NOT_NUM : NUM_OCT;
		}
	}
	
	for(dot=digit=0;*s && s<= e;++s)
	{
		if (!isdigit(*s))
		{
			/* Can only have 1 decimal point */
			if (dot) return 0;
			if (*s == '.') dot = 1; else return 0;
		}
		else digit = 1; /* Have to have at least 1 digit */
	}
	return digit ? NUM_DEC : NOT_NUM;
}




void basicError(int err, st_token *token)
{
	/*** token should never be NULL but just in case... ***/
	if (token)
	{
		dualMessage("BASIC: ERROR %d: %s at '%s' on line %d",
			err,error_mesg[err],token->str,token->file_linenum);
	}
	else dualMessage("BASIC: ERROR %d: %s at '?' on line ?",err,error_mesg[err]);
}
