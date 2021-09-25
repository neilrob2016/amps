/*** Support for st_value struct which is used for the dual numeric/string
     nature of BASIC variables and expressions ***/

#include "globals.h"


void initValue(st_value *value)
{
	value->strval = NULL;
	value->val = 0;
}




void setValue(st_value *value, double val, char *strval)
{
	clearValue(value);

	if (strval)
	{
		value->strval = strdup(strval);
		assert(value->strval);
	}
	else value->val = val;
}




/*** Set value1 based on value2 */
void setValueByValue(st_value *value1, st_value *value2)
{
	setValue(value1,value2->val,value2->strval);
}




void setDirectStringValue(st_value *value, char *strval)
{
	clearValue(value);
	value->strval = strval;
	value->val = 0;
}




void appendStringValue(st_value *value1, st_value *value2)
{
	char *str1;
	char *str2;
	value1->val = 0;

	str1 = value1->strval;
	str2 = value2->strval;

	if (str2) 
	{
		if (str1)
		{
			str1 = realloc(str1,strlen(str1)+strlen(str2)+1);
			assert(str1);
			strcat(str1,str2);
		}
		else
		{
			str1 = strdup(str2);
			assert(str1);
		}
	}
	value1->strval = str1;
}




/*** Multiply the string by given count. Caller needs to free memory ***/
char *multStringValue(st_value *value, int cnt)
{
	char *out;
	int i;

	if (!cnt || !value->strval) return strdup("");
	assert((out = (char *)malloc(strlen(value->strval) * cnt + 1)));
	out[0] = 0;

	for(i=0;i < cnt;++i) strcat(out,value->strval);
	return out;
}




void clearValue(st_value *value)
{
	FREE(value->strval);
	value->val = 0;
}




int trueValue(st_value *value)
{
	return (value->val || (value->strval && value->strval[0]));
}
