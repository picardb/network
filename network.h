#ifndef _NETWORK_H
#define _NETWORK_H

#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>

/***************************************************************************************/
/*	Global variables declarations																			*/
/***************************************************************************************/
extern fd_set	g_socketsSet;	// Set containing all the opened sockets file descriptors

/***************************************************************************************/
/*	Functions declarations																					*/
/***************************************************************************************/
bool NetworkInit();		// Initializes the Windows Winsock library
void NetworkCleanup();	// Performs a cleanup of the resources used by the network library
int NetworkOpenListenerSocket(int *pSocket, char *portNumberStr, int backlog);	// Opens a socket in listening state
int NetworkOpenConnectingSocket(int *pSocket, char *addressStr, char *portNumberStr);	// Opens a socket and connects to a server
int NetworkOpenClientSocket(int listenerSocket, int *pClientSocket = NULL);	// Accepts a new connection from a client and adds the new socket to the sockets set
int NetworkPollSockets(fd_set *pReadSockets, int timeout = 0);	// Polls all the open sockets to find sockets that have data to read from
void NetworkCloseSocket(int socket);	// Closes a socket and removes its file descriptor from the master set
int NetworkReadFromSocket(int socket, char *pBuffer, int bufferLength);	// Reads data from a socket
int NetworkSendToSocket(int socket, char *pBuffer, int bufferLength);	// Sends data to a socket
std::string NetworkGetPeerAddress(int socket);	// Gets the address of a peer from a socket file descriptor

#endif