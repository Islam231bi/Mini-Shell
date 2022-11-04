
/*
 * CS-413 Spring 98
 * shell.y: parser for shell
 *
 * This parser compiles the following grammar:
 *
 *	cmd [arg]* [> filename]
 *
 * you must extend it to understand the complete shell grammar
 *
 */

// Yacc definitions

%union	{
		char  *string_val;
	}

%token	<string_val> WORD

%token 	NOTOKEN GREAT NEWLINE SMALL GREATGREAT PIPE AMPERSAND WILDCARD GREATAMPERSAND GREATGREATAMPERSAND 

// C declarations

%{
extern "C" 
{
	int yylex();
	void yyerror (char const *s);
}
#define yylex yylex
#include <stdio.h>
#include "command.h"
%}

%%


// Grammar 
goal:	
	commands
	;


// non-terminal LHS

commands: 
	command 
	| commands command 
	;

command: simple_command
        ;

simple_command:
	pipeline iomodifier_opt_list ampersand NEWLINE {
		printf("   Yacc: Execute command\n");
		Command::_currentCommand.execute();
	}
	| NEWLINE
	| error NEWLINE { yyerrok; }
	;

ampersand:
	AMPERSAND {
		Command::_currentCommand._background = 1;
	}
	|/*can be empty*/
	;

pipeline:
	pipeline PIPE command_and_args
	| command_and_args
	;

command_and_args:
	command_word arg_list {
		Command::_currentCommand.
			insertSimpleCommand( Command::_currentSimpleCommand );
	}
	;


arg_list:
	arg_list argument
	| /* can be empty */
	;

argument:
	WORD {
               printf("   Yacc: insert argument \"%s\"\n", $1);

	       Command::_currentSimpleCommand->insertArgument( $1 );\
	}
	;

command_word:
	WORD {
               printf("   Yacc: insert command \"%s\"\n", $1);
	       
	       Command::_currentSimpleCommand = new SimpleCommand();
	       Command::_currentSimpleCommand->insertArgument( $1 );
	}
	;

iomodifier_opt_list:
	 iomodifier_opt
	| iomodifier_opt_list iomodifier_opt
	;

iomodifier_opt:
	GREAT WORD {
		printf("   Yacc: insert output \"%s\"\n", $2);
		Command::_currentCommand._outFile = $2;
	}
	|GREATGREAT WORD {
		printf("   Yacc: insert output \"%s\"\n", $2);
		Command::_currentCommand._outFile = $2;
		Command::_currentCommand._append = 1;
	}

	| SMALL WORD{
		printf("   Yacc: insert Input \"%s\"\n", $2);
		Command::_currentCommand._inputFile = $2;
	}
	|GREATGREATAMPERSAND WORD {
		printf("   Yacc: insert output \"%s\"\n", $2);
		printf("   Yacc: insert Error \"%s\"\n", $2);
		Command::_currentCommand._outFile = $2;
		Command::_currentCommand._errFile = $2;
		Command::_currentCommand._append = 1;
	}
	|GREATAMPERSAND WORD {
		printf("   Yacc: insert output \"%s\"\n", $2);
		printf("   Yacc: insert Error \"%s\"\n", $2);
		Command::_currentCommand._outFile = $2;
		Command::_currentCommand._errFile = $2;
	}
	|/*can be empty*/  
	;
	
%%

void
yyerror(const char * s)
{
	fprintf(stderr,"%s", s);
}

#if 0
main()
{
	yyparse();
}
#endif
