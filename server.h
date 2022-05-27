#pragma once
#include <winsock2.h>

const int TIME_PORT = 13711;
const int MAX_SOCKETS = 60;
const int MAX_SIZE_BUFF = 1024;
const int EMPTY = 0;
const int LISTEN = 1;
const int RECEIVE = 2;
const int IDLE = 3;
const int SEND = 4;
const int SEND_TIME = 1;
const int SEND_SECONDS = 2;


typedef struct SocketState
{
	SOCKET id;			// Socket handle
	int	recv;			// Receiving?
	int	send;			// Sending?
	int sendSubType;	// Sending sub-type
	char buffer[MAX_SIZE_BUFF];
	int len;
}SocketState;


SocketState sockets[MAX_SOCKETS] = { 0 };
int socketsCount = 0;

bool addSocket(SOCKET id, int what);
void removeSocket(int index);
void acceptConnection(int index);
void receiveMessage(int index);
void sendMessage(int index);

enum requestType {
	OPTIONS = 0, GET, HEAD, PUT, POST, DELETE_FILE, TRACE, OTHER
};

typedef string(*responseFunction) (SocketState* socket);

const int SIZE_OF_BUFFER_ARRAY = 1024;

//arrays for determine the type of methods requested.
const int amountOfHttpMethods = 7;
const char* httpMethods[] = { "OPTIONS", "GET", "HEAD", "PUT", "POST", "DELETE", "TRACE" };
const int httpMethodsLength[] = { 7, 3, 4, 3, 4, 6, 5 };

void handleRequest(SocketState* socket);
string prepareResponse(SocketState* socket);
string optionsMethod(SocketState* socket);
string getOrHeadMethod(SocketState* socket);
string putOrPostMethod(SocketState* socket);
string deleteMethod(SocketState* socket);
string traceMethod(SocketState* socket);

string generateResponseHeaders(const char* status, int content_length);
string uriExtractor(SocketState* socket);
int putOrPostFile(SocketState* socket, string& filename);
bool checkFileExists(string& fname);
void copyFileContent2String(ifstream& inFile, string& str);

const responseFunction responseFunctionsArr[amountOfHttpMethods] = {
	optionsMethod ,
	getOrHeadMethod ,
	getOrHeadMethod ,
	putOrPostMethod ,
	putOrPostMethod ,
	deleteMethod ,
	traceMethod
};