/******************************************************************************/
// SVN
// $Revision: 1068 $
// $Date: 2015-08-21 12:03:48 +0200 (ven., 21 août 2015) $
// $Author: picard $
/******************************************************************************/
/*		Project:		WISDOM																		*/
/*		Unit:			network library															*/
/*		Company:		LATMOS																		*/
/******************************************************************************/
/*		File:			network.cpp																	*/
/* 	Version:		v1.02																			*/
/*		Author:		Benoit Picard																*/
/******************************************************************************/
/**
 * @file
 * @brief Implementation of the network library.
 *
 * This file contains the definitions of all functions of the network library.
 * The library uses a global file descriptor set "g_socketsSet" to keep track
 * of all open sockets.
 * @defgroup libmng Library Management
 * @defgroup sockcreation Socket Creation
 * @defgroup sockdestruction Socket Destruction
 * @defgroup socketuse Socket Use
 */
/******************************************************************************/
/* Edit history:																					*/
/*																										*/
/*	Date			Ver.	Auth.	Description														*/
/*	--------------------------------------------------------------------------	*/
/*	2015/08/07	v1.00	BPD	Creation															*/
/* 2015/08/13	v1.01 BPD	Renamed NetPollSockets to NetPollReadableSockets	*/
/*									Added NetPollWritableSockets function definition	*/
/* 								Changed client sockets to nonblocking mode			*/
/*									Make the NetReadFromSocket read all available data	*/
/*										from a socket												*/
/* 2015/08/21	v1.02 BPD	Timeout added to NetReadFromSocket and					*/
/*										NetWriteToSocket											*/
/******************************************************************************/

#include "network.h"

using namespace std;

/***************************************************************************************/
/*	Global variables definitions																			*/
/***************************************************************************************/
/**
 * @brief Sockets master set
 *
 * Stores the file descriptors of all open sockets
 * @ingroup libmng
 */
fd_set	g_netSocketsSet;

/***************************************************************************************/
/* NetInit																										*/
/***************************************************************************************/
/** 
 * @brief Initializes the Winsock library.
 * 
 * This function calls the Winsock WSAStartup function to initiate the use of the 
 * Winsock DLL by the program.
 * @return true if the library was initialized successfully, false otherwise
 * @ingroup libmng
 */
bool NetInit() {
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0) {
		return false;
	}

	return true;
}

/***************************************************************************************/
/* NetCleanup																									*/
/***************************************************************************************/
/**
 * @brief Performs a cleanup of the resources.
 * 
 * This function releases all used resources by unloading the Winsock library and
 * closing all open sockets.
 * @ingroup libmng
 */
void NetCleanup() {
	for (unsigned int i = 0 ; i < g_netSocketsSet.fd_count ; i++) {
		NetCloseSocket(g_netSocketsSet.fd_array[i]);
	}
	WSACleanup();
}

/***************************************************************************************/
/* NetOpenListenerSocket																					*/
/***************************************************************************************/
/** 
 * @brief Opens a socket in listening state.
 * 
 * This function opens a new socket and starts listening on that socket. The socket is
 * added to the master socket set. A listener socket will be usually used for a server
 * application.
 * @param pSocket pointer to the new socket file descriptor
 * @param portNumberStr c-type string containing the port number to be used by the 
 *		service
 * @param backlog size of the socket incoming queue
 * @return 0 if the operation was successful, a Winsock error number otherwise
 * @ingroup sockcreation
 */
int NetOpenListenerSocket(int *pSocket, char *portNumberStr, int backlog) {
	/* Get local address info */
	struct addrinfo *pInfo;
	struct addrinfo hints;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if (getaddrinfo(NULL, portNumberStr, &hints, &pInfo) != 0) {
		return WSAGetLastError();
	}

	/* Open socket */
	*pSocket = socket(pInfo->ai_family, pInfo->ai_socktype, pInfo->ai_protocol);
	if (*pSocket == INVALID_SOCKET) {
		return WSAGetLastError();
	}

	/* Bind socket */
	if (bind(*pSocket, pInfo->ai_addr, pInfo->ai_addrlen) == SOCKET_ERROR) {
		return WSAGetLastError();
	}

	/* Start listening on socket */
	if (listen(*pSocket, backlog) == SOCKET_ERROR) {
		return WSAGetLastError();
	}

	/* Add socket to the set */
	FD_SET(*pSocket, &g_netSocketsSet);

	return 0;
}

