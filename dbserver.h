/*
** dbserver.h
**      CSSE2310/7231 - Assignment Four - 2022 - Semester One
**
**      Written by Jamie Katsamatsas, j.katsamatsas@uq.net.au
**      s4674720
*/

#ifndef DBSERVER_H
#define DBSERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <csse2310a3.h>
#include <csse2310a4.h>
#include <semaphore.h>
#include "http.h"

/* Public and Private instances of string stores */
typedef struct {
    StringStore* publicStore;
    StringStore* privateStore;
} StringStores;

/* The arguments passed to dbserver */
typedef struct {
    char* authfile;
    int connections;
    char* port;
} ServerArguments;

/* The dbserver statistics */
typedef struct {
    int connectedClients;
    int completedClients;
    int authFailures;
    int getOperations;
    int putOperations;
    int deleteOperations;
} Statistics;

/* Locks for the database and statistics to enforce mutual exclusion */
typedef struct {
    sem_t databaseLock;
    sem_t statisticsLock;
} Locks;

/* Arguments passed to the thread handling client connections */
typedef struct ThreadArguments {
    int fdClient;
    Locks* locks;
    Statistics* stats;
    StringStores* stringStores;
    ServerArguments* serverArgs;
} ThreadArguments;

/* Arguments passed to the thread handling the signal SIGHUP */
typedef struct {
    Statistics* stats;
    sigset_t set;
    Locks* locks;
} SignalThreadArguments;

/* The different types of exit statuses */
typedef enum {
    OK = 0,
    USAGE_ERROR = 1,
    AUTHENTICATION_ERROR = 2,
    LISTEN_ERROR = 3
} ErrorType;

/* process_command_line()
* −−−−−−−−−−−−−−−
* Validates the command line arguments given to dbserver.
*
* The expected structure of the command line arguments is:
*
*     ./dbserver authfile connections [portnum]
*
* "authfile" is the name of a text file containing the authentication string. 
* "connections" is a positive integer limiting the number of allowed active 
* connections. "portnum" is the portnumber the server is to bind to that must 
* be a positive integer between 1024 and 65535 inclusive.
*
* argc: the number of command line arguments passed.
* argv: an array containing the command line arguments
*
* Returns: initialised ServerArguments struct
* Errors: exists with status 1 if there is any error with the provided 
* arguments
*/
ServerArguments process_command_line(int argc, char** argv);

/* initialise_server()
* −−−−−−−−−−−−−−−
* Initialises the server connection.
*
* Creates a socket that the server listens on to receive incoming connections.
* If the provided port is "0" the server will attempt to listen on an ephemeral
* port
*
* port: the port the server will try to bind to and listen on.
*
* Returns: the file descriptor the server is listening on
* Errors: if there is an issue getting the address info this function will exit
* early.
* Failure to bind to the specified port will print out PORT_BIND_ERROR and
* exit with status 3
* Reference: CSSE2310 Week 10 server-multithreaded.c
*/
int initialise_server(const char* port);

/* process_connections()
* −−−−−−−−−−−−−−−
* Accepts new connections and creates threads to handle the incoming 
* connections.
*
* If the connection limit is reached the client connection handling thread will
* not be created.
*
* fdServer: file descriptor the server listens on for incomming connections. 
* Not NULL
* serverArgs: ServerArguments struct containing the command line arguments used
* when calling dbserver. Not NULL
* 
* Reference: CSSE2310 Week 10 server-multithreaded.c
*/
void process_connections(int fdServer, ServerArguments serverArgs);

/* client_thread()
* −−−−−−−−−−−−−−−
* Thread function opens file streams to the client and processes requests.
*
* This function will handle multiple requests coming from one client. If the 
* incoming http request is valid the dbserver statistics will update 
* accordingly.
*
* arg: ThreadArguments struct cast to a void*. Not NULL
*
* Reference: CSSE2310 Week 10 server-multithreaded.c
*/
void* client_thread(void* arg);

/* process_client_request()
* −−−−−−−−−−−−−−−
* Processes an individual client http request.
*
* The http request is read and authentication is checked for validity if 
* required. If the request is valid a http response is formed and sent to the 
* client.
*
* to: file stream used to write to the client. Not NULL
* from: File stream used to receive data from the client. Not NULL
* threadArgs: ThreadArguments struct containing data passed into the 
* thread function. Not NULL
*
* Returns: 0 if the http request recieved is malformed. 1 if the http request
* is succesfully received, processed and sent to the client.
*/
int process_client_request(FILE* to, FILE* from, ThreadArguments* threadArgs);

/* valid_authfile()
* −−−−−−−−−−−−−−−
* Checks if the authfile provided is valid.
*
* A valid auth file is one that can be opened for reading and does not have an
* empty first line.
*
* authCommand: the name of the authfile that was passed as a command line 
* argument on call to dbserver
*
* Returns: true if the authentication file is valid, false otherwise. 
*/
bool valid_authfile(char* authCommand);

/* print_port()
* −−−−−−−−−−−−−−−
* Prints the port number the server is bound to.
*
* serverFd: the file descriptor the server is bound to.
*
* Reference: CSSE2310 Week 10 server-multithreaded.c
*/
void print_port(int serverFd);

