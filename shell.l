
/*
 *
 * CS-413 Spring 98
 * shell.l: lexical analyzer for shell
 * You have to extend it.
 *
 */

/*
 *first part (optional)
 */
%{

#include <string.h>
#include "y.tab.h"

%}

/*
 *
 * Second part
 */
%%

\n 	{
		return NEWLINE;
	}

[ \t] 	{
		/* Discard spaces and tabs */
     	}	

">" 	{
		return GREAT;
	}

"<"		{
		return SMALL;
	}

"<<"	{
		return SMALLSMALL;
	}

">>"	{
		return GREATGREAT;
	}

"|"		{
		return PIPE;
	}

[^ \t\n][^ \t\n]*  {
		/* Assume that file names have only alpha chars */
		yylval.string_val = strdup(yytext);
		return WORD;
	}

.	{
		/* Invalid character in input */
		return NOTOKEN;
	}

%%
	