/***************************************************************************************/
/* NetOpenConnectingSocket																					*/
/***************************************************************************************/
/**
 * @brief Opens a socket and connects to a server.
 * 
 * This function opens a new socket and calls connect with the provided address and port 
 * number. The socket is added to the master socket set. A connecting socket will 
 * typically be used in a client application.
 * @param pSocket pointer to the new socket file descriptor
 * @param addressStr c-type string containing the address of the server
 * @param portNumberStr c-type string containing the port number of the service
 * @return 0 if the operation was successful, a Winsock error number otherwise
 * @ingroup sockcreation
 */
int NetOpenConnectingSocket(int *pSocket, char *addressStr, char *portNumberStr) {
	/* Get server address info */
	struct addrinfo *pInfo;
	struct addrinfo hints;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	if (getaddrinfo(addressStr, portNumberStr, &hints, &pInfo) != 0) {
		return WSAGetLastError();
	}

	/* Open socket */
	*pSocket = socket(pInfo->ai_family, pInfo->ai_socktype, pInfo->ai_protocol);
	if (*pSocket == INVALID_SOCKET) {
		return WSAGetLastError();
	}

	/* Connect */
	if (connect(*pSocket, pInfo->ai_addr, pInfo->ai_addrlen) == SOCKET_ERROR) {
		return WSAGetLastError();
	}
	
	/* Add socket to the set */
	FD_SET(*pSocket, &g_netSocketsSet);

	return 0;
}

/***************************************************************************************/
/* NetOpenClientSocket																						*/
/***************************************************************************************/
/**
 * @brief Accepts a new connection from a client.
 * 
 * This function opens a new socket by accepting a connection on a listener socket. The
 * socket is then added to the master sockets set. If the calling function doesn't need
 * the new socket file descriptor, the pClientSocket parameter can be omitted.
 * @param listenerSocket file descriptor of the listener socket
 * @param pClientSocket optional pointer to the new client socket file descriptor
 * @return 0 if the operation was successful, a Winsock error number otherwise
 * @ingroup sockcreation
 */
int NetOpenClientSocket(int listenerSocket, int *pClientSocket) {
	/* Accept connection */
	int newSocket = accept(listenerSocket, NULL, NULL);

	/* Check for error */
	if (newSocket == SOCKET_ERROR) {
		return WSAGetLastError();
	}

	/* Set socket to non-blocking mode */
	unsigned long enableNonBlocking = 1;
	if (ioctlsocket(newSocket, FIONBIO, &enableNonBlocking) == SOCKET_ERROR) {
		return WSAGetLastError();
	}

	/* Add the new socket to the master sockets set */
	FD_SET(newSocket, &g_netSocketsSet);
	if (pClientSocket != NULL) {
		*pClientSocket = newSocket;
	}

	return 0;
}

/***************************************************************************************/
/* NetPollReadableSockets																					*/
/***************************************************************************************/
/**
 * @brief Polls all open sockets for read.
 * 
 * This function polls all the sockets of the master sockets set and fill the 
 * pReadSockets argument with all sockets that have data to read from.
 * @param pReadSockets pointer to the sockets set of "readable" sockets
 * @param timeout optional timeout of the polling operations in milliseconds. Default 
 * value is 0, meaning that the function will block until there is a socket to read from
 * @return 0 if the operation was successful, a Winsock error number otherwise
 * @ingroup socketuse
 */
int NetPollReadableSockets(fd_set *pReadSockets, int timeout) {
	/* Copy master set in the "read" set */
	*pReadSockets = g_netSocketsSet;

	/* Call select function */
	int status;
	if (timeout > 0) {
		/* Call select with a timeout */
		struct timeval tv;
		tv.tv_sec = 0;
		tv.tv_usec = timeout * 1000;
		status = select(0, pReadSockets, NULL, NULL, &tv);
	}
	else {
		/* Call select without a timeout (blocking) */
		status = select(0, pReadSockets, NULL, NULL, NULL);
	}

	/* Check select return value */
	if (status == SOCKET_ERROR) {
		return WSAGetLastError();
	}
	else {
		return 0;
	}
}

/***************************************************************************************/
/* NetPollWritableSockets																					*/
/***************************************************************************************/
/**
 * @brief Polls all open sockets for write.
 * 
 * This function polls all the sockets of the master sockets set and fill the 
 * pWriteSockets argument with all sockets that can be written to.
 * @param pWriteSockets pointer to the sockets set of "writable" sockets
 * @param timeout optional timeout of the polling operations in microseconds. Default 
 * value is 0, meaning that the function will poll all sockets and return immediately
 * @return 0 if the operation was successful, a Winsock error number otherwise
 * @ingroup socketuse
 */
