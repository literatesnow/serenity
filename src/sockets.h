/*
  Copyright (C) 2004-2008 Chris Cuthbertson

  This file is part of Serenity.

  Serenity is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Serenity is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Serenity.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _H_SOCKETS
#define _H_SOCKETS

#define _WIN32_WINNT 0x0500

#ifdef WIN32
#include <windows.h>
#endif
#ifdef UNIX
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#endif

#include "util.h"

#define SOCKETSTATE_FAILEDCONNECT -1
#define SOCKETSTATE_FREE 0
#define SOCKETSTATE_DISCONNECTED 1
#define SOCKETSTATE_BEGINCONNECT 2
#define SOCKETSTATE_SOCKCONNECTED 3
#define SOCKETSTATE_CONNECTED 4

#ifdef WIN32
#define _close(s) closesocket(s)
#endif
#ifdef UNIX
#define _close(s) close(s)
#endif

#ifdef WIN32
#define ECONNREFUSED WSAECONNREFUSED
#define EINPROGRESS WSAEINPROGRESS
#define EWOULDBLOCK WSAEWOULDBLOCK
#define ENOTSOCK WSAENOTSOCK
#define EISCONN WSAEISCONN
#define EALREADY WSAEALREADY
#define ETIMEDOUT WSAETIMEDOUT
#define EHOSTDOWN WSAEHOSTDOWN
#define EHOSTUNREACH WSAEHOSTUNREACH
#define ECONNABORTED WSAECONNABORTED
#endif

#ifdef UNIX
#define SOCKET_ERROR -1
#define INVALID_SOCKET -1
#endif

int ConnectSocket(int *sock, struct sockaddr_in *addr, int *state, double *last);

int InitTCPSocket(int *sock, struct sockaddr_in *addr);
int InitUDPSocket(int *sock, struct sockaddr_in *addr);
void CloseSocket(int *sock);

int SocketWait(int socket, long ms);

#endif //_H_SOCKETS

