#include <iostream>

#include <unistd.h>

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>

using namespace std;

int main(){
	int sock, bytes_recieved , is_true = 1;  
        char send_data [1024] , recv_data[1024];

        struct sockaddr_in server_addr, local_addr;
        //int sin_size;

	struct hostent *local;

	system("rm ./flow.dat");

	//test_handler();


        local = gethostbyname("127.0.0.1");
	local_addr.sin_addr = *((struct in_addr *)local->h_addr);

        /* open the socket for controller */
	/* this socket connects to APIs and hosts. */
        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            perror("Socket");
            exit(1);
        }

        
        server_addr.sin_family = AF_INET;         
        server_addr.sin_port = htons(4321);     
        server_addr.sin_addr.s_addr = INADDR_ANY; 
        bzero(&(server_addr.sin_zero),8); 

	cout << "binding" << endl;

        if (bind(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr))
                                                                       == -1) {
            perror("Unable to bind");
            exit(1);
        }

        if (listen(sock, 5) == -1) {
            perror("Listen");
            exit(1);
        }
		
	printf("\nTCPServer Waiting for client on port 5405\n");
        fflush(stdout);


        while(1){

	    struct sockaddr_in client_addr;
	    char data[128];
	    char* src_mac;
	    char* src_ip;
	    char* client_ip;
	    char cmd[256];


	    int sin_size = sizeof(struct sockaddr_in);
	    int connected = accept(sock, (struct sockaddr *)&client_addr,(socklen_t*) &sin_size);

	    recv(connected,data,128,0);

	    client_ip = inet_ntoa(client_addr.sin_addr);
	    cout << "data = " << data << " addr = " << client_ip << endl;

	    sprintf(cmd, "./vlc_player %s", client_ip);
	    system(cmd);
	    close(connected);
	}

}
