/*
** http.h
**      CSSE2310/7231 - Assignment Four - 2022 - Semester One
**
**      Written by Jamie Katsamatsas, j.katsamatsas@uq.net.au
**      s4674720
*/

#ifndef HTTP_H
#define HTTP_H

#include <semaphore.h>
#include <stdbool.h>
#include "stringstore.h"

/* Explanations corresponding with different http request statuses */
#define STATUS_EXPLANATION_OK "OK"
#define STATUS_EXPLANATION_BAD_REQUEST "Bad Request"
#define STATUS_EXPLANATION_UNAUTHORIZED "Unauthorized"
#define STATUS_EXPLANATION_NOT_FOUND "Not Found"
#define STATUS_EXPLANATION_INTERNAL_SERVER_ERROR "Internal Server Error"
#define STATUS_EXPLANATION_SERVICE_UNAVAILABLE "Service Unavailable"

typedef struct HttpHeader {
    char* key;
    char* value;
} HttpHeader;


/* Contains the information in a http request */
typedef struct HttpRequest {
    char* method;
    char* dbType;
    char* key;
    HttpHeader** headers;
    char* body;
    bool messageAuthenticated;
} HttpRequest;

/* The values of a http response */
typedef struct HttpResponse {
    int status;
    char* statusExplanation;
    HttpHeader** headers;
    char* body;
} HttpResponse;

/* Different status values for a http response */
typedef enum {
    STATUS_OK = 200,
    STATUS_BAD_REQUEST = 400,
    STATUS_UNAUTHORIZED = 401,
    STATUS_NOT_FOUND = 404,
    STATUS_INTERNAL_SERVER_ERROR = 500,
    STATUS_SERVICE_UNAVAILABLE = 503
} StatusValues;

/**
 * Returns the HTTP response received on the file stream provided.
 * 
 * from: File stream http response is recieved on
 * http_response: pointer to HttpResponse struct of the received response.
 * 
 * The values of the HTTP response are set in the response pointer provided.
*/
int get_http_response(FILE* from, HttpResponse* http_response);


/* get_status_explanation()
* −−−−−−−−−−−−−−−
* Returns the http response status explanation corresponding to the http 
* response status passed in.
*
* status: The http response status
*
* Returns: copy of the status explanation on success created with malloc, 
* NULL if the status does not match any of the status checks.
*/
char* get_status_explanation(int status);

/* deconstruct_address()
* −−−−−−−−−−−−−−−
* Splits the http request address into its database type and key, and saves
* them in the httpRequest struct.
*
* httpRequest: HttpRequest struct holding the http request information. Not 
* NULL
* address: http request address in the form of "/<database type>/<key>"
*/
void deconstruct_address(HttpRequest* httpRequest, char* address);

/* get_auth_string()
* −−−−−−−−−−−−−−−
* Gets the authentication string from the http request header.
*
* httpRequest: HttpRequest struct holding the http request information. Not 
* NULL
*
* Returns: the authentication string from the http request header. Returns NULL
* if the authentication header doesn't exists
*/
char* get_auth_string(HttpRequest* httpRequest);

/* valid_http_method_and_address()
* −−−−−−−−−−−−−−−
* Checks if the method and address of the http request is valid.
*
* A valid http request method contains one of "GET", "PUT", or "DELETE".
* A valid http request address is one that contains "public", or "private".
*
* httpRequest: HttpRequest struct holding the http request information. Not 
* NULL
*
* Returns: True if both the http request method and address are valid. False 
* otherwise
*/
bool valid_http_method_and_address(HttpRequest* httpRequest);

/* free_http_request()
* −−−−−−−−−−−−−−−
* Frees all memory associated with the given HttpRequest.
*
* httpRequest: HttpRequest struct holding the http request information. Not 
* NULL 
*/
void free_http_request(HttpRequest* httpRequest);

/* free_http_response()
* −−−−−−−−−−−−−−−
* Frees all memory associated with the given HttpResponse.
*
* httpResponse: HttpResponse struct holding the http response information. Not 
* NULL 
*/
void free_http_response(HttpResponse* httpResponse);

#endif

