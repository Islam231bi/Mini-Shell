
/*
 * CS354: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <cstdlib>

#include "command.h"

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_RESET   "\x1b[0m"

using namespace std;

void
handler(int sig)
{
    write(STDOUT_FILENO, " ", 2);
}

SimpleCommand::SimpleCommand()
{
	// Creat available space for 5 arguments
	_numberOfAvailableArguments = 5;
	_numberOfArguments = 0;
	_arguments = (char **) malloc( _numberOfAvailableArguments * sizeof( char * ) );
}

void
SimpleCommand::insertArgument( char * argument )
{
	if ( _numberOfAvailableArguments == _numberOfArguments  + 1 ) {
		// Double the available space
		_numberOfAvailableArguments *= 2;
		_arguments = (char **) realloc( _arguments,
				  _numberOfAvailableArguments * sizeof( char * ) );
	}
	
	_arguments[ _numberOfArguments ] = argument;

	// Add NULL argument at the end
	_arguments[ _numberOfArguments + 1] = NULL;
	
	_numberOfArguments++;
}

Command::Command()
{
	// Create available space for one simple command
	_numberOfAvailableSimpleCommands = 1;
	_simpleCommands = (SimpleCommand **)
		malloc( _numberOfSimpleCommands * sizeof( SimpleCommand * ) );

	_numberOfSimpleCommands = 0;
	_outFile = 0;
	_inputFile = 0;
	_errFile = 0;
	_background = 0;
	_append = 0;
}

void
Command::insertSimpleCommand( SimpleCommand * simpleCommand )
{
	if ( _numberOfAvailableSimpleCommands == _numberOfSimpleCommands ) {
		_numberOfAvailableSimpleCommands *= 2;
		_simpleCommands = (SimpleCommand **) realloc( _simpleCommands,
			 _numberOfAvailableSimpleCommands * sizeof( SimpleCommand * ) );
	}
	
	_simpleCommands[ _numberOfSimpleCommands ] = simpleCommand;
	_numberOfSimpleCommands++;
}

void
Command:: clear()
{
	for ( int i = 0; i < _numberOfSimpleCommands; i++ ) {
		for ( int j = 0; j < _simpleCommands[ i ]->_numberOfArguments; j ++ ) {
			free ( _simpleCommands[ i ]->_arguments[ j ] );
		}
		
		free ( _simpleCommands[ i ]->_arguments );
		free ( _simpleCommands[ i ] );
	}

	if ( _outFile ) {
		free( _outFile );
	}

	if ( _inputFile ) {
		free( _inputFile );
	}

	_numberOfSimpleCommands = 0;
	_outFile = 0;
	_inputFile = 0;
	_errFile = 0;
	_background = 0;
	_append = 0;
}

void
Command::print()
{
	printf("\n\n");
	printf("              COMMAND TABLE                \n");
	printf("\n");
	printf("  #   Simple Commands\n");
	printf("  --- ----------------------------------------------------------\n");
	
	for ( int i = 0; i < _numberOfSimpleCommands; i++ ) {
		printf("  %-3d ", i );
		for ( int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++ ) {
			printf("\"%s\" \t", _simpleCommands[i]->_arguments[ j ] );
		}
		printf("\n");
	}

	printf( "\n\n" );
	printf( "  Output       Input        Error        Background\n" );
	printf( "  ------------ ------------ ------------ ------------\n" );
	printf( "  %-12s %-12s %-12s %-12s\n", _outFile?_outFile:"default",
		_inputFile?_inputFile:"default", _errFile?_errFile:"default",
		_background?"YES":"NO");
	printf( "\n\n" );
	
}

void
Command::execute()
{
	// Don't do anything if there are no simple commands
	if ( _numberOfSimpleCommands == 0 ) {
		prompt();
		return;
	}

	// Print contents of Command data structure
	print();

	// File redirection
	int defaultin = dup( 0 );
	int defaultout = dup( 1 );
	int defaulterr = dup( 2 );
	
	int infd = -1;
	int outfd = -1;
	int errfd = -1;

	const auto cmd_size =  _currentCommand._numberOfSimpleCommands;
	const auto pipeline_size = cmd_size - 1;

	// Array storing all pipes needed
	int fdpipe[pipeline_size][2];

	// initializing pipes
	for (auto i = 0 ; i < pipeline_size - 1 ; i++){
		if ( pipe( fdpipe[i]) == -1 ) {
				perror( "Error creating pipe");
				exit( 2 );
		}
	}

	for (auto cmd = 0 ; cmd < cmd_size ; cmd++) {
		if(!(strcmp(_currentCommand._simpleCommands[cmd]->_arguments[0],"exit"))){
			exit ( 2 );
		}

		if(!(strcmp(_currentCommand._simpleCommands[cmd]->_arguments[0],"cd"))){
			int val = 0;
			if (_currentCommand._simpleCommands[cmd]->_numberOfArguments == 1){
				val = chdir(getenv("HOME"));
			}
			else{
				val = chdir(_currentCommand._simpleCommands[cmd]->_arguments[1]);
			}
			if (val < 0){
				perror("cd ");
			}
			clear();
			prompt();
			return;
		}

		if ( cmd == 0) {
			if (_currentCommand._inputFile != 0) {
				infd = open( _currentCommand._inputFile , O_RDWR, 0666);
			
				if ( infd < 0 ) {
					perror( "Error reading from Input file" );
					exit( 2 );
				}
				// Redirect input to the assigned input file instead off reading from stdin 
				dup2( infd, 0 );
				close( infd );
			}

			if (cmd_size > 1) {
				// Redirect output to pipe
				dup2( fdpipe[cmd][ 1 ], 1 );
			}			
		}

		if (cmd == cmd_size -1) {
			if (_currentCommand._outFile != 0){
				if (_currentCommand._append == 1){
					outfd = open( _currentCommand._outFile , O_CREAT|O_RDWR|O_APPEND, 0666);
				}
				else{
					outfd = open( _currentCommand._outFile ,O_CREAT|O_RDWR|O_TRUNC, 0666);
				}
			
				if ( outfd < 0 ) {
					perror( "Error creating out file" );
					exit( 2 );
				}

				// Redirect output to the created outfile instead off printing to stdout 
				dup2( outfd, 1 );
				close( outfd );
			}

			if (_currentCommand._errFile != 0){
				// Open for appending
				if (_currentCommand._append == 1){
					errfd = open( _currentCommand._errFile , O_CREAT|O_RDWR|O_APPEND, 0666);
				}
				// Open for overwriting
				else{
					errfd = open( _currentCommand._errFile ,O_CREAT|O_RDWR|O_TRUNC, 0666);
				}
			
				if ( errfd < 0 ) {
					perror( "Error creating Error file" );
					exit( 2 );
				}
				// Redirect error to the created error file instead off printing to stderr 
				dup2( errfd, 2 );
				close( errfd );
			}

			if (cmd_size > 1) {
				// Redirect output to pipe
				dup2( fdpipe[cmd - 1][ 0 ], 0 );
			}
		}

		else{
			// getting input from previous pipe
			dup2( fdpipe[cmd - 1][ 0 ], 0 );

			//redirecting output to pipe
			dup2( fdpipe[cmd][1], 1);
		}
		
		int pid = fork();
		if ( pid == -1 ) {
			perror( "Error forking\n");
			exit( 2 );
		}
		if (pid == 0) {

			close( defaultin );
			close( defaultout );
			close( defaulterr );
			close(fdpipe[cmd][0]);
			close(fdpipe[cmd][1]);
			if (cmd > 1){
				close(fdpipe[cmd - 1][0]);
				close(fdpipe[cmd - 1][1]);
			}

			const auto size = _currentCommand._simpleCommands[cmd]->_numberOfArguments;

			// args array
			char *args[size + 1];
			args[size] = NULL;

			for (auto i = 0 ; i < size ; i++){
				args[i] = _currentCommand._simpleCommands[cmd]->_arguments[i];
			}

			//execution
			execvp(args[0],args);

			perror( "Command Error: ");
			exit( 2 );
		}		

		if (_currentCommand._background == 0)
			waitpid( pid, 0, 0 );

	}

	// Restore input, output, and error

	dup2( defaultin, 0 );
	dup2( defaultout, 1 );
	dup2( defaulterr, 2 );

	// Close file descriptors that are not needed
	
	close( defaultin );
	close( defaultout );
	close( defaulterr );

	//Closing all pipes

	for (int i = 0 ; i < pipeline_size ; i++) {
		close( fdpipe[i][0]);
		close( fdpipe[i][1]);
	}

	// Clear to prepare for next command
	clear();
		
	// Print new prompt
	prompt();
}

// Shell implementation

void
Command::prompt()
{
	char *user = getenv("USER");
	char *dist = getenv("DESKTOP_SESSION");
	char *pwd = getenv("PWD");
	printf(ANSI_COLOR_GREEN	"%s@%s-os:"	ANSI_COLOR_RESET ,user,dist);
	printf(ANSI_COLOR_BLUE	"%s"	ANSI_COLOR_RESET,pwd);
	printf("$ ");
	fflush(stdout);
}

Command Command::_currentCommand;
SimpleCommand * Command::_currentSimpleCommand;

int yyparse(void);

int 
main()
{
	signal(SIGINT, handler);
	Command::_currentCommand.prompt();
	yyparse();
	return 0;
}
