
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

#include "command.h"

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

	// if ( _errFile && (_outFile) ) {
	// 	free( _errFile );
	// }

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
	// print();

	// File redirection
	int defaultin = dup( 0 );
	int defaultout = dup( 1 );
	int defaulterr = dup( 2 );
	
	int infd = -1;
	int outfd = -1;
	int errfd = -1;

	if (_currentCommand._outFile != 0){
		if (_currentCommand._append == 1){
			outfd = open( _currentCommand._outFile , O_CREAT|O_WRONLY|O_APPEND, 0666);
		}
		else{
			outfd = open( _currentCommand._outFile ,O_CREAT|O_WRONLY|O_TRUNC, 0666);
		}
	
		if ( outfd < 0 ) {
			perror( "Error creating out file" );
			exit( 2 );
		}
		// Redirect output to the created outfile instead off printing to stdout 
		dup2( outfd, 1 );
		close( outfd );

		// Redirect input
		dup2( defaultin, 0 );
		
		// Redirect output to file
		dup2( outfd, 1 );

		// Redirect err
		dup2( defaulterr, 2 );

	}

	if (_currentCommand._inputFile != 0){
		infd = open( _currentCommand._inputFile , O_RDWR, 0666);
	
		if ( infd < 0 ) {
			perror( "Error reading from Input file" );
			exit( 2 );
		}
		// Redirect input to the assigned input file instead off reading from stdin 
		dup2( infd, 0 );
		close( infd );

		// Redirect output
		dup2( defaultout, 1 );

		// Redirect err
		dup2( defaulterr, 2 );

	}

	if (_currentCommand._errFile != 0){
		// Open for appending
		if (_currentCommand._append == 1){
			errfd = open( _currentCommand._errFile , O_CREAT|O_WRONLY|O_APPEND, 0666);
		}
		// Open for overwriting
		else{
			errfd = open( _currentCommand._errFile ,O_CREAT|O_WRONLY|O_TRUNC, 0666);
		}
	
		if ( errfd < 0 ) {
			perror( "Error creating Error file" );
			exit( 2 );
		}
		// Redirect error to the created error file instead off printing to srderr 
		dup2( errfd, 2 );
		close( errfd );

		// Redirect input
		dup2( defaultin, 0 );
	
		// Redirect err
		dup2( errfd, 2 );

	}

	int pid = fork();
	if ( pid == -1 ) {
		perror( "ls: fork\n");
		exit( 2 );
	}
	if (pid == 0) {
		//Child
		const auto size = _currentCommand._currentSimpleCommand->_numberOfArguments;
		char *args[size + 1];
		args[size] = NULL;

		for (auto i = 0 ; i < size ; i++){
			args[i] = _currentCommand._currentSimpleCommand->_arguments[i];
		}

		execvp(args[0],args);

		perror( "Command Error: ");
		exit( 2 );
	}

	// Restore input, output, and error

	dup2( defaultin, 0 );
	dup2( defaultout, 1 );
	dup2( defaulterr, 2 );

	// Close file descriptors that are not needed
	close( outfd );
	close( infd );
	close( errfd );
	close( defaultin );
	close( defaultout );
	close( defaulterr );

	if (_currentCommand._background == 0)
		waitpid( pid, 0, 0 );
	

	
	// Add execution here
	// For every simple command fork a new process
	// Setup i/o redirection
	// and call exec



	// Clear to prepare for next command
	clear();
	
	// Print new prompt
	prompt();
}

// Shell implementation

void
Command::prompt()
{
	printf("myshell>");
	fflush(stdout);
}

Command Command::_currentCommand;
SimpleCommand * Command::_currentSimpleCommand;

int yyparse(void);

int 
main()
{
	Command::_currentCommand.prompt();
	yyparse();
	return 0;
}

