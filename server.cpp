#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream>
using namespace std;
#pragma comment(lib, "Ws2_32.lib")
#include <winsock2.h>
#include <string>
#include <time.h>
#include <fstream>
#include "server.h"

int main()
{
	WSAData wsaData;

	if (NO_ERROR != WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		cout << "Server: Error at WSAStartup()\n";
		return 0;
	}

	SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (INVALID_SOCKET == listenSocket)
	{
		cout << "Server: Error at socket(): " << WSAGetLastError() << endl;
		WSACleanup();
		return 0;
	}

	sockaddr_in serverService;
	serverService.sin_family = AF_INET;
	serverService.sin_addr.s_addr = INADDR_ANY;
	serverService.sin_port = htons(TIME_PORT);

	if (SOCKET_ERROR == bind(listenSocket, (SOCKADDR*)&serverService, sizeof(serverService)))
	{
		cout << "Server: Error at bind(): " << WSAGetLastError() << endl;
		closesocket(listenSocket);
		WSACleanup();
		return 0;
	}

	if (SOCKET_ERROR == listen(listenSocket, 5))
	{
		cout << "Server: Error at listen(): " << WSAGetLastError() << endl;
		closesocket(listenSocket);
		WSACleanup();
		return 0;
	}
	addSocket(listenSocket, LISTEN);

	while (true)
	{
		fd_set waitRecv;
		FD_ZERO(&waitRecv);
		for (int i = 0; i < MAX_SOCKETS; i++)
		{
			if ((sockets[i].recv == LISTEN) || (sockets[i].recv == RECEIVE))
				FD_SET(sockets[i].id, &waitRecv);
		}

		fd_set waitSend;
		FD_ZERO(&waitSend);
		for (int i = 0; i < MAX_SOCKETS; i++)
		{
			if (sockets[i].send == SEND)
				FD_SET(sockets[i].id, &waitSend);
		}

		int nfd;

		nfd = select(0, &waitRecv, &waitSend, NULL, NULL);

		if (nfd == SOCKET_ERROR)
		{
			cout << "Server: Error at select(): " << WSAGetLastError() << endl;
			WSACleanup();
			return 0;
		}

		for (int i = 0; i < MAX_SOCKETS && nfd > 0; i++)
		{
			if (FD_ISSET(sockets[i].id, &waitRecv))
			{
				nfd--;
				switch (sockets[i].recv)
				{
				case LISTEN:
					acceptConnection(i);
					break;

				case RECEIVE:
					receiveMessage(i);
					break;
				}
			}
		}

		for (int i = 0; i < MAX_SOCKETS && nfd > 0; i++)
		{
			if (FD_ISSET(sockets[i].id, &waitSend))
			{
				nfd--;
				switch (sockets[i].send)
				{
				case SEND:
					sendMessage(i);
					break;
				}
			}
		}
	}

	// Closing connections and Winsock.
	cout << "Server: Closing Connection.\n";
	closesocket(listenSocket);
	WSACleanup();
	return 0;
}


void removeSocket(int index)
{
	sockets[index].recv = EMPTY;
	sockets[index].send = EMPTY;
	socketsCount--;
}

void acceptConnection(int index)
{
	SOCKET id = sockets[index].id;
	struct sockaddr_in from;		// Address of sending partner
	int fromLen = sizeof(from);

	SOCKET msgSocket = accept(id, (struct sockaddr*)&from, &fromLen);
	if (INVALID_SOCKET == msgSocket)
	{
		cout << "Server: Error at accept(): " << WSAGetLastError() << endl;
		return;
	}
	cout << "Server: Client " << inet_ntoa(from.sin_addr) << ":" << ntohs(from.sin_port) << " is connected." << endl;

	//
	// Set the socket to be in non-blocking mode.
	//
	unsigned long flag = 1;
	if (ioctlsocket(msgSocket, FIONBIO, &flag) != 0)
	{
		cout << "Server: Error at ioctlsocket(): " << WSAGetLastError() << endl;
	}

	if (addSocket(msgSocket, RECEIVE) == false)
	{
		cout << "\t\tToo many connections, dropped!\n";
		closesocket(id);
	}
	return;
}

