/*
** dbclient.h
**      CSSE2310/7231 - Assignment Four - 2022 - Semester One
**
**      Written by Jamie Katsamatsas, j.katsamatsas@uq.net.au
**      s4674720
*/

#ifndef DBCLIENT_H
#define DBCLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <netdb.h>
#include <csse2310a3.h>
#include <csse2310a4.h>
#include "http.h"

/* Arguments passed into dbclient */
typedef struct {
    char* port;
    char* key;
    char* value;
} ClientArguments;

/* Different types of exit statuses */
typedef enum {
    OK = 0,
    USAGE_ERROR = 1,
    CONNECTION_ERROR = 2,
    GET_REQUEST_ERROR = 3,
    PUT_REQUEST_ERROR = 4
} ErrorType;

/* process_command_line()
* −−−−−−−−−−−−−−−
* Validates the command line arguents given to dbclient.
*
* The expected structure of the command line arguments is:
*
*     ./dbclient portnum key [value]
*
* "portnum" is the portnumber the client is to send the request to. "key" is
* the key to pass to the server. "value" is optional and is the value 
* associated with the key that is passed to the server
* 
* argc: the number of command line aguments given.
* argv: array containing the command line arguments.
*
* Returns: ClientArguments struct containing the values extracted from the 
* command line arguments.
* Errors: if there are not enough command line arguments passed USAGE_ERROR_MSG
* is printed and the programs exists with status 1.
* If the key contains a space or newline character KEY_ERROR is printed and 
* the program exits with status 1.
*/
ClientArguments process_command_line(int argc, char** argv);

/* create_connection()
* −−−−−−−−−−−−−−−
* Creates a TCP connection on localhost using the port number provided.
*
* clientArgs: the command line arguments provided to dbclient. Not NULL
*
* Returns: file descriptor of the open connection
* Errors: if the program is unable to get the address information or if the 
* connection fails PORT_CONNECTION_ERROR is printed and the program exits with
* status 2
* Reference: CSSE2310 Semester 1 2022 Week 9 net2.c
*/
int create_connection(ClientArguments clientArgs);

/* construct_http_request()
* −−−−−−−−−−−−−−−
* Constructs and sends a GET or PUT http request to the given file stream 
* depending on the given parameters.
*
* If a key and value is provided a PUT request is created. If only a key is 
* provided then a GET request is created.
*
* key: key to include in the http request
* value: value to include in the http request
* toServer: file stream used to write to the server
*/
void construct_http_request(char* key, char* value, FILE* toServer);

/* is_get_request()
* −−−−−−−−−−−−−−−
* Checks if the command line arguments are for a get request.
*
* The request is GET if the value of "value" passed in to the command line is 
* NULL.
*
* value: command line argument passed into dbclient
*
* Returns: true if value == NULL. false otherwise.
*/
bool is_get_request(char* value);

/* exit_client()
* −−−−−−−−−−−−−−−
* Exits the client with the exit status corresponding to the http response 
* received.
*
* If the http response status == 200 then the program exits with status 0.
* Otherwise if the request was a GET the program exits with status 3.
* Otherwise if the request was PUT the program exits with status 4.
*
* httpResponse: HttpResponse struct holding the http response information. Not 
* NULL
* requestIsGet: indicates if the original request sent was a GET request or not
* Not NULL.
*/
void exit_client(HttpResponse* httpResponse, bool requestIsGet);

#endif