/* inti_lock()
* −−−−−−−−−−−−−−−
* Initialises a semaphore lock with a value of 1.
*
* lock: the semaphore to initialise as a lock
*
* Reference: CSSE2310 Week 7 race3.c
*/
void init_lock(sem_t* lock);

/* take_lock()
* −−−−−−−−−−−−−−−
* Takes a lock of the given semaphore.
*
* lock: the semaphore to wait on
*
* Reference: CSSE2310 Week 7 race3.c
*/
void take_lock(sem_t* lock);

/* release_lock()
* −−−−−−−−−−−−−−−
* Release a lock of the given semaphore.
*
* lock: the semaphore to post with
*
* Reference: CSSE2310 Week 7 race3.c
*/
void release_lock(sem_t* lock);

/* initialise_stringstores()
* −−−−−−−−−−−−−−−
* Initialises the public and private string stores.
*
* Returns: pointer to StringStores containing public and private stringstore 
* created with malloc.
*/
StringStores* initialise_stringstores(void);

/* check_valid_authentication()
* −−−−−−−−−−−−−−−
* Checks if the authentication string provided in the http request header 
* matches with the authentication file used by the server.
* 
* The first line in the authentication file is used as the authentication 
* string for the database server, that must match up with the authentication
* http header provided
*
* httpRequest: HttpRequest struct containing the http request information Not 
* NULL
* threadArgs: ThreadArguments struct containing the arguments passed into the 
* client thread function. Not NULL
*
* Returns: true if the authenitcation string is valid, false otherwise
*/
bool check_valid_authentication(HttpRequest* httpRequest, 
	ThreadArguments* threadArgs);

/* check_connection_limit()
* −−−−−−−−−−−−−−−
* Checks if the connection limit has been reached.
*
* The connection limit specified in the dbserver command line arguments 
* restricts the number of active clients that can be connected to the server 
* at once.
*
* fdClient: file descriptor used to communicate with client
* locks: Locks struct containing the statistics lock. Not NULL
* stats: Statistics struct containing the statistics for the server. Not NULL
* serverArgs: ServerArguments struct containing the arguments passed into 
* dbserver. Not NULL
*
* Returns: true if the client is able to connect. false if the connection
* limit has been reached, and client is unable to connect.
*/
bool check_connection_limit(int fdClient, Locks* locks, Statistics* stats, 
	ServerArguments serverArgs);

/* create_signal_thread()
* −−−−−−−−−−−−−−−
* Creates a thread that handles incoming SIGHUP signals.
*
* SIGHUP is blocked on the main thread and a new thread is created to handle
* SIGHUP.
*
* stats: Statistics struct that holds the statistics for dbserver. Not NULL
* locks: Locks struct holding the locks for the database and statistics. Not 
* NULL
*
* Reference: pthread_sigmask(3) man page example
*/
void create_signal_thread(Statistics* stats, Locks* locks);

/* signal_thread()
* −−−−−−−−−−−−−−−
* Catches SIGHUP and prints out the statistics.
*
* arg: SignalThread struct holding the parameters passed into signal_thread 
* cast as a void*. Not NULL.
*
* Reference: pthread_sigmask(3) man page example 
*/
void* signal_thread(void* arg);

/* update_statistics()
* −−−−−−−−−−−−−−−
* Updates the statistics stuct according to the httpRequest that is passed in.
*
* The statistics are only updated when the http response contains a 
* httpResponse->status value of STATUS_OK. The statistics are updated according
* to the type of request passed in (GET, PUT, DELETE), a successfull GET, PUT 
* or DELETE request results in the corresponding statistic to increase by 1.
*
* httpRequest: HttpRequest struct holding the information of the http request, 
* Not NULL
* httpResponse: httpResponse struct holding the information of the http 
* response. Not NULL
* threadArgs: ThreadArguments struct holding the arguments passed into the 
* signal_thread
*/
void update_statistics(HttpRequest* httpRequest, HttpResponse* httpResponse, 
	ThreadArguments* threadArgs);

/* handle_http_request()
* −−−−−−−−−−−−−−−
* Handles the http request, calling the necessary stringstore functions.
*
* The relevant stringstore function is called according to the type of http 
* request provided. This function will handle GET, PUT and DELETE http 
* requests. If the http request provided has an invalid method or address or if
* the message is unauthorized then the function will return before executing 
* any stringstore functions.
*
* httpRequest: HttpRequest struct holding the http request information. Not 
* NULL.
* httpResponse: HttpResponse struct holding the http response information. Not 
* NULL.
* threadArgs: ThreadArguments struct holding the arguments passed to the 
* client thread
*/
void handle_http_request(HttpRequest* httpRequest, HttpResponse* httpResponse, 
	ThreadArguments* threadArgs);

/* initialise_thread_arguments()
* −−−−−−−−−−−−−−−
* Initialises the thread arguments struct.
*
* Returns: pointer to a ThreadArguments struct created with malloc
*/
ThreadArguments* initialise_thread_arguments(void);

#endif
