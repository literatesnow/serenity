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

#include "sockets.h"

int ConnectSocket(int *sock, struct sockaddr_in *addr, int *state, double *last) {
  if (*state < SOCKETSTATE_FREE) {
    return 0;
  }

  if (*state > SOCKETSTATE_BEGINCONNECT) {
    return 0;
  }

  if (*state > SOCKETSTATE_DISCONNECTED) {
    struct timeval timeout;
    fd_set fdr;
    fd_set fdw;
    int r;

    FD_ZERO(&fdr);
    FD_SET(*sock, &fdr);
    FD_ZERO(&fdw);
    FD_SET(*sock, &fdw);

    timeout.tv_sec = 0;
    timeout.tv_usec = 1 * 50;

    r = select((*sock) + 1, &fdr, &fdw, NULL, &timeout);
#ifdef SUPERDEBUG
    //printf("select() %d %d %d\n", r, FD_ISSET(*sock, &fdr), FD_ISSET(*sock, &fdw));
#endif
    if (r > 0 && (FD_ISSET(*sock, &fdr) || FD_ISSET(*sock, &fdw))) {
      int opt;
      unsigned int optlen = sizeof(opt);

#ifdef WIN32
      r = getsockopt(*sock, SOL_SOCKET, SO_ERROR, (char *)&opt, &optlen);
#endif
#ifdef UNIX
      r = getsockopt(*sock, SOL_SOCKET, SO_ERROR, &opt, &optlen);
#endif
      if (!r) {
        if (!opt) {
          *state = SOCKETSTATE_SOCKCONNECTED;
#ifdef SUPERDEBUG
          printf("socket connected!\n");
#endif
          return 1;
        } else {
#ifdef SUPERDEBUG
          printf("getsockopt() ok but SO_ERROR, failing: %d %s\n", opt, strerror(opt));
#endif
          goto _fail;
        }
      } else {
#ifdef SUPERDEBUG
        printf("getsockopt() failed with: %d %s\n", errno, strerror(errno));
#endif
      }
    } else if (*last > 0.0 && (*last < DoubleTime())) { //timeout
#ifdef _DEBUG
      printf("failed to connect.\n");
#endif
_fail:
      *state = SOCKETSTATE_FAILEDCONNECT;
      *last = 0.0;
      _sleep(10);
    }

    return 0;
  }

  if (*sock == INVALID_SOCKET) {
#ifdef SUPERDEBUG
    //printf("dud socket\n");
#endif
    if (!InitTCPSocket(sock, NULL)) {
      CloseSocket(sock);
      *state = SOCKETSTATE_DISCONNECTED;
      *last = 0.0;
      return 0;
    }
  }

#ifdef _DEBUG
  printf("* Connecting to %s:%d...\n", inet_ntoa(addr->sin_addr), ntohs(addr->sin_port));
#endif
  if (connect(*sock, (struct sockaddr *)addr, sizeof(struct sockaddr)) == SOCKET_ERROR) {
#ifdef WIN32
#ifdef _DEBUG
    printf("connect(): [%d]\n", WSAGetLastError());
#endif
    switch (WSAGetLastError()) {
#endif
#ifdef UNIX
#ifdef _DEBUG
    printf("connect(): [%d] %s\n", errno, strerror(errno));
#endif
    switch (errno) {
#endif
      case EISCONN:
      case ENOTSOCK:
      case ECONNABORTED:
#ifdef UNIX
      case EBADF:
#endif
#ifdef SUPERDEBUG
        printf("closing socket and try again\n");
#endif
        CloseSocket(sock);
        *state = SOCKETSTATE_DISCONNECTED;
        *last = 0.0;
        return 0;

#ifdef WIN32
#ifdef _DEBUG
      case WSANOTINITIALISED:
        printf("LOL (init WSA)\n");
        return 0;
#endif
#endif

      case EALREADY:
      case EWOULDBLOCK:
      case EINPROGRESS:
#ifdef SUPERDEBUG
        printf("connection in progress\n");
#endif
        _sleep(10);
        break; //do not return here

      case ETIMEDOUT:
      case ECONNREFUSED:
      case EHOSTDOWN:
      case EHOSTUNREACH:
#ifdef WIN32
      case WSAEINVAL:
#endif
#ifdef SUPERDEBUG
        printf("connect failed (timeout,refuse,down,unreach)\n");
#endif
        goto _fail;

      default:
#ifdef _DEBUG
#ifdef UNIX
        printf("WARNING: connect() failed with: [%d] %s\n", errno, strerror(errno));
#endif
#ifdef WIN32
        printf("WARNING: connect() failed with: [%d]\n", WSAGetLastError());
#endif
#endif
        break;
    }
  }

#ifdef SUPERDEBUG
  printf("socket state set to begin\n");
#endif

  *state = SOCKETSTATE_BEGINCONNECT;
  *last = DoubleTime() + 5.0;

  return 0;
}

int InitTCPSocket(int *sock, struct sockaddr_in *addr)
{
  struct sockaddr_in bindaddr;
#ifdef WIN32
  unsigned long _true = 1;
#endif
#ifdef UNIX
  int flags;
#endif

#ifdef SUPERDEBUG
  printf("InitTCPSocket()\n");
#endif

  CloseSocket(sock);

#ifdef WIN32
  if ((*sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
#endif
#ifdef UNIX
  if ((*sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
#endif
  {
#ifdef _DEBUG
    printf("Error: Invalid TCP socket\n");
#endif
    return 0;
  }

#ifdef WIN32
  if (ioctlsocket(*sock, FIONBIO, &_true) == -1)
  {
#ifdef _DEBUG
    printf("Error: TCP ioctlsocket() failed\n");
#endif
    return 0;
  }
#endif

#ifdef UNIX
  if ((flags = fcntl(*sock, F_GETFL, 0)) < 0) 
  {
#ifdef _DEBUG
    printf("Error: TCP get fcntl() failed\n");
#endif
    return 0;
  } 

  if (fcntl(*sock, F_SETFL, flags | O_NONBLOCK) < 0) 
  {
#ifdef _DEBUG
    printf("Error: TCP set ioctlsocket() failed\n");
#endif
    return 0;
  } 
#endif

  if (!addr) {
    memset(&(bindaddr.sin_zero), '\0', sizeof(bindaddr.sin_zero));
    bindaddr.sin_family = AF_INET;
    bindaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    bindaddr.sin_port = htons(0);
    addr = &bindaddr;
  }

#ifdef WIN32
  if (bind(*sock, (struct sockaddr *)addr, sizeof(struct sockaddr)) == SOCKET_ERROR)
#endif
#ifdef UNIX
  if (bind(*sock, (struct sockaddr *)addr, sizeof(struct sockaddr)) == -1)
#endif
  {
#ifdef _DEBUG
    printf("Error: Couldn\'t bind TCP socket\n");
#endif
    return 0;
  }

  return 1;
}

int InitUDPSocket(int *sock, struct sockaddr_in *addr)
{
  struct sockaddr_in bindaddr;
#ifdef WIN32
  unsigned long _true = 1;
#endif
#ifdef UNIX
  int flags;
#endif

#ifdef SUPERDEBUG
  printf("InitUDPSocket()\n");
#endif

  CloseSocket(sock);

#ifdef WIN32
  if ((*sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET)
#endif
#ifdef UNIX
  if ((*sock = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
#endif
  {
#ifdef _DEBUG
    printf("Error: Invalid UDP socket\n");
#endif
    return 0;
  }

#ifdef WIN32
  if (ioctlsocket(*sock, FIONBIO, &_true) == -1)
  {
#ifdef _DEBUG
    printf("Error: UDP ioctlsocket() failed\n");
#endif
    return 0;
  }
#endif

#ifdef UNIX
  if ((flags = fcntl(*sock, F_GETFL, 0)) < 0) 
  {
#ifdef _DEBUG
    printf("Error: TCP get fcntl() failed\n");
#endif
    return 0;
  } 

  if (fcntl(*sock, F_SETFL, flags | O_NONBLOCK) < 0) 
  {
#ifdef _DEBUG
    printf("Error: TCP set ioctlsocket() failed\n");
#endif
    return 0;
  } 
#endif

  if (!addr) {
    memset(&(bindaddr.sin_zero), '\0', sizeof(bindaddr.sin_zero));
    bindaddr.sin_family = AF_INET;
    bindaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    bindaddr.sin_port = htons(0);
    addr = &bindaddr;
  }

#ifdef WIN32
  if (bind(*sock, (struct sockaddr *)addr, sizeof(struct sockaddr)) == SOCKET_ERROR)
#endif
#ifdef UNIX
  if (bind(*sock, (struct sockaddr *)addr, sizeof(struct sockaddr)) == -1)
#endif
  {
#ifdef _DEBUG
    printf("Error: Couldn\'t bind UDP socket\n");
    printf("err?: [%d] %s\n", errno, strerror(errno));
#endif
    return 0;
  }

#if 0
  {
    int sz = 98304;
    setsockopt(*sock, SOL_SOCKET, SO_RCVBUF, (char *)&sz, sizeof(sz));
  }
#endif

  return 1;
}

void CloseSocket(int *sock)
{
  //shutdown(*sock, 2); //SD_BOTH
  if (*sock > 0) {
    _close(*sock);
  }
  *sock = INVALID_SOCKET;
}

int SocketWait(int socket, long ms)
{
  struct timeval timeout;
  fd_set fd1;

#ifdef SUPERDEBUG
  //superdebug("SocketWait()\n");
#endif

  if (socket == INVALID_SOCKET) {
    return 0;
  }

  FD_ZERO(&fd1);
  FD_SET(socket, &fd1);

  timeout.tv_sec = 0;
  timeout.tv_usec = ms * 1000;

  return select(socket + 1, &fd1, NULL, NULL, &timeout);
}



