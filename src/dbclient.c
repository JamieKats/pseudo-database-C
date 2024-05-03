/*
** dbclient.c
**      CSSE2310/7231 - Assignment Four - 2022 - Semester One
**
**      Written by Jamie Katsamatsas, j.katsamatsas@uq.net.au
**      s4674720
**
** Usage:
**      dbclient portnum key [value]
** Any additional arguments are to be silently ignored.
** portnum: the port the client is to connect to
** key: key to send to the server in the http request
** value: value to send to the server in the http request body
*/

#include "dbclient.h"

// Minimum number of arguments required by dbclient 
#define MIN_NUM_ARGS 3

// Carriage-return line-feed
#define CRLF "\r\n"

// Usage errors
#define USAGE_ERROR_MSG "Usage: dbclient portnum key [value]\n"
#define KEY_ERROR "dbclient: key must not contain spaces or newlines\n"
#define PORT_CONNECT_ERROR "dbclient: unable to connect to port %s\n"

/* Structure of a http GET and PUT request */
#define GET_REQUEST "GET /public/%s HTTP/1.1%s%s"
#define PUT_REQUEST "PUT /public/%s HTTP/1.1%sContent-Length: %ld%s%s%s"

int main(int argc, char** argv) {
    ClientArguments clientArgs = process_command_line(argc, argv);
 
    // try connect to port
    int fd = create_connection(clientArgs);
    int fd2 = dup(fd);
    FILE* to = fdopen(fd, "w");
    FILE* from = fdopen(fd2, "r");

    construct_http_request(clientArgs.key, clientArgs.value, to);

    bool requestIsGet = is_get_request(clientArgs.value);
    
    // Prepare http response
    HttpResponse httpResponse = {0};
    // memset(&httpResponse, 0, sizeof(HttpResponse));

    get_http_response(from, &httpResponse);
    
    // Free resources and exit
    fclose(to);
    fclose(from);
    exit_client(&httpResponse, requestIsGet);
    return 0;
}

ClientArguments process_command_line(int argc, char** argv) {
    // Check min args are provided
    if (argc < MIN_NUM_ARGS) {
	    fprintf(stderr, USAGE_ERROR_MSG);
        exit(USAGE_ERROR);
    }

    // Check key doesn't contain new lines or spaces
    int keyLen = strlen(argv[2]);
    for (int i = 0; i < keyLen; i++) {
        if (argv[2][i] == ' ' || argv[2][i] == '\n') {
            fprintf(stderr, KEY_ERROR);
            exit(USAGE_ERROR);
	    }
    }

    // Set up ClientArguments struct used to pass around the command arguments
    ClientArguments clientArgs;
    clientArgs.port = argv[1];
    clientArgs.key = argv[2];
    clientArgs.value = NULL;
    if (argc > 3) {
        clientArgs.value = argv[3];
    }
    return clientArgs;
}

int create_connection(ClientArguments clientArgs) {
    // Get IPv4 TCP addr info
    struct addrinfo* ai = 0;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM;
    int err;
    err = getaddrinfo("localhost", clientArgs.port, &hints, &ai);

    // Connect to server
    int fdServer = socket(AF_INET, SOCK_STREAM, 0); // TCP
    if (
        err || connect(
            fdServer, 
            (struct sockaddr*)ai->ai_addr, 
            sizeof(struct sockaddr)
            )
    ) {
        freeaddrinfo(ai);
	fprintf(stderr, PORT_CONNECT_ERROR, clientArgs.port);
	exit(CONNECTION_ERROR);
    }
    freeaddrinfo(ai);
    return fdServer;
}

void construct_http_request(char* key, char* value, FILE* toServer) {
    if (value != NULL) { // PUT request
        fprintf(
            toServer, 
            PUT_REQUEST,
            key, 
            CRLF, 
            strlen(value), 
            CRLF, 
            CRLF, 
            value);
    } else { // GET Request
        fprintf(toServer, GET_REQUEST, key, CRLF, CRLF);
    }
    fflush(toServer);
}

bool is_get_request(char* value) {
    if (value == NULL) {
        return true;
    }
    return false;
}

void exit_client(HttpResponse* httpResponse, bool requestIsGet) {
    // If response was recieved successfully print out body if GET request 
    // sent
    int exitStatus = OK;
    if (httpResponse->status == STATUS_OK) {
        if (requestIsGet) {
	    fprintf(stdout, "%s\n", httpResponse->body);
	    fflush(stdout);
	}
    } else if (requestIsGet) {
        exitStatus = GET_REQUEST_ERROR;
    } else {
        exitStatus = PUT_REQUEST_ERROR;
    }
    free_http_response(httpResponse);
    exit(exitStatus);
}

