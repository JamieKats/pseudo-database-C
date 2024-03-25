/*
** dbserver.c
**      CSSE2310/7231 - Assignment Four - 2022 - Semester One
**
**      Written by Jamie Katsamatsas, j.katsamatsas@uq.net.au
**      s4674720
**
** Usage:
**      ./dbserver authfile connections [portnum]
** The authfile argument is the name of a text file, the first line of which 
** is to be used as an authentication.
** The connections argument indicates the maximum number of simultaneous client
** connections to be permitted. If this is zero, then there is no limit to how 
** many clients may connect.
** The portnum argument, if specified, indicates which localhost port dbserver 
** is to listen on. If the port number is absent, then dbserver is to use an 
** ephemeral port.
** Any additional arguments are to be silently ignored.
*/

#include "dbserver.h"

/* Error messages */
#define USAGE_ERROR_MSG "Usage: dbserver authfile connections [portnum]\n"
#define PORT_BIND_ERROR "dbserver: unable to open socket for listening\n"
#define AUTH_STRING_ERROR "dbserver: unable to read authentication string\n"

/* Base 10 used for calls to strtol */
#define BASE_10 10

/* Minimum and maximum valid port numbers inclusive */
#define MIN_VALID_PORT_NUM 1024
#define MAX_VALID_PORT_NUM 65535

/* Default port number if none specified */
#define DEFAULT_PORT "0"

/* Minimum number of arguments expected without a port number specified */
#define MIN_NUM_ARGS_WITHOUT_PORTNUM 3

/* Queue length for call to listen() when listening on a socket */
#define LISTEN_QUEUE_LENGTH 100

/* Statistics for dbserver */
#define STATS_CONNECTED_CLIENTS "Connected clients:%d\n"
#define STATS_COMPLETED_CLIENTS "Completed clients:%d\n"
#define STATS_AUTH_FAILURES "Auth failures:%d\n"
#define STATS_GET_OPERATIONS "GET operations:%d\n"
#define STATS_PUT_OPERATIONS "PUT operations:%d\n"
#define STATS_DELETE_OPERATIONS "DELETE operations:%d\n"

/* Minimum and maximum number of arguments required for dbserver */
#define MIN_NUM_ARGS 3
#define MAX_NUM_ARGS 4

int main(int argc, char** argv) {
    ServerArguments serverArgs = process_command_line(argc, argv);
   
    int fdServer = initialise_server(serverArgs.port);
    process_connections(fdServer, serverArgs);
    
    return 0;
}

ServerArguments process_command_line(int argc, char** argv) {
    // Check min args are provided
    if (argc < MIN_NUM_ARGS || argc > MAX_NUM_ARGS) {
	fprintf(stderr, USAGE_ERROR_MSG);
        exit(USAGE_ERROR);
    }

    // Check connections is a positive integer
    char* endOfInt;
    int connections = strtol(argv[2], &endOfInt, BASE_10);
    if (*endOfInt != '\0' || connections < 0) {
	fprintf(stderr, USAGE_ERROR_MSG);
        exit(USAGE_ERROR);
    }

    // Check port number in range of 1024 and 65535
    if (argc > MIN_NUM_ARGS_WITHOUT_PORTNUM) {
        int port = strtol(argv[3], &endOfInt, BASE_10);
	if (*endOfInt != '\0' 
		|| !((port >= MIN_VALID_PORT_NUM && port <= MAX_VALID_PORT_NUM)
		|| port == 0)) {
            fprintf(stderr, USAGE_ERROR_MSG);
            exit(USAGE_ERROR);
	}
    }

    // Check authentication file valid
    if (!valid_authfile(argv[1])) {
        fprintf(stderr, AUTH_STRING_ERROR);
	exit(AUTHENTICATION_ERROR);
    }

    // Set up ServerArguments with valid arguments provided
    ServerArguments serverArgs;
    serverArgs.authfile = argv[1];
    serverArgs.connections = connections;
    serverArgs.port = DEFAULT_PORT;
    if (argc > MIN_NUM_ARGS_WITHOUT_PORTNUM) {
        serverArgs.port = argv[3];
    }
    return serverArgs;
}

