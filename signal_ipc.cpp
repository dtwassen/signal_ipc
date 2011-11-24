/*
 * Copyright (c) 2011, Dirk Tassilo Hettich <dthettich@gmail.com>
 *
 
 MIT License (MIT)
 
 Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>

#include <stdlib.h>

using namespace std;

volatile sig_atomic_t send_ready = 0;

volatile sig_atomic_t handshake_ack = 0;

volatile sig_atomic_t received_zero = 0;

volatile sig_atomic_t received_sthn = 0;


/* child process function */
void signal_ready ( int sig ) {
		
	send_ready = 1;
}

/* handshake */
void handshake ( int sig ) {
		
	handshake_ack = 1; 
}

/* received signal */
void received_signal( int sig ){

	if ( sig == SIGUSR1 ) {
		
		received_zero = 1;
	} else {
		received_zero = 0;
	}
	
	received_sthn = 1;
}
	

int main (void)	{
	
	int pid, j, i;
	
	pid = fork();
	
	// catch error on fork
	if ( pid == -1 ) {
		perror( "Could not fork process." );
		exit( 1 );
	}
	// child process
	else if (pid == 0)
	{
		struct sigaction parent_ready;
		sigset_t block_mask;
		pid_t parent_id = getppid( ); // get parent id
		
		/* Establish the child signal handler. */
		if ( sigfillset( &block_mask ) == -1 ) {
			printf( "sigfillset failed.\n" );
			exit( 1 );
		}
		parent_ready.sa_handler = signal_ready; // function to call
		parent_ready.sa_mask = block_mask;
		parent_ready.sa_flags = 0;
		if ( sigaction( SIGUSR1, &parent_ready, NULL ) == -1 ) {
			printf( "Could not establish child signal handler.\n" );
			exit( 1 );
		}
		printf( " INFO Child: Handler established. \n" );

		
		char message[100];
		printf( "Please enter message: \n" );
		cin.getline( message, 100 );
		
		printf( " INFO Child: Hello \n" );
		
		
		if ( kill( parent_id, SIGUSR1 ) == -1 ) {
			perror( "Could not send signal SIGUSR1.");
			exit( 1 );
		}
				
		for ( i = 0; i < 100; ++i ) {
			
			if ( i>0 && message[i - 1] == '\0' ) {
				
				printf( " INFO Child terminated\n" );
				exit( 0 );
			}
			
			// send chars in bits to parent
			for ( j = 0; j < 8; ++j ) {
				while ( !send_ready ) {
					;
				}
				send_ready = 0;

				// get bit at pos j of char i
				if ( !( message[i] & (1 <<  j)) ) {
					// catch error
					if ( kill( parent_id, SIGUSR1 ) == -1 ) {
						perror( "Could not send signal SIGUSR1.");
						exit( 1 );
					}

				} else {
					if ( kill( parent_id, SIGUSR2 ) == -1 ) {
						perror( "Could not send signal SIGUSR2.");
						exit( 1 );
					}

				}
								
			}
		}

		exit (0);
	}
	else if (pid > 0)
	{ 
		printf( " INFO Initiate handshake.\n" );
		// receive
		if ( signal( SIGUSR1, handshake ) == SIG_ERR ) {
			printf("Could not send signal\n.");
			exit( 1 );
		} 
		
		while ( !handshake_ack ) ;
		
		printf( " INFO Parent: ACK \n" ); 

		struct sigaction handle_SIGUSR1;
		sigset_t block_mask1;
		
		/* Establish the parent signal handler. */
		if ( sigfillset( &block_mask1 ) == -1 ) {
			printf( "sigfillset parent SIGUSR1 failed.\n" );
			exit( 1 );
		}
		handle_SIGUSR1.sa_handler = received_signal; // function to call
		handle_SIGUSR1.sa_mask = block_mask1;
		handle_SIGUSR1.sa_flags = 0;
		
		if ( sigaction( SIGUSR1, &handle_SIGUSR1, NULL ) == -1 ) {
			printf( "Could not establish parent signal handler SIGUSR1.\n" );
			exit( 1 );
		}
		
		printf( " INFO Parent: Handler SIGUSR1 established. \n" );
		
		
		struct sigaction handle_SIGUSR2;
		sigset_t block_mask2;
		
		/* Establish the parent signal handler. */
		if ( sigfillset( &block_mask2 ) == -1 ) {
			printf( "sigfillset parent SIGUSR2 failed.\n" );
			exit( 1 );
		}
		handle_SIGUSR2.sa_handler = received_signal; // function to call
		handle_SIGUSR2.sa_mask = block_mask2;
		handle_SIGUSR2.sa_flags = 0;
		if ( sigaction( SIGUSR2, &handle_SIGUSR2, NULL ) == -1 ) {
			printf( "Could not establish parent signal handler SIGUSR2.\n" );
			exit( 1 );
		}
		printf( " INFO Parent: Handler SIGUSR2 established. \n" );
		
		
		char buffer = 0;
		int shift_counter = 0;
		
		// parent process, main
		while ( true ) {
			
			// send
			if ( kill( pid, SIGUSR1 ) == -1 ) {
				perror( "Could not send signal SIGUSR1.");
				exit( 1 );
			} 

			while ( !received_sthn ) ;
			
			if ( received_zero ) {
				// set bit at position shift counter to 0
				buffer = buffer & ~( 1 << shift_counter );
			} else {
				// set bit at position shift counter to 1
				buffer = buffer | ( 1 << shift_counter );
			}
			
			++shift_counter;

			if ( shift_counter == 8 ) { // output char
				printf( "%c", buffer );
				
				if ( buffer == '\0' ) {
					printf( "\n INFO Process terminated\n" );

					exit( 0 );
				}
				
				shift_counter = 0;
			}
			// read next bit 
			received_sthn = 0;
						
		}
			
	}
	else
	{   
		/* if negative value -> error */
		fprintf (stderr, "Error");
		exit (1);
	}
	return 0;
}