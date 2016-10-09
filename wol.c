/* Compile on Windows:

cl /nologo /W3 /O2 /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_X86_" /link wsock32.lib wol.c

Must configure nic to accept magic package and setup power profile to allow
wake on lan from nic.
Not for WiFi.

*/
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>

#ifdef _WIN32
#define ERR_RETURN( x, e) if (x == SOCKET_ERROR) { checkError( e); return(0); }
#else
#define ERR_RETURN( x, e) if (x < 0) { checkError( e); return(0); }
#define SOCKET int
#endif

static int checkError( char *szMsg)
{
#ifdef _WIN32
	int err = WSAGetLastError();
	if ((err == WSAEINTR) || (err == WSAEWOULDBLOCK) || (err == WSAEINPROGRESS))
	{
		printf( "Warning error=%08.8x\n", errno);
		return 1;
	}
	else
	{
		printf("Error %d: %s\n", err, szMsg);
		WSACleanup();
		return 0;
	}
#else
	perror(e);
	if ((errno == EINTR) || (errno = EWOULDBLOCK) || (errno == EINPROGRESS))
		return 1;
	else
		return 0;
#endif
}
#ifdef _WIN32
/*--------------------------------------------------------
  Window socket startup
  --------------------------------------------------------*/
int socketStartWin32(void)
{
	WSADATA wsaData;
	if (WSAStartup(0x202,&wsaData) == SOCKET_ERROR)
	{
		printf("WSAStartup failed with error %d\n",WSAGetLastError());
		WSACleanup();
		return -1;
	}
	return 0;
}
void socketEndWin32(void)
{
   WSACleanup();
}
#endif

/* Magic package format:
	06 x 255 or (0xff)
	16 x MAC Address of target PC

*/ 
int main(int argc, char **argv)
{
	int i;
	unsigned char tosend[102];
	unsigned char mac[6];

	if ((argc < 2) || (strlen(argv[1]) < 17))
	{
		printf("Usage wol <mac address aa:bb:cc:dd:ee:ff>\n");
		exit(0);
	}

	/** store mac address **/
	for (i=0; i < sizeof(mac); i++)
	{
		char *cp = &argv[1][i*3];
		mac[i] = 0x00;
		sscanf(cp,"%2X", &mac[i]);
	}
	if (argc > 2)	// validate before sent
	{
		int ecn = 0;
		printf( "Send magic package to ");
		for (i=0; i < sizeof(mac); i++)
		{
			if (i) printf(":");
			printf( "%.2X", mac[i]);
			if (!mac[i]) ecn++;
		}
		printf("\n");
		if (ecn)
		{
			printf("Invalid mac address\n");
			exit(0);
		}
	}


	/** first 6 bytes of 255 **/
	for( i = 0; i < 6; i++) {
		tosend[i] = 0xFF;
	}
	/** append it 16 times to packet **/
	for( i = 1; i <= 16; i++)
	{
		memcpy(&tosend[i * 6], mac, 6 * sizeof(unsigned char));
	}

#ifdef _WIN32
	if (socketStartWin32())
		exit(-1);
#endif
	if (1)
	{
		int udpSocket;
		struct sockaddr_in udpClient, udpServer;
		int broadcast = 1;
		int rv;

		udpSocket = socket(AF_INET, SOCK_DGRAM, 0);

		/** you need to set this so you can broadcast **/
		if (setsockopt(udpSocket, SOL_SOCKET, SO_BROADCAST, (char *) &broadcast, sizeof broadcast))
		{
			checkError("setsockopt (SO_BROADCAST)");
			exit(1);
		}
		udpClient.sin_family = AF_INET;
		udpClient.sin_addr.s_addr = INADDR_ANY;
		udpClient.sin_port = 0;

		bind(udpSocket, (struct sockaddr*)&udpClient, sizeof(udpClient));

		/** …make the packet as shown above **/

		/** set server end point (the broadcast addres)**/
		udpServer.sin_family = AF_INET;
		udpServer.sin_addr.s_addr = inet_addr("192.168.1.255");
		udpServer.sin_port = htons(9);

		/** send the packet **/
		rv = sendto(udpSocket, tosend, sizeof(unsigned char) * 102, 0, (struct sockaddr*)&udpServer, sizeof(udpServer));
		if (argc > 2) printf( "Sent %d bytes\n", rv);
	}

#ifdef _WIN32
	socketEndWin32();
#endif
	return 0;
}