int initialise_server(const char* port) {
    struct addrinfo* ai = 0;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET; //IPv4
    hints.ai_socktype = SOCK_STREAM;
    
    // Get information for given port on "localhost"
    int err;
    if ((err = getaddrinfo("localhost", port, &hints, &ai))) {
	freeaddrinfo(ai);
	fprintf(stderr, "%s\n", gai_strerror(err));
	return 1;
    }
    int listenfd = socket(AF_INET, SOCK_STREAM, 0); // TCP

    // Allow address (port number) to be reused immediately
    int optVal = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optVal, sizeof(int));

    // Bind and listen on the port specified
    if (bind(listenfd, (struct sockaddr*)ai->ai_addr, sizeof(struct sockaddr)) 
	    < 0) {
        fprintf(stderr, PORT_BIND_ERROR);
	freeaddrinfo(ai);
	exit(LISTEN_ERROR);
    }

    listen(listenfd, LISTEN_QUEUE_LENGTH);
    print_port(listenfd);
    freeaddrinfo(ai);
    return listenfd;
}

void process_connections(int fdServer, ServerArguments serverArgs) {
    int fdClient;
    struct sockaddr_in fromAddr;
    socklen_t fromAddrSize = sizeof(struct sockaddr_in);

    // Create Initial semaphore lock, stores and statistics structs
    Locks locks;
    memset(&locks, 0, sizeof(Locks));
    init_lock(&(locks.databaseLock));
    init_lock(&(locks.statisticsLock));
    StringStores* stringStores = initialise_stringstores();
    Statistics stats;
    memset(&stats, 0, sizeof(Statistics));
    create_signal_thread(&stats, &locks);

    // Keep accepting new connections and creating threads to handle the 
    // connection
    while(true) {
        fdClient = 
	    accept(fdServer, (struct sockaddr*)&fromAddr, &fromAddrSize);

	if (!check_connection_limit(fdClient, &locks, &stats, serverArgs)) {
	    continue;
	}
        ThreadArguments* threadArgs = initialise_thread_arguments();
        threadArgs->fdClient = fdClient;
	threadArgs->locks = &locks;
	threadArgs->stats = &stats;
	threadArgs->stringStores = stringStores;
	threadArgs->serverArgs = &serverArgs;
	
	pthread_t threadId;
	pthread_create(&threadId, NULL, client_thread, threadArgs);
        pthread_detach(threadId);
    }
}

bool check_connection_limit(int fdClient, Locks* locks, Statistics* stats, 
	ServerArguments serverArgs) {
    take_lock(&(locks->statisticsLock));
    
    // Reject connection if the number of connections == connection limit
    if (serverArgs.connections != 0
	    && stats->connectedClients >= serverArgs.connections) {
        FILE* to = fdopen(fdClient, "w");
	
	// create response
	char* response = construct_HTTP_response(
		STATUS_SERVICE_UNAVAILABLE, 
		STATUS_EXPLANATION_SERVICE_UNAVAILABLE, NULL, NULL);
	
	// Send response
	fprintf(to, "%s", response);
	fflush(to);
	fclose(to);
	release_lock(&(locks->statisticsLock));
	return false;
    }
    stats->connectedClients++;
    release_lock(&(locks->statisticsLock));
    return true;
}

void* client_thread(void* arg) {
    ThreadArguments* threadArgs = (ThreadArguments*)arg;

    // Convert fd to FILE*
    int fd2 = dup(threadArgs->fdClient);
    FILE* to = fdopen(threadArgs->fdClient, "w");
    FILE* from = fdopen(fd2, "r");

    // Keep processing multiple requests from the client
    while (1) {
        if (!process_client_request(to, from, threadArgs)) {
	    break;
	}
    }
    take_lock(&(threadArgs->locks->statisticsLock));
    threadArgs->stats->connectedClients--;
    threadArgs->stats->completedClients++;
    release_lock(&(threadArgs->locks->statisticsLock));
    
    // Free resources and exit
    fclose(to);
    fclose(from);
    free(arg);
    pthread_exit(NULL);
}