bool addSocket(SOCKET id, int what)
{
	for (int i = 0; i < MAX_SOCKETS; i++)
	{
		if (sockets[i].recv == EMPTY)
		{
			sockets[i].id = id;
			sockets[i].recv = what;
			sockets[i].send = IDLE;
			sockets[i].len = 0;
			socketsCount++;
			return (true);
		}
	}
	return (false);
}

void receiveMessage(int index)
{
	SOCKET msgSocket = sockets[index].id;

	int len = sockets[index].len;
	int bytesRecv = recv(msgSocket, &sockets[index].buffer[len], sizeof(sockets[index].buffer) - len, 0);

	if (SOCKET_ERROR == bytesRecv)
	{
		cout << "Server: Error at recv(): " << WSAGetLastError() << endl;
		closesocket(msgSocket);
		removeSocket(index);
		return;
	}
	if (bytesRecv == 0)
	{
		closesocket(msgSocket);
		removeSocket(index);
		return;
	}
	else
	{
		sockets[index].buffer[len + bytesRecv] = '\0'; //add the null-terminating to make it a string
		cout << "Server: Recieved: " << bytesRecv << " bytes of: \n" << &sockets[index].buffer[len] << endl;

		sockets[index].len += bytesRecv;

		if (sockets[index].len > 0) {
			handleRequest(&(sockets[index]));
		}
	}
}

void sendMessage(int index)
{
	int bytesSent = 0;
	char sendBuff[MAX_SIZE_BUFF] = { 0 };

	SOCKET msgSocket = sockets[index].id;
	string respond = prepareResponse(&(sockets[index]));
	sockets[index].len = 0;
	sprintf(sendBuff, "%s", respond.c_str());
	cout << "strlen: " << (int)strlen(sendBuff) << endl;
	cout << "respond.length: " << respond.length() << endl;
	bytesSent = send(msgSocket, sendBuff, (int)strlen(sendBuff), 0);

	if (SOCKET_ERROR == bytesSent) {
		cout << "Server: Error at send(): " << WSAGetLastError() << endl;
		return;
	}

	cout << "Server: Sent: " << bytesSent << "\\" << strlen(sendBuff) << " bytes of: \n" << sendBuff << "\n============\n";
	sockets[index].send = IDLE;
}


// =================================================

void handleRequest(SocketState* socket) {
	socket->send = SEND;
	socket->sendSubType = OTHER;
	for (int i = 0; i < amountOfHttpMethods; i++) {
		if (strncmp(socket->buffer, httpMethods[i], httpMethodsLength[i]) == 0) {
			socket->sendSubType = i;
			return;
		}
	}

}

string prepareResponse(SocketState* socket) {
	string response;
	if (socket->sendSubType == OTHER) {
		response = generateResponseHeaders("400 Bad Request", 0);
	}
	else {
		response = responseFunctionsArr[socket->sendSubType](socket);
	}
	return response;
}

string optionsMethod(SocketState* socket) {
	time_t timeRightNow;
	string header("HTTP/1.1 ");
	header.append("200 OK");
	header.append("\r\n");
	header.append("The date: ");
	time(&timeRightNow);
	header.append(ctime(&timeRightNow));
	header.append("Methods that the server supports are: GET, PUT, POST, HEAD, OPTIONS, DELETE, TRACE\r\n");
	header.append("Content-Length: 0\r\n");
	header.append("Content-Type: text/html\r\n");
	header.append("\r\n");
	return header;
}

string getOrHeadMethod(SocketState* socket) {
	string response;
	ifstream inFile;
	string uri = uriExtractor(socket);
	inFile.open(uri);
	if (inFile.fail()){
		cout << "Web Server failed to open the file " << uri << "Error code: " << WSAGetLastError() << endl;
		response = generateResponseHeaders("404 Not Found", 0);
	}
	else {
		string fileContent = "";
		copyFileContent2String(inFile, fileContent);
		response = generateResponseHeaders("200 OK", fileContent.length());
		if (socket->sendSubType == GET) { // on GET => append the content.
			response.append(fileContent);
		} 
	}
	inFile.close();
	return response;
}