int NetPollWritableSockets(fd_set *pWriteSockets, int timeout) {
	/* Copy master set in the "write" set */
	*pWriteSockets = g_netSocketsSet;

	/* Call select function */
	int status;
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = timeout;
	status = select(0, NULL, pWriteSockets, NULL, &tv);

	/* Check select return value */
	if (status == SOCKET_ERROR) {
		return WSAGetLastError();
	}
	else {
		return 0;
	}
}

/***************************************************************************************/
/* NetCloseSocket																								*/
/***************************************************************************************/
/**
 * @brief Closes a socket.
 * 
 * This function closes an open socket and removes it from the master set.
 * @param socket socket file descriptor
 * @ingroup sockdestruction
 */
void NetCloseSocket(int socket) {
	closesocket(socket);
	FD_CLR(socket, &g_netSocketsSet);
}

/***************************************************************************************/
/* NetReadFromSocket																							*/
/***************************************************************************************/
/**
 * @brief Reads data from a socket.
 * 
 * This function calls the recv function to retrieve data from a socket. The user should
 * make sure that the socket has data available to read by polling the sockets first.
 * @param socket socket to read data from
 * @param pBuffer pointer to the buffer to store the data
 * @param size the number of bytes to read
 * @param timeout timeout on the read operation in milliseconds
 * @return the number of read bytes. 0 if the connection has been closed on the other 
 * side and SOCKET_ERROR if an error occured
 * @ingroup socketuse
 */
int NetReadFromSocket(int socket, char *pBuffer, int size, int timeout) {
	int totalReadBytes = 0;

	DWORD end = GetTickCount() + timeout;
	while (totalReadBytes < size) {
		int read = recv(socket, pBuffer + totalReadBytes, size - totalReadBytes, 0);
		if (read == 0) { // Connection closed by the client
			return 0;
		}
		else if (read == SOCKET_ERROR) {
			if (WSAGetLastError() == WSAEWOULDBLOCK) { // No more data to read
				if (GetTickCount() > end) {
					return totalReadBytes;
				}
			}
			else { // Socket error
				return SOCKET_ERROR;
			}
		}
		else { // Data read
			totalReadBytes += read;
			end = GetTickCount() + timeout;
		}	
	}

	return totalReadBytes;
}

/***************************************************************************************/
/* NetSendToSocket																							*/
/***************************************************************************************/
/**
 * @brief Sends data to a socket.
 * 
 * This function calls the send function to send data to a socket.
 * @param socket socket to send data to
 * @param pBuffer pointer to the buffer containing the data
 * @param size the number of bytes to send
 * @param timeout timeout on the send operation in milliseconds
 * @return the number of sent bytes. SOCKET_ERROR if an error occured
 * @ingroup socketuse
 */
int NetSendToSocket(int socket, char *pBuffer, int size, int timeout) {
	int totalSentBytes = 0;

	DWORD end = GetTickCount() + timeout;
	while (totalSentBytes < size) {
		int sent = send(socket, pBuffer + totalSentBytes, size - totalSentBytes, 0);
		if (sent == SOCKET_ERROR) {
			if (WSAGetLastError() == WSAEWOULDBLOCK) { // Cannot send right now
				if (GetTickCount() > end) {
					return totalSentBytes;
				}
			}
			else { // Socket error
				return SOCKET_ERROR;
			}
		}
		else { // Data sent
			totalSentBytes += sent;
			end = GetTickCount() + timeout;
		}	
	}

	return totalSentBytes;
}

/***************************************************************************************/
/* NetGetPeerAddress																							*/
/***************************************************************************************/
/**
 * @brief Gets the address of a peer.
 * 
 * This function gets the address of a peer from its socket file descriptor. The address
 * is returned as a C++ std::string.
 * @param socket socket file descriptor
 * @return address of the peer as a string
 * @ingroup socketuse
 */
string NetGetPeerAddress(int socket) {
	struct sockaddr_in clientAddr;
	socklen_t clientAddrSize = sizeof clientAddr;
	char clientAddrCStr[INET_ADDRSTRLEN];

	getpeername(socket, (struct sockaddr *)&clientAddr, &clientAddrSize);

#ifdef _WINXP
	strcpy_s(clientAddrCStr, INET_ADDRSTRLEN, inet_ntoa(clientAddr.sin_addr));	// Windows XP
#else
	inet_ntop(clientAddr.sin_family, (void *)(&clientAddr.sin_addr), clientAddrCStr, INET_ADDRSTRLEN); // Windows Vista and superior
#endif

	string clientAddrStr(clientAddrCStr);

	return clientAddrStr;
}