int process_client_request(FILE* to, FILE* from, ThreadArguments* threadArgs) {
    HttpRequest httpRequest;
    memset(&httpRequest, 0, sizeof(HttpRequest));
    HttpResponse httpResponse;
    memset(&httpResponse, 0, sizeof(HttpResponse));
    httpRequest.messageAuthenticated = true;

    // If EOF or a badly formed request is received return early
    char* address;
    if (!get_HTTP_request(from, &(httpRequest.method), &address, 
            &(httpRequest.headers), &(httpRequest.body))) {
	return 0;
    }
    deconstruct_address(&httpRequest, address);

    // If authentication fails mark http request as not authenticated. To be
    // handled in handle_http_request
    if (strcmp(httpRequest.dbType, "private") == 0 
	    && !check_valid_authentication(&httpRequest, threadArgs)) {
	take_lock(&(threadArgs->locks->statisticsLock));
	threadArgs->stats->authFailures++;	    
	release_lock(&(threadArgs->locks->statisticsLock));
	httpRequest.messageAuthenticated = false;
    }

    // Handle http request and update statistics
    take_lock(&(threadArgs->locks->databaseLock));
    handle_http_request(&httpRequest, &httpResponse, threadArgs);
    release_lock(&(threadArgs->locks->databaseLock));
    update_statistics(&httpRequest, &httpResponse, threadArgs);
    
    // Create response and send to client
    httpResponse.statusExplanation = 
	    get_status_explanation(httpResponse.status);
    char* response = construct_HTTP_response(httpResponse.status, 
	        httpResponse.statusExplanation, NULL, httpResponse.body);
    fprintf(to, "%s", response);
    fflush(to);

    // Free resources
    free(response);
    free_http_request(&httpRequest);
    free_http_response(&httpResponse); 
    return 1;
}

void update_statistics(HttpRequest* httpRequest, HttpResponse* httpResponse, 
	ThreadArguments* threadArgs) {
    // Only update statistics if the http response status = STATUS_OK
    if (httpResponse->status != STATUS_OK) {
        return;
    }
    take_lock(&(threadArgs->locks->statisticsLock));
    if (strcmp(httpRequest->method, "GET") == 0) {
	threadArgs->stats->getOperations++;
    } else if (strcmp(httpRequest->method, "PUT") == 0) {
	threadArgs->stats->putOperations++;
    } else if (strcmp(httpRequest->method, "DELETE") == 0) {
	threadArgs->stats->deleteOperations++;
    }
    release_lock(&(threadArgs->locks->statisticsLock));
}

void create_signal_thread(Statistics* stats, Locks* locks) {
    SignalThreadArguments* sigThreadArgs = 
	    malloc(sizeof(SignalThreadArguments));
    memset(sigThreadArgs, 0, sizeof(SignalThreadArguments));
    sigset_t set;

    // Block SIGHUP
    sigemptyset(&set);
    sigaddset(&set, SIGHUP);
    pthread_sigmask(SIG_BLOCK, &set, NULL);

    // Create client connection handling thread
    pthread_t threadId;
    sigThreadArgs->set = set;
    sigThreadArgs->stats = stats;
    sigThreadArgs->locks = locks;
    pthread_create(&threadId, NULL, &signal_thread, (void*)sigThreadArgs);
    pthread_detach(threadId);
}

void* signal_thread(void* arg) {
    SignalThreadArguments* sigThreadArgs = (SignalThreadArguments*)arg;
    int sig;

    for (;;) {
        sigwait(&(sigThreadArgs->set), &sig);
	take_lock(&(sigThreadArgs->locks->statisticsLock));
	fprintf(stderr, STATS_CONNECTED_CLIENTS, 
		sigThreadArgs->stats->connectedClients);
	fprintf(stderr, STATS_COMPLETED_CLIENTS, 
		sigThreadArgs->stats->completedClients);
	fprintf(stderr, STATS_AUTH_FAILURES, 
		sigThreadArgs->stats->authFailures);
	fprintf(stderr, STATS_GET_OPERATIONS, 
		sigThreadArgs->stats->getOperations);
	fprintf(stderr, STATS_PUT_OPERATIONS, 
		sigThreadArgs->stats->putOperations);
	fprintf(stderr, STATS_DELETE_OPERATIONS, 
		sigThreadArgs->stats->deleteOperations);
	fflush(stderr);
	release_lock(&(sigThreadArgs->locks->statisticsLock));
    }
}

