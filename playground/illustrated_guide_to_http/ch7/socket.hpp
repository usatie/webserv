#ifndef _SOCKET_HPP_
#define _SOCKET_HPP_

#include <string>
#include <iostream>
#include <fstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/select.h>

// ----------------------------------------------------------------------------

// Define this to the size of the largest ascii line of data your application
// expects to receive.
#define MAX_SOCK_BUFFER 16384
#define NO_EOL 1
#define REUSE_PORT 1

class Socket
{
public:
    Socket();
    ~Socket();

    // Server initialization
    int Create();                           // Allocate a socket for use
    int Passive(short int sPort);           // Turn the socket into a passive socket.
                                            // Do not set SO_REUSEADDR.
    int Passive(short int sPort,
                  int iReuse);              // Turn the socket into a passive socket.
                                            // Allow setting of SO_REUSEADDR.
    Socket  *Accept();                      // Listen for connections.
    int Connect(char *szBuf, short sPort);  // Connect the socket to the remote
                                            // host.
    int Recv();                             // Receive bytes on this socket.
    int Recv(int iBytes);                   // Receive up to iBytes on this socket.
    int RecvTeol();                         // Receive up to the telnet eol.
    int RecvTeol(int iToast);               // Receive up to the telnet eol.
    int Send(char *szBuf, size_t iLen);     // Send the buffer on this socket.
    int Send(char *szBuf);                  // Send the buffer on this socket.
    int Send(const char *szBuf);            // Send the buffer on this socket.
    int SendText(char *szFileName);         // Send this text file accross the
                                            // socket.
    int SendBinary(char *szFileName);       // Send this binary file accross the
                                            // socket.
    int ResolveName();                      // Look up the ip address and name
                                            // of the peer.
    int Close();                            // Close this socket.


    int iSock;                              // The socket number allocated.
    int iErr;                               // The last error code on a socket
                                            // call.
    char  *szOutBuf;                        // Used to return data in.
    char  *szPeerName;                      // The ip name of the peer
                                            // connected.
    char  *szPeerIp;                        // The ip address of the peer
                                            // connected.
    unsigned long ulTimeout;                // The timeout for receives in
                                            // seconds.
protected:
    struct sockaddr_in  siUs;               // Our address
    struct sockaddr_in  siThem;             // Their address
    short int sPortUs;                      // Our port
    short int sPortThem;                    // Their port
    socklen_t iLen;                         // The size of siUs and siThem
    int iBuf;                               // Active buffer flag.
    char  *szBuf1, *szBuf2;                 // Internal buffers.
    int iBeg1, iEnd1, iBeg2, iEnd2;         // Buffer markers.
};
#endif