string putOrPostMethod(SocketState* socket) {
	string response;
	string uri = uriExtractor(socket);
	int retCode = putOrPostFile(socket, uri);
	switch (retCode)
	{
	case 200:
		response = generateResponseHeaders("200 OK", 0);
		break;
	case 201:
		response = generateResponseHeaders("201 Created", 0);
		break;
	case 204:
		response = generateResponseHeaders("204 No Content", 0);
		break;
	default:
		cout << (socket->sendSubType == PUT ? "PUT " : "POST ") << uri << "Failed";
		response = generateResponseHeaders("500 Internal Server Error", 0);
		break;
	}

	return response;
}

string deleteMethod(SocketState* socket) {
	string response;
	fstream fileToDelete;
	string uri = uriExtractor(socket);
	fileToDelete.open(uri);
	if (fileToDelete.is_open())	{
		fileToDelete.close();
		if (remove(uri.c_str()) == 0) {
			response = generateResponseHeaders("200 OK", 0);
		}
		else {
			response = generateResponseHeaders("500 Internal Server Error", 0);
		}
	}
	else {
		response = generateResponseHeaders("404 Not Found", 0);
	}
	return response;
}

string traceMethod(SocketState* socket) {
	string response;
	string bodyContent(socket->buffer);
	response = generateResponseHeaders("200 OK", bodyContent.length());
	response.append(bodyContent);
	return response;
}

string generateResponseHeaders(const char* status, int content_length) {
	time_t timeRightNow;
	string header("HTTP/1.1 ");
	header.append(status);
	header.append("\r\n");
	header.append("The date: ");
	time(&timeRightNow);
	header.append(ctime(&timeRightNow));
	header.append("Content-Length: ");
	header += std::to_string(content_length);
	header.append("\r\n");
	header.append("Content-Type: text/html\r\n");
	header.append("\r\n");
	return header;
}

string uriExtractor(SocketState* socket)
{
	int index_start_uri = ((string)(socket->buffer)).find(" ", 0) + 1;
	int index_end_uri = ((string)(socket->buffer)).find(" ", index_start_uri);
	string uri = ((string)(socket->buffer)).substr(index_start_uri, index_end_uri - index_start_uri);
	if (uri.length() > 1 && uri[0] == '/')
	{
		uri = uri.substr(1, uri.length());
	}
	return uri;
}

int putOrPostFile(SocketState* socket, string& filename) {
	int retCode = 200; // Ok	
	ofstream outPutFile;

	if (!checkFileExists(filename)) {
		retCode = 201; // Created
	}
	if (socket->sendSubType == PUT) {
		outPutFile.open(filename); // Truncation
	}
	else { // == POST
		outPutFile.open(filename, std::fstream::in | std::fstream::out | std::fstream::app); // Append
	}

	if (!outPutFile.is_open())	{
		cout << "HTTP/text Server: Error writing file to local storage: " << WSAGetLastError() << endl;
		return 0; // Error
	}

	string bodyContent = ((string)(socket->buffer)).substr(((string)(socket->buffer)).find("\r\n\r\n") + 4, socket->len);

	if (bodyContent.length() == 0) {
		retCode = 204; // No content
	}
	else {
		outPutFile << bodyContent;
	}

	outPutFile.close();
	return retCode;
}

bool checkFileExists(string& fname) {
	ifstream ifile;
	ifile.open(fname);
	if (ifile) {
		ifile.close();
		return true;
	}
	else {
		ifile.close();
		return false;
	}
}

void copyFileContent2String(ifstream& inFile, string& str) {
	if (inFile.is_open())	{
		char currentChar = inFile.get();
		while (currentChar != EOF)		{
			str.push_back(currentChar);
			currentChar = inFile.get();
		}
	}
}