bool valid_authfile(char* authCommand) {
    FILE* auth = fopen(authCommand, "r");
    if (auth == NULL) {
        return false;
    }

    char* line = read_line(auth);
    if (line == NULL || line[0] == '\0') {
        free(line);
	fclose(auth);
        return false;
    }
    free(line);
    fclose(auth);
    return true;
}

void print_port(int serverFd) {
    struct sockaddr_in ad;
    memset(&ad, 0, sizeof(struct sockaddr_in));
    socklen_t len = sizeof(struct sockaddr_in);
    
    // Get port name and print out
    getsockname(serverFd, (struct sockaddr*)&ad, &len);
    fprintf(stderr, "%u\n", ntohs(ad.sin_port));
    fflush(stderr);
}

void init_lock(sem_t* lock) {
    sem_init(lock, 0, 1);
}

void take_lock(sem_t* lock) {
    sem_wait(lock);
}

void release_lock(sem_t* lock) {
    sem_post(lock);
}

StringStores* initialise_stringstores(void) {
    StringStores* stringStores = (StringStores*)malloc(sizeof(StringStores));
    memset(stringStores, 0, sizeof(StringStores));
    stringStores->publicStore = stringstore_init();
    stringStores->privateStore = stringstore_init();
    return stringStores;
}

bool check_valid_authentication(HttpRequest* httpRequest, 
	ThreadArguments* threadArgs) {
    char* authWord = get_auth_string(httpRequest);
    if (authWord == NULL) {
        return false;
    }

    // Open authfile and read in the auth string stored
    FILE* authFile = fopen(threadArgs->serverArgs->authfile, "r");
    char* storedAuthWord = read_line(authFile);

    // Check if the database requires authentication and if its valid
    if (strcmp(httpRequest->dbType, "private") == 0 
	    && strcmp(authWord, storedAuthWord) == 0) {
        free(storedAuthWord);
	fclose(authFile);
	return true;
    }
    free(storedAuthWord);
    fclose(authFile);
    return false;
}

void handle_http_request(HttpRequest* httpRequest, HttpResponse* httpResponse, 
	ThreadArguments* threadArgs) {
    httpResponse->body = NULL;
    if (!valid_http_method_and_address(httpRequest)) {
        httpResponse->status = STATUS_BAD_REQUEST;
        return;
    }

    // If user not authorized set httpResponse for 401 (Unauthorized) and exit
    if (!httpRequest->messageAuthenticated) {
        httpResponse->status = STATUS_UNAUTHORIZED;
        return;
    }

    // Set stringstore to either public or private
    StringStore* stringStore = threadArgs->stringStores->publicStore;
    if (strcmp(httpRequest->dbType, "private") == 0) {
        stringStore = threadArgs->stringStores->privateStore;
    }

    // Handle different scenarios for GET, PUT and DELETE requests
    httpResponse->status = STATUS_OK;
    if (strcmp(httpRequest->method, "GET") == 0) {
	// GET request response either 200 (OK) | 404 (Not Found)
        char* valueRetrieved = 
	        (char*)stringstore_retrieve(stringStore, httpRequest->key);

	if (valueRetrieved == NULL) {
            httpResponse->status = STATUS_NOT_FOUND;
        } else {
	    httpResponse->body = strdup(valueRetrieved);
	}
    } else if (strcmp(httpRequest->method, "PUT") == 0) {
	// PUT request response either 200 (OK) | 500 (Internal Server Error)
	if (!stringstore_add(stringStore, httpRequest->key, 
		httpRequest->body)) {
	    httpResponse->status = STATUS_INTERNAL_SERVER_ERROR;
	}
    } else if (strcmp(httpRequest->method, "DELETE") == 0) {
	// DELETE request response either 200 (OK) | 404 (Not Found)
	if (!stringstore_delete(stringStore, httpRequest->key)) {
	    httpResponse->status = STATUS_NOT_FOUND;
	}
    }
}

ThreadArguments* initialise_thread_arguments(void) {
    ThreadArguments* threadArgs = 
	    (ThreadArguments*)malloc(sizeof(ThreadArguments));
    memset(threadArgs, 0, sizeof(ThreadArguments));
    return threadArgs;
}

