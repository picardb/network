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
/*		File:			network.h																	*/
/* 	Version:		v1.02																			*/
/*		Author:		Benoit Picard																*/
/******************************************************************************/
/* Edit history:																					*/
/*																										*/
/*	Date			Ver.	Auth.	Description														*/
/*	--------------------------------------------------------------------------	*/
/*	2015/08/07	v1.00	BPD	Creation															*/
/* 2015/08/13	v1.01 BPD	Renamed NetPollSockets to NetPollReadSockets			*/
/*									Added NetPollWriteSockets function declaration		*/
/* 2015/08/21	v1.02 BPD	Timeout added to NetReadFromSocket and					*/
/*										NetWriteToSocket											*/
/******************************************************************************/

#ifndef _NETWORK_H
#define _NETWORK_H

#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>

/***************************************************************************************/
/*	Global variables declarations																			*/
/***************************************************************************************/
extern fd_set	g_netSocketsSet;	// Set containing all the opened sockets file descriptors

/***************************************************************************************/
/*	Functions declarations																					*/
/***************************************************************************************/
bool NetInit();		// Initializes the Windows Winsock library
void NetCleanup();	// Performs a cleanup of the resources used by the network library
int NetOpenListenerSocket(int *pSocket, char *portNumberStr, int backlog);	// Opens a socket in listening state
int NetOpenConnectingSocket(int *pSocket, char *addressStr, char *portNumberStr);	// Opens a socket and connects to a server
int NetOpenClientSocket(int listenerSocket, int *pClientSocket = NULL);	// Accepts a new connection from a client and adds the new socket to the sockets set
int NetPollReadableSockets(fd_set *pReadSockets, int timeout = 0);	// Polls all the open sockets to find sockets that have data to read from
int NetPollWritableSockets(fd_set *pWriteSockets, int timeout = 0);	// Polls all the open sockets to find sockets that can be written to
void NetCloseSocket(int socket);	// Closes a socket and removes its file descriptor from the master set
int NetReadFromSocket(int socket, char *pBuffer, int size, int timeout);	// Reads data from a socket
int NetSendToSocket(int socket, char *pBuffer, int size, int timeout);		// Sends data to a socket
std::string NetGetPeerAddress(int socket);	// Gets the address of a peer from a socket file descriptor

#endif