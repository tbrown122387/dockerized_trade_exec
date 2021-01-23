/* Copyright (C) 2019 Interactive Brokers LLC. All rights reserved. This code is subject to the terms
 * and conditions of the IB API Non-Commercial License or the IB API Commercial License, as applicable. */

#include "StdAfx.h"

#include <stdio.h>
#include <stdlib.h>
#include <cstdlib> // std::getenv
#include <chrono>
#include <thread>

#include <signal.h>
#include <unistd.h>
#include <cstring>
#include <atomic>

#include "execution_client.h"


//https://stackoverflow.com/questions/4250013/is-destructor-called-if-sigint-or-sigstp-issued
std::atomic<bool> quit(false);    // signal flag
void got_signal(int)
{
    quit.store(true);
}


const unsigned MAX_ATTEMPTS = 50;
const unsigned SLEEP_TIME = 50; //seconds
// keep in mind it takes about 16 seconds per symbol to request data...



int main(int argc, char** argv)
{
	const char* host = argc > 1 ? argv[1] : std::getenv("IB_GATEWAY_URLNAME");
	int port = atoi(std::getenv("IB_GATEWAY_URLPORT"));
	const char* connectOptions = argc > 3 ? argv[3] : "";
	int clientId = 0;

	unsigned attempt = 0;
	printf( "Start of C++ Socket Client Test %u\n", attempt);


    // more signal handling code from SE
    struct sigaction sa;
    memset( &sa, 0, sizeof(sa) );
    sa.sa_handler = got_signal;
    sigfillset(&sa.sa_mask);
    sigaction(SIGINT,&sa,NULL);

	for (;;) {
		++attempt;
		printf( "Attempt %u of %u\n", attempt, MAX_ATTEMPTS);

		ExecClient client;

		if( connectOptions) {
			client.setConnectOptions( connectOptions);
		}
		
		client.connect( host, port, clientId);
		
		while( client.isConnected()) {
			client.processMessages();

            if( quit.load() ) return 0;    // exit normally after SIGINT

		}
		if( attempt >= MAX_ATTEMPTS) {
			break;
		}

		printf( "Sleeping %u seconds before next attempt\n", SLEEP_TIME);
		std::this_thread::sleep_for(std::chrono::seconds(SLEEP_TIME));
	}

	printf ( "End of C++ Socket Client Test\n");
}

