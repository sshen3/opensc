#include <iostream>
#include "func.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>

using namespace std;

int main(int argc, char *argv[]){
	int sock, bytes_recieved;  
        char send_data[1024],recv_data[1024];
        struct hostent *host;
        struct sockaddr_in server_addr;
	struct ifreq ifr;
	char data[128];
	char cmd[1024];

	string server_ip = "192.168.100.1";

	if(argc < 3){
		cout << "Usage: vlc_player <source IP> <target IP>" << endl;
		exit(1);
	}

	system("ping 10.1.1.3 -c 3");

        host = gethostbyname(server_ip.c_str());

	/* Open a socket to connect to the controller. */
        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {   	 
		perror("Socket");
      		exit(1);
        }

        server_addr.sin_family = AF_INET;     
        server_addr.sin_port = htons(5407);   
        server_addr.sin_addr = *((struct in_addr *)host->h_addr);
        bzero(&(server_addr.sin_zero),8); 
        if (connect(sock, (struct sockaddr *)&server_addr,
                    sizeof(struct sockaddr)) == -1) {
        	perror("Connect");
        	exit(1);
        }

	string myMac = getMacAddr("eth1");
	strncpy(ifr.ifr_name, "eth1", IFNAMSIZ-1);

	ioctl(sock, SIOCGIFADDR, &ifr);

	sprintf(data, "%s %s", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr), myMac.c_str());
	send(sock, data, 128, 0);


	close(sock);


	sprintf(cmd, "sudo arp -s %s fe:29:69:8e:57:17", argv[1]);

	system(cmd);
	sleep(20);

	//sprintf(cmd, "cvlc --play-and-exit http://%s/obama.mp4 :sout='#transcode{vcodec=h264,acodec=mpga,ab=128,channels=2,samplerate=44100}:udp{dst=%s:1234}' :sout-keep"
	//	, argv[1], argv[2]);
	//system(cmd);

	sprintf(cmd, "vlc http://%s/obama.mp4", argv[1]);

	system(cmd);
	return 0;
	
}
