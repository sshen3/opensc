#include "opensc.h"

#include <string.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

vector < string > cutToken(string input, string ss){
	vector < string > token;
	int len = input.length() + 1;
	char *line = new char[len];
	memcpy(line, input.c_str(), len);

	char* word = strtok(line, ss.c_str());
        while(word){
                string newWord(word);
                token.push_back(newWord);

                word = strtok(NULL, ss.c_str());
        }

	delete line;

	return token;
}


opensc::opensc(string _server_ip){
	server_ip = _server_ip;
}

int opensc::openSock(){
	int sock;
	struct sockaddr_in server_addr;

	struct hostent* host = gethostbyname(server_ip.c_str());

	/* Open a socket to connect to the controller. */
        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {   	 
		perror("Socket");
      		exit(1);
        }

        server_addr.sin_family = AF_INET;     
        server_addr.sin_port = htons(5405);   
        server_addr.sin_addr = *((struct in_addr *)host->h_addr);
        bzero(&(server_addr.sin_zero),8); 
        if (connect(sock, (struct sockaddr *)&server_addr,
                    sizeof(struct sockaddr)) == -1) {
        	perror("Connect");
        	exit(1);
        }

	return sock;
}

int opensc::closeSock(int sock){
	close(sock);
	return 0;
}

vector < struct host_info > opensc::get_host_list(){
	int sock = openSock();
	string cmd = "get_host_list";
	vector < struct host_info > hosts;
	char ret_data[255];
	vector < string > tokens;
	struct host_info current_host;

	current_host.host_addr = 0;

	send(sock, cmd.c_str(), cmd.length(), 0);


	recv(sock, ret_data, 255, 0);


	tokens = cutToken(ret_data, "\t");


	for(int i = 0; i < tokens.size(); i++){
		int label = atoi(tokens[i].c_str());

		if(label == 1){
			if(current_host.host_addr != 0) hosts.push_back(current_host);

			for(int j = 0; j < MAX_NUM_NF; j++) current_host.active_nf[j].nf_id = -1;
			current_host.host_addr = inet_addr(tokens[i + 1].c_str());
			i++;
		}

		else if(label == 2){
			strcpy(current_host.mac_addr, tokens[i + 1].c_str());
		}

		else if(label == 3){
			int nf_num = atoi(tokens[i + 1].c_str());
			int nf_index = atoi(tokens[i + 2].c_str());
			struct nf_info nf;
			nf.nf_id = nf_num - 1;

			current_host.active_nf[nf_index] = nf;

			i += 2;
		}

	}

	if(current_host.host_addr != 0) hosts.push_back(current_host);

	closeSock(sock);

	return hosts;
}

void opensc::disconnect_host(int host_ID){
	int sock = openSock();
	char cmd[1024];

	sprintf(cmd, "disconnect_host %d", host_ID + 1);

	send(sock, cmd, 1024, 0);

	closeSock(sock);
}

void opensc::add_nf(int host_num, int nf_num){
	int sock = openSock();
	char cmd[1024];

	sprintf(cmd, "add_nf %d %d", host_num + 1, nf_num + 1);

	send(sock, cmd, 1024, 0);

	closeSock(sock);
}

void opensc::add_flow(string src_mac, string dst_mac, string src_ip, string dst_ip
	, string src_port, string dst_port){
	int sock = openSock();
	char cmd[1024];

	sprintf(cmd, "add_flow %s %s %s %s %s %s", src_ip.c_str(), dst_ip.c_str()
		, src_port.c_str(), dst_port.c_str(), src_mac.c_str(), dst_mac.c_str());

	send(sock, cmd, 1024, 0);

	closeSock(sock);
}


vector < struct flow_info > opensc::get_flow(){
	int sock = openSock();
	char cmd[1024];
	char ret_data[255];
	vector < struct flow_info > flows;
	struct flow_info current_flow;
	vector < string > tokens;

	sprintf(cmd, "get_flow");

	send(sock, cmd, 1024, 0);

	recv(sock, ret_data, 255, 0);


	tokens = cutToken(ret_data, " ");

	for(int i = 0; i + 6 < tokens.size(); i+=5){
		inet_aton(tokens[i + 1].c_str(), &current_flow.src_ip.sin_addr);
		inet_aton(tokens[i + 2].c_str(), &current_flow.dst_ip.sin_addr);

		current_flow.src_port = atoi(tokens[i + 3].c_str());
		current_flow.dst_port = atoi(tokens[i + 4].c_str());

		strcpy(current_flow.src_mac, tokens[i + 5].c_str());
		strcpy(current_flow.dst_mac, tokens[i + 6].c_str());

		flows.push_back(current_flow);

		
	}
	
	closeSock(sock);

	return flows;

}

void opensc::add_service(int service_i, int host_i, int nf_i){
	int sock = openSock();
	char cmd[1024];

	sprintf(cmd, "add_service %d %d %d", service_i + 1, host_i + 1, nf_i);

	send(sock, cmd, 1024, 0);

	closeSock(sock);
}


