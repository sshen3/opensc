#include "opensc.h"
#include <unistd.h>

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>

#define CAPACITY	1

int test_handler(){
	opensc sc("localhost");

	sc.add_nf(0, 1);
	//sleep(2);
	//sc.add_nf(0, 1);

	return 0;
}


int video_handler(string src_mac, string dst_mac, string src_ip, string dst_ip
	, string src_port, string dst_port){
	opensc sc("localhost");
	sc.add_flow(src_mac, dst_mac, src_ip, dst_ip, src_port, dst_port);
	int num_nf = 1;
	int mode;

	//sc.add_nf(0, 2);
	//sleep(2);
	//sc.add_nf(1, 1);
	//sleep(2);

	ifstream inputfile("mode.dat", ifstream::in);
	inputfile >> mode;
	inputfile.close();

	vector < struct flow_info > flows = sc.get_flow();

	num_nf = flows.size(); 

	vector < struct host_info > hosts = sc.get_host_list();

	/*
	if(flows.size() > CAPACITY){
		sc.add_nf(hosts.size() - 1, 1);
		return 0;
	}
	*/

	if(mode == 0 && flows.size() > CAPACITY) return 0;


	for(int i = 0; i < hosts.size(); i++){
		struct sockaddr_in host_ip;
		host_ip.sin_addr.s_addr = hosts[i].host_addr;

		cout << "src mac = " << src_mac << " host mac = " << hosts[i].mac_addr << endl;


		if(src_mac.compare(hosts[i].mac_addr) == 0){
			sc.add_nf(i, 2);
			sleep(2);
			sc.add_service(flows.size() - 1, i, 0);
			cout << "add service : " << (flows.size() - 1) << " " << i << endl;

		}
			

		for(int j = 0; j < MAX_NUM_NF; j++){
			if(hosts[i].active_nf[j].nf_id > -1){
			}
		}
	}

	sc.add_nf(hosts.size() - 1, 1);
	sleep(2);
	sc.add_service(flows.size() - 1, hosts.size() - 1, num_nf - 1);



}

int main(){
	int sock, bytes_recieved , is_true = 1;  
        char send_data [1024] , recv_data[1024];

        struct sockaddr_in server_addr, local_addr;
        //int sin_size;

	struct hostent *local;

	system("rm ./flow.dat");

	//test_handler();
	//exit(0);


        local = gethostbyname("127.0.0.1");
	local_addr.sin_addr = *((struct in_addr *)local->h_addr);

        /* open the socket for controller */
	/* this socket connects to APIs and hosts. */
        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            perror("Socket");
            exit(1);
        }

        
        server_addr.sin_family = AF_INET;         
        server_addr.sin_port = htons(5407);     
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


	    int sin_size = sizeof(struct sockaddr_in);
	    int connected = accept(sock, (struct sockaddr *)&client_addr,(socklen_t*) &sin_size);

	    recv(connected,data,128,0);

	    src_ip = strtok(data, " ");
	    src_mac = strtok(NULL, " ");

	    video_handler(src_mac, "fe:29:69:8e:57:17", src_ip, "10.1.1.6", "5000", "80");

	    close(connected);
	}
	
}
