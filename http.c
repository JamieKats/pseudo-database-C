/*
** http.c
**      CSSE2310/7231 - Assignment Four - 2022 - Semester One
**
**      Written by Jamie Katsamatsas, j.katsamatsas@uq.net.au
**      s4674720
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "http.h"

bool valid_http_method_and_address(HttpRequest* httpRequest) {
    char* method = httpRequest->method;
    // HTTP request method must be either "GET", "PUT". or "DELETE"
    if (strcmp(method, "GET") != 0 && strcmp(method, "PUT") != 0 
	    && strcmp(method, "DELETE") != 0) {
        return false;
    }
    char* dbType = httpRequest->dbType;
    // Type of string store is either "public" or "private"
    if (strcmp(dbType, "public") != 0 && strcmp(dbType, "private") != 0) {
        return false;
    }
    return true;
}

char* get_status_explanation(int status) {
    if (status == STATUS_OK) {
        return strdup(STATUS_EXPLANATION_OK);
    } else if (status == STATUS_BAD_REQUEST) {
        return strdup(STATUS_EXPLANATION_BAD_REQUEST);
    } else if (status == STATUS_NOT_FOUND) {
        return strdup(STATUS_EXPLANATION_NOT_FOUND);
    } else if (status == STATUS_INTERNAL_SERVER_ERROR) {
        return strdup(STATUS_EXPLANATION_INTERNAL_SERVER_ERROR);
    } else if (status == STATUS_UNAUTHORIZED) {
	return strdup(STATUS_EXPLANATION_UNAUTHORIZED);
    } else {
        return NULL;
    }
}

void deconstruct_address(HttpRequest* httpRequest, char* address) {
    char delimiter = '/';
    unsigned int maxFields = 3; // Number of strings to split string by
    char** tokens = split_by_char(address, delimiter, maxFields);
    httpRequest->dbType = strdup(tokens[1]);
    httpRequest->key = strdup(tokens[2]);
    free(address);
    free(tokens);
}

char* get_auth_string(HttpRequest* httpRequest) {
    int i = 0;
    while (httpRequest->headers[i]) {
        if (strcmp(httpRequest->headers[i]->name, "Authorization") == 0) {
	    return httpRequest->headers[i]->value;
	}
	i++;
    }
    return NULL;
}

void free_http_request(HttpRequest* httpRequest) {
    free(httpRequest->method);
    free(httpRequest->dbType);
    free(httpRequest->key);
    free_array_of_headers(httpRequest->headers);
    free(httpRequest->body);
}

void free_http_response(HttpResponse* httpResponse) {
    free(httpResponse->statusExplanation);
    free_array_of_headers(httpResponse->headers);
    free(httpResponse->body);
}
