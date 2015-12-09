#include "interface.h"
#include "func.h"

#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include <sstream>
#include <iterator>

#include <json-c/json.h>

#include <sys/mman.h>
#include <signal.h>

#define DEBUG 1


interface::interface(){
	/* initial globel variable for hosts */
	for(int i = 0; i < MAX_NUM_HOST; i++){
		pipe(fd_down[i]);  /* the pipe is between main process and socket proecess. */

		host[i] = (struct host_info *) mmap(NULL, sizeof(struct host_info), PROT_READ | PROT_WRITE,
        		MAP_SHARED | MAP_ANONYMOUS, -1, 0);
		host[i] -> host_addr = 0;
		host[i] -> is_ovs = 0;

		for(int j = 0; j < MAX_NUM_NF; j++) host[i]->active_nf[j].nf_id = -1;

		service[i] = (struct service_info *) mmap(NULL, sizeof(struct service_info), PROT_READ | PROT_WRITE,
        		MAP_SHARED | MAP_ANONYMOUS, -1, 0);
		service[i] -> flow_id = -1;
		service[i] -> len = 0;

		for(int j = 0; j < MAX_CHAIN_LEN; j++){
			service[i]->host_chain[j] = -1;
			service[i]->nf_chain[j] = -1;
		}



		active_client[i] = (bool *) mmap(NULL, sizeof(bool), PROT_READ | PROT_WRITE,
        		MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        }

	/* In the beginning, all NFs are inactive. */
	for( int i = 0; i < MAX_NUM_NF; i++){
		active_nf[i].nf_id = -1;
	}

	set_graphic();

}

interface::~interface(){
	kill(pid, SIGKILL);
	kill(pid2, SIGKILL);
}

/* if is_server = true, run server (controller) process. if is_client = true,
run agent process. */
void interface::runner(bool is_server, bool is_client, string addr, int is_ovs){
	//pid_t pid, pid2;
	pid = fork();

	
	if(pid == 0){
		if(is_client) agent_runner(addr, is_ovs);
		exit(0);
	}
	else if(pid > 0){
	
		pid2 = fork();
		if(pid2 == 0 && is_server) server_runner();
		else if(pid2 > 0){
			for(int i = 0; i < MAX_NUM_HOST; i++)
				close(fd_down[i][0]);
			
			shell();
			kill(pid2, SIGKILL);
			kill(pid, SIGKILL);
		}
		else exit(0);
	}
}

/* the shell for test. */
void interface::shell(){
	bool quit = false;

	while(!quit){
		string cmd = "";
		cout << "ALL-SDN SHELL>>";
		getline(cin, cmd);

		send_msg(cmd);

		if(cmd.compare("quit") == 0) quit = true;
	}
}

/* the flow information is written to a file for globel use. This also can be
impelented as globel variable. */
void interface::save_flow(string flow_info){
	ofstream outputfile("flow.dat", ofstream::app);

	outputfile << flow_info << endl;

	outputfile.close();
}

void interface::load_flow(){
	ifstream inputfile("flow.dat", ifstream::in);


	while(!inputfile.eof()){
		char line[1024];
		char src_ip[32];
		char dst_ip[32];
		char src_mac[64];
		char dst_mac[64];
		uint32_t src_port;
		uint32_t dst_port;
		int flow_id;


		inputfile.getline(line, 1024);

		if(strlen(line) <= 0) break;

		strcpy(src_ip, strtok(line, " "));
		strcpy(dst_ip, strtok(NULL, " "));
		src_port = atoi(strtok(NULL, " "));
		dst_port = atoi(strtok(NULL, " "));
		strcpy(src_mac, strtok(NULL, " "));
		strcpy(dst_mac, strtok(NULL, " "));

		struct flow_info newFlow;
		inet_aton(src_ip, &newFlow.src_ip.sin_addr);
		inet_aton(dst_ip, &newFlow.dst_ip.sin_addr);
		newFlow.src_port = src_port;
		newFlow.dst_port = dst_port;
		strcpy(newFlow.src_mac, src_mac);
		strcpy(newFlow.dst_mac, dst_mac);
		

		flows.push_back(newFlow);
		flow_id = flows.size() - 1;


		
		for(int j = 0; j < MAX_NUM_SERVICE; j++){
			if(service[j]->flow_id < 0){
				service[j]->flow_id = j;
			}
		}
		
		


	}


	inputfile.close();
}

int interface::send_msg(string msg){
	string cmd;
	char cmd_down[256];
	int host_ID;
	int vm_ID, vm_index;
	char* token_ptr;

	struct json_object *root; /* the messages between controller and agents is wrapped in json format. */

	if(msg.length() <= 0) return 0;

	/* cut msg into tokens */
	vector < string > tokens;
	strcpy(cmd_down, msg.c_str());

	token_ptr = strtok(cmd_down, " ");

	while(token_ptr){
		string newToken(token_ptr);
		tokens.push_back(newToken);
		token_ptr = strtok(NULL, " ");
	}



	cmd = tokens[0];


	
	/* the controller will ask the host with host_ID to enable the NF with vm_ID. */
	if(cmd.compare("start_nf") == 0){

		if(tokens.size() < 3){
			cout << "Usage: start_nf <host ID> <VM ID>" << endl;
			return -1;
		}
		
		host_ID = atoi(tokens[1].c_str());
		vm_ID = atoi(tokens[2].c_str());
		vm_index = atoi(tokens[3].c_str());

		char json_str[512];

		/* wrap information into json format. */
		root = json_object_new_object();
		json_object_object_add(root, "cmd", json_object_new_string(cmd.c_str()));
		json_object_object_add(root, "host ID", json_object_new_int(host_ID));
		json_object_object_add(root, "vm ID", json_object_new_int(vm_ID));
		json_object_object_add(root, "vm index", json_object_new_int(vm_index));

		string json_msg(json_object_to_json_string(root));
		strcpy(json_str, json_object_to_json_string(root));

		json_object_put(root);
		

		/* write to the pipe to ask socket proecess to send the json massage to the host. */
		int byte_sent = write(fd_down[host_ID][1], json_str, 512);
	}

	/* the controller ask the host to delete a NF. */
	else if(cmd.compare("del_nf") == 0){
		host_ID = atoi(tokens[1].c_str());
		vm_ID = atoi(tokens[2].c_str());
		char json_str[512];

		
		root = json_object_new_object();
		json_object_object_add(root, "cmd", json_object_new_string(cmd.c_str()));
		json_object_object_add(root, "host ID", json_object_new_int(host_ID));
		json_object_object_add(root, "vm ID", json_object_new_int(vm_ID));

		string json_msg(json_object_to_json_string(root));
		strcpy(json_str, json_object_to_json_string(root));

		json_object_put(root);
		


		int byte_sent = write(fd_down[host_ID][1], json_str, 512);
	}

	else if(cmd.compare("reset_nf") == 0){
		host_ID = atoi(tokens[1].c_str());
		char json_str[512];

		
		root = json_object_new_object();
		json_object_object_add(root, "cmd", json_object_new_string(cmd.c_str()));
		json_object_object_add(root, "host ID", json_object_new_int(host_ID));

		string json_msg(json_object_to_json_string(root));
		strcpy(json_str, json_object_to_json_string(root));

		json_object_put(root);
		


		int byte_sent = write(fd_down[host_ID][1], json_str, 512);
	}


	/* The controller asks the host to disconnect with the controller. */
	else if(cmd.compare("disconnect_host") == 0){
		if(tokens.size() >= 2){
			char json_str[512];

			host_ID = atoi(tokens[1].c_str());

			root = json_object_new_object();
			json_object_object_add(root, "cmd", json_object_new_string(cmd.c_str()));
			json_object_object_add(root, "host ID", json_object_new_int(host_ID));

			string json_msg(json_object_to_json_string(root));
			strcpy(json_str, json_object_to_json_string(root));

			json_object_put(root);
			
			int byte_sent = write(fd_down[host_ID][1], json_str, 512);
		}
	}

	/* Quit main controller or host proecess by enter "quit" in test shell. */
	else if(cmd.compare("quit")){
		char quit_str[] = "quit";

		int byte_sent = write(fd_down[host_ID][1], quit_str, strlen(quit_str) + 1);
	}

		
	/* output the number of host for test. */
	if(cmd.compare("num_host") == 0){
		int host_count = 0;
		for(int i = 0; i < MAX_NUM_HOST; i++){
			if(host[i] -> host_addr != 0){
				host_count++;
			}
		}
	}
	//write(fd_down[msg.vm_ID][1], &msg, sizeof(struct VM_msg));
}

/* the process for agent who receives command from the controller. */
void interface::agent_runner(string server_ip, int is_ovs){
	
	int sock, bytes_recieved;  
        char send_data[1024],recv_data[1024];
        struct hostent *host;
        struct sockaddr_in server_addr;  

        host = gethostbyname(server_ip.c_str());

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

	char client_mac_addr[128];
	if(is_ovs == 1)
		sprintf(client_mac_addr, "%s %d", getMacAddr("br0").c_str(), is_ovs);
	else
		sprintf(client_mac_addr, "%s %d", getMacAddr("eth1").c_str(), is_ovs);
	//strcpy(client_mac_addr, getMacAddr("eth1").c_str());
	send(sock, client_mac_addr, 128, 0);

        while(1){
		fd_set  active_fd_set;
                //int   max_fd;
                //int   ret;
		int host_ID = 0;

                FD_ZERO(&active_fd_set);
                FD_SET(sock, &active_fd_set);
		FD_SET(fd_down[0][0], &active_fd_set);

                int max_fd;

		if(sock > fd_down[0][0]) max_fd= sock;
		else max_fd = fd_down[0][0];

                struct timeval  tv;
                tv.tv_sec = 1;
                tv.tv_usec = 0;

		
		/* waiting for the messages from socket or pipe. */
                int ret = select(max_fd + 1, &active_fd_set, NULL, NULL, &tv);
                if (ret == -1) {
                        perror("select()");
                        return;
                } else if (ret == 0) {
                        //printf("select timeout\n");
                        //if(msg_to_server[host_ID]->active){
                                //send(connected, msg_to_host[host_ID],sizeof(struct VM_msg), 0);
                        //}
                        continue;
                } else {
			/* message is from socket (the controller)*/
                        if(FD_ISSET(sock, &active_fd_set)){
				char json_str[512];
				char ret_handle[512];
				bytes_recieved=recv(sock, json_str, 512,0);

				int ret_host_handler = host_handler(json_str, ret_handle);

				send(sock, ret_handle, 512, 0);

				if(ret_host_handler == -1){
					close(sock);
					exit(0);
				}

                        }
			/* message is from pipe (main process or NF)*/
			else if(FD_ISSET(sock, &active_fd_set)){
			}
                }

        
           		//close(sock);
        }   
	
}

void interface::server_runner(){
	int sock, bytes_recieved , is_true = 1;  
        char send_data [1024] , recv_data[1024];

        struct sockaddr_in server_addr, local_addr;    
        //int sin_size;

	struct hostent *local;

        local = gethostbyname("127.0.0.1");
	local_addr.sin_addr = *((struct in_addr *)local->h_addr);

        /* open the socket for controller */
	/* this socket connects to APIs and hosts. */
        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            perror("Socket");
            exit(1);
        }

        
        server_addr.sin_family = AF_INET;         
        server_addr.sin_port = htons(5405);     
        server_addr.sin_addr.s_addr = INADDR_ANY; 
        bzero(&(server_addr.sin_zero),8); 


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

	    //fd_set server_fd_set;
	    int max_fd;
	    int ret;

	    //FD_ZERO(&server_fd_set);
	    //FD_SET(pipe_fd_to[0], &server_fd_set);
	    //FD_SET(sock, &server_fd_set);

	    //if(pipe_fd_to[0] > sock) max_fd = pipe_fd_to[0];
	    //else max_fd = sock;

	    //struct timeval  tv;
	    //tv.tv_sec = 2;
            //tv.tv_usec = 0;

	    struct sockaddr_in client_addr;


	    int sin_size = sizeof(struct sockaddr_in);
	    int connected = accept(sock, (struct sockaddr *)&client_addr,(socklen_t*) &sin_size);

	    /* fork process for each connection to support multiple connections simultaneously. */
	    int pid = fork();
	    if(pid > 0) continue;


	    int host_ID;

            printf("\n I got a connection from (%s , %d)",
                   inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));
	    fflush(stdout);
	    
	    /* If the socket is from local IP, the messages are from API or web UI. */
	    /* To support APIs remotely, we should have better way to differentiate the message from
		API and hosts. */
	    if(local_addr.sin_addr.s_addr == client_addr.sin_addr.s_addr){
		char command[1024];
		char* token_ptr;
		cout << "From local host" << endl;

		fflush(stdout);

			memset(command, 0, 1024);
			recv(connected,command,1024,0);
			cout << "Command = " << command << endl;

			vector < string > tokens;

			token_ptr = strtok(command, " ");

			while(token_ptr){
				string newToken(token_ptr);
				tokens.push_back(newToken);
				token_ptr = strtok(NULL, " ");
			}

			/* the following if-else if statements represents different command from APIs or Web UI. */
			/* new command should be add here. */

			/* get the list of active hosts connecting to the controller. */
			if(tokens[0].compare("get_host_list") == 0){
				cout << "get host list" << endl;
				char ret[255];
				memset(ret, 0, 255);

				for(int i = 0; i < MAX_NUM_HOST; i++){
					if(host[i]->host_addr != 0){
						struct in_addr newIP;
						newIP.s_addr = host[i]->host_addr;
						strcat(ret, "1\t");
						strcat(ret, inet_ntoa(newIP));
						strcat(ret, "\t");

						strcat(ret, "2\t");
						strcat(ret, host[i]->mac_addr);
						strcat(ret, "\t");

						for(int j = 0; j < MAX_NUM_NF; j++){
							if(host[i]->active_nf[j].nf_id >= 0){
								char nf_num[16];
								char nf_index[16];
								sprintf(nf_num, "%d", (host[i]->active_nf[j].nf_id + 1));
								sprintf(nf_index, "%d", j);

								strcat(ret, "3\t");
								strcat(ret, nf_num);
								strcat(ret, "\t");
								strcat(ret, nf_index);
								strcat(ret, "\t");
							}
						}
					}
				}

				send(connected, ret, 255, 0);
			}

			/* disconnect hosts. */
			else if(tokens[0].compare("disconnect_host") == 0){

				if(tokens.size() >= 2){
					int host_num = atoi(tokens[1].c_str()) - 1;
					char cmd[128];

					for(int i = 0; i < MAX_NUM_NF; i++)
						host[host_num]->active_nf[i].nf_id = -1;
					host[host_num]->host_addr = 0;
					
					sprintf(cmd, "disconnect_host %d", host_num);
					send_msg(cmd);
				}
			}

			/* turn off an active NF. */
			else if(tokens[0].compare("del_nf") == 0){
				if(tokens.size() >= 3){
					int host_num = atoi(tokens[1].c_str()) - 1;
					int nf_index = atoi(tokens[2].c_str());
					char cmd[128];

					host[host_num]->active_nf[nf_index].nf_id = -1;

					sprintf(cmd, "del_nf %d %d", host_num, nf_index);
					send_msg(cmd);
				}
			}

			else if(tokens[0].compare("reset_nf") == 0){
				for(int i = 0; i < MAX_NUM_HOST; i++){
					if(host[i]->host_addr != 0){
						char cmd[128];

						sprintf(cmd, "reset_nf %d", i);
						send_msg(cmd);
					}
				}

				set_graphic();
			}

			/* turn on a NF. */
			else if(tokens[0].compare("add_nf") == 0){
				if(tokens.size() >= 3){
					int host_num = atoi(tokens[1].c_str()) - 1;
					int nf_num = atoi(tokens[2].c_str()) - 1;
					char cmd[128];
					int nf_index;

					for(int i = 0; i < MAX_NUM_NF; i++){
						if(host[host_num]->active_nf[i].nf_id < 0){
							host[host_num]->active_nf[i].nf_id = nf_num;
							nf_index = i;
							break;
						}
					}

					set_graphic();

					sprintf(cmd, "start_nf %d %d %d", host_num, nf_num, nf_index);
					send_msg(cmd);

				}
			}

			else if(tokens[0].compare("add_flow") == 0){
				if(tokens.size() >= 5){

					string new_flow_str(tokens[1]);
					new_flow_str.append(" ");
					new_flow_str.append(tokens[2]);
					new_flow_str.append(" ");
					new_flow_str.append(tokens[3]);
					new_flow_str.append(" ");
					new_flow_str.append(tokens[4]);

					if(tokens.size() >= 7){
						new_flow_str.append(" ");
						new_flow_str.append(tokens[5]);
						new_flow_str.append(" ");
						new_flow_str.append(tokens[6]);
					}


					save_flow(new_flow_str);



					/*
					struct flow_info newFlow;
					inet_aton(tokens[1].c_str(), &newFlow.src_ip.sin_addr);
					inet_aton(tokens[2].c_str(), &newFlow.dst_ip.sin_addr);
					newFlow.src_port = atoi(tokens[3].c_str());
					newFlow.dst_port = atoi(tokens[4].c_str());

					flows.push_back(newFlow);
					cout << "num of flows = " << flows.size() << endl;
					*/
				}
			}

			else if(tokens[0].compare("get_flow") == 0){
				char ret[255];

				load_flow();


				memset(ret, 0, 255);
				for(int i = 0; i < flows.size(); i++){
					char flow_num[16];
					char src_port_str[16];
					char dst_port_str[16];

					sprintf(flow_num, "%d", (i + 1));
					sprintf(src_port_str, "%u", flows[i].src_port);
					sprintf(dst_port_str, "%u", flows[i].dst_port);

					strcat(ret, flow_num);
					strcat(ret, " ");
					strcat(ret, inet_ntoa(flows[i].src_ip.sin_addr));
					strcat(ret, " ");
					strcat(ret, inet_ntoa(flows[i].dst_ip.sin_addr));
					strcat(ret, " ");
					strcat(ret, src_port_str);
					strcat(ret, " ");
					strcat(ret, dst_port_str);
					strcat(ret, " ");
					strcat(ret, flows[i].src_mac);
					strcat(ret, " ");
					strcat(ret, flows[i].dst_mac);
					strcat(ret, " ");

				}


				send(connected, ret, 255, 0);
			}

			else if(tokens[0].compare("add_service") == 0){
				if(tokens.size() >= 4){

					int service_i = atoi(tokens[1].c_str()) - 1;
					int host_i = atoi(tokens[2].c_str()) - 1;
					int nf_i = atoi(tokens[3].c_str());

					service[service_i]->host_chain[service[service_i]->len] = host_i;
					service[service_i]->nf_chain[service[service_i]->len] = nf_i;

					service[service_i]->len++;


					if(service[service_i]->len > 1){
						int i = service[service_i]->len - 1;
						struct match flow_match;
						int flow_i;
						int src_host_i = service[service_i]->host_chain[i - 1];
						int src_nf_i = service[service_i]->nf_chain[i - 1];
						int src_nf_port = host[src_host_i]->active_nf[src_nf_i].portNum;
						int dst_host_i = service[service_i]->host_chain[i];
						int dst_nf_i = service[service_i]->nf_chain[i];
						int dst_nf_port = host[dst_host_i]->active_nf[dst_nf_i].portNum;
						map < string, pair < string, int > > host_attach;

						char src_mac_addr[64];
						char dst_mac_addr[64];

						memcpy(src_mac_addr, host[src_host_i]->mac_addr, 64);
						memcpy(dst_mac_addr, host[dst_host_i]->mac_addr, 64);


						load_flow();
						flow_i = service[service_i]->flow_id;


						flow_match.src_mac = flows[flow_i].src_mac;//"9c:b6:54:bb:02:05";
						//flow_match.dst_mac = "9c:b6:54:bb:0b:9d";
						flow_match.dst_mac = flows[flow_i].dst_mac;//"fe:29:69:8e:57:17";

						flow_match.src_ip = inet_ntoa(flows[flow_i].src_ip.sin_addr);
						flow_match.dst_ip = inet_ntoa(flows[flow_i].dst_ip.sin_addr);
						flow_match.src_port = flows[flow_i].src_port;
						flow_match.dst_port = flows[flow_i].dst_port;

						
						if(host[src_host_i]->is_ovs == 0 || host[dst_host_i]->is_ovs == 0){
							host_attach = getHost();
							if(host[src_host_i]->is_ovs == 0){
								memcpy(src_mac_addr, host_attach[host[src_host_i]->mac_addr].first.c_str(), 64);
								src_nf_port = host_attach[host[src_host_i]->mac_addr].second;
							}
							if(host[dst_host_i]->is_ovs == 0){
								memcpy(dst_mac_addr, host_attach[host[dst_host_i]->mac_addr].first.c_str(), 64);
								dst_nf_port = host_attach[host[dst_host_i]->mac_addr].second;
							}
						}
						



						//setHop(flow_match, "9c:b6:54:bb:02:05", "9c:b6:54:bb:0b:9d", src_nf_port, dst_nf_port);
						//setHop(flow_match, host[src_host_i]->mac_addr, host[dst_host_i]->mac_addr, src_nf_port, dst_nf_port);
						setHop(flow_match, src_mac_addr, dst_mac_addr, src_nf_port, dst_nf_port);

					}
				}
			}

			else if(tokens[0].compare("get_service") == 0){
				if(tokens.size() >= 2){
					int flow_i = atoi(tokens[1].c_str()) - 1;
					char ret[255];

					memset(ret, 0, 255);
					for(int i = 0; i < service[flow_i]->len; i++){
						char host_id[32];
						char nf_id[32];
						char nf_index[32];

						int host_id_i = service[flow_i]->host_chain[i];
						int nf_index_i = service[flow_i]->nf_chain[i];
						int nf_id_i = host[host_id_i]->active_nf[nf_index_i].nf_id;

						sprintf(host_id, "%d", host_id_i + 1);
						sprintf(nf_id, "%d", nf_id_i + 1);
						sprintf(nf_index, "%d", nf_index_i);

						strcat(ret, host_id);
						strcat(ret, " ");
						strcat(ret, nf_id);
						strcat(ret, " ");
						strcat(ret, nf_index);
						strcat(ret, " ");

					}

					send(connected, ret, 255, 0);
				}
			}

			else if(tokens[0].compare("set_graphic") == 0){
				cout << "set graphic" << endl;
				send(connected, "OK", 3, 0);
			}

			else if(tokens[0].compare("change_mode") == 0){
				cout << "change mode" << endl;

				ofstream outputfile("mode.dat", ofstream::out);
				outputfile << tokens[1];
				outputfile.close();
			}

			else if(tokens[0].compare("run_traffic") == 0){
				if(tokens.size() >= 3){
					int flow_i = atoi(tokens[1].c_str()) - 1;
					int rate = atoi(tokens[2].c_str());

					cout << "run traffic " << rate << endl;

				}
			}

			close(connected);
			//continue;
			exit(0);
		//}
	    }
	    
	    /* the connection is from a host, and then we inital the data for the new host. */
	    char client_mac_addr[64];
	    char client_info[128];
	    int is_ovs;
	    bytes_recieved = recv(connected,client_info,128,0);

	    strcpy(client_mac_addr, strtok(client_info, " "));
	    is_ovs = atoi(strtok(NULL, " "));


	    for(int i = 0; i < MAX_NUM_HOST; i++){
		//if(msg_to_host[i] -> host_addr == 0 || msg_to_host[i] -> host_addr == client_addr.sin_addr.s_addr){
		if(host[i] -> host_addr == 0 || host[i] -> host_addr == client_addr.sin_addr.s_addr){ 
			host_ID = i;
			//msg_to_host[i] -> host_addr = client_addr.sin_addr.s_addr;
			host[i] -> host_addr = client_addr.sin_addr.s_addr;
			memcpy(host[i] -> mac_addr, client_mac_addr, 64);
			host[i]->is_ovs = is_ovs;
			//	<< client_mac_addr << endl;
			*active_client[i] = true;
			close(fd_down[i][1]);
			break;
		}
	    }

            while (1)
            {
		fd_set	active_fd_set;
		//int	max_fd;
		//int	ret;
		
		FD_ZERO(&active_fd_set);
		FD_SET(connected, &active_fd_set);
		FD_SET(fd_down[host_ID][0], &active_fd_set);

		if(fd_down[host_ID][0] > connected) max_fd = fd_down[host_ID][0];
		else max_fd = connected;

		struct timeval  tv;
		tv.tv_sec = 1;
        	tv.tv_usec = 0;

		/* wait for the message from pipe or socket. */

		ret = select(max_fd + 1, &active_fd_set, NULL, NULL, &tv);
		if (ret == -1) {
            		perror("select()");
            		return;
        	} else if (ret == 0) {
            		//printf("select timeout %d\n", host_ID);
			/*
			if(msg_to_host[host_ID]->active){
				send(connected, msg_to_host[host_ID],sizeof(struct VM_msg), 0);
				msg_to_host[host_ID]->active = false;
			}
			*/
            		//continue;
        	} else {

			/* The message is from the socket (host). */
			if(FD_ISSET(connected, &active_fd_set)){
				char json_str[512];
				bytes_recieved = recv(connected,json_str,512,0);
				
				/*the host disconnects */
				if(strcmp(json_str, "quit") == 0){
					close(connected);
					cout << "==================  Disconnected ================" << endl;
					exit(0);
				}

				/* call the corresponding handler function to deal with the requests from the host. */
				if(server_handler_map.find(client_addr.sin_addr.s_addr) != server_handler_map.end()){
					server_handler_map[client_addr.sin_addr.s_addr](json_str);
				}
			}

			/* the message is from the pipe. */
			else if(FD_ISSET(fd_down[host_ID][0], &active_fd_set)){
				struct json_object *root, *cmd_j, *nf_index_j;
				struct json_object *r_root, *r_cmd, *r_value, *r_instance;
				int byte_received, nf_index;
				char json_str[512];
				bytes_recieved = read(fd_down[host_ID][0], json_str, 512);

				root = json_tokener_parse(json_str);
				json_object_object_get_ex(root, "cmd", &cmd_j);
				json_object_object_get_ex(root, "vm index", &nf_index_j);

				string cmd(json_object_get_string(cmd_j));
				nf_index = json_object_get_int(nf_index_j);
				

				if(bytes_recieved > 0){
					send(connected, json_str, 512, 0);

					if(cmd.compare("disconnect_host") == 0){
						close(connected);

						
						exit(0);
					}

					memset(json_str, 0, 512);
					bytes_recieved = recv(connected,json_str,512,0);


					r_root = json_tokener_parse(json_str);
					json_object_object_get_ex(r_root, "cmd", &r_cmd);
					cmd = json_object_get_string(r_cmd);


					if(cmd.compare("portNum") == 0){

						json_object_object_get_ex(r_root, "value", &r_value);
						json_object_object_get_ex(r_root, "instance", &r_instance);

						int port_num = json_object_get_int(r_value);
						string instance(json_object_get_string(r_instance));


						host[host_ID]->active_nf[nf_index].portNum = port_num;
						strcpy(host[host_ID]->active_nf[nf_index].instance_id
							, instance.c_str());

					}


					
				}
			}
		}

		
	      /*
              printf("\n SEND (q or Q to quit) : ");
              gets(send_data);
              
              if (strcmp(send_data , "q") == 0 || strcmp(send_data , "Q") == 0)
              {
                send(connected, send_data,strlen(send_data), 0); 
                close(connected);
                break;
              }
               
              else
                 send(connected, send_data,strlen(send_data), 0);  

              bytes_recieved = recv(connected,recv_data,1024,0);

              recv_data[bytes_recieved] = '\0';

              if (strcmp(recv_data , "q") == 0 || strcmp(recv_data , "Q") == 0)
              {
                close(connected);
                break;
              }

              else 
              printf("\n RECIEVED DATA = %s " , recv_data);
              fflush(stdout);
	      */
            }
        }       

      close(sock);
}

int interface::host_handler(char* json_str, char* ret_handle){

	struct json_object *root, *cmd_j, *host_ID_j, *vm_ID_j;
	struct json_object *r_root, *r_cmd, *r_value;
	int host_ID, vm_ID;
	root = json_tokener_parse(json_str);

	json_object_object_get_ex(root, "cmd", &cmd_j);
	json_object_object_get_ex(root, "host ID", &host_ID_j);
	json_object_object_get_ex(root, "vm ID", &vm_ID_j);

	string cmd(json_object_get_string(cmd_j));
	host_ID = json_object_get_int(host_ID_j);
	vm_ID = json_object_get_int(vm_ID_j);


	if(cmd.compare("start_nf") == 0){
		int num_nf = 0;

		for(int i = 0; i < MAX_NUM_NF; i++){
			if(active_nf[i].nf_id != -1) num_nf++;
		}


		pair < string, string > instance_info = startNF(vm_ID, num_nf);
		int portNum;
		char json_str[512];


		for(int i = 0; i < MAX_NUM_NF; i++){
			if(active_nf[i].nf_id == -1){

				active_nf[i].nf_id = vm_ID;
				strcpy(active_nf[i].instance_id, instance_info.first.c_str());
				strcpy(active_nf[i].port, instance_info.second.c_str());

				break;
			}
		}

		portNum = getPortNum(instance_info.second);
		r_root = json_object_new_object();
		json_object_object_add(r_root, "cmd", json_object_new_string("portNum"));
		json_object_object_add(r_root, "value", json_object_new_int(portNum));
		json_object_object_add(r_root, "instance", json_object_new_string(instance_info.first.c_str()));

		string json_msg(json_object_to_json_string(r_root));
		strcpy(json_str, json_object_to_json_string(r_root));

		json_object_put(r_root);

		strcpy(ret_handle, json_str);

		return 0;
	}

	else if(cmd.compare("del_nf") == 0){
		pair < string, string > instance_info;
		char json_str[512];

		if(active_nf[vm_ID].nf_id >= 0){
			instance_info.first = active_nf[vm_ID].instance_id;
			instance_info.second = active_nf[vm_ID].port;

			active_nf[vm_ID].nf_id = -1;

			stopNF(instance_info);
		}

		r_root = json_object_new_object();
		json_object_object_add(r_root, "cmd", json_object_new_string("done"));

		string json_msg(json_object_to_json_string(r_root));
		strcpy(json_str, json_object_to_json_string(r_root));

		json_object_put(r_root);
		strcpy(ret_handle, json_str);

		return 0;

	}

	else if(cmd.compare("reset_nf") == 0){
		pair < string, string > instance_info;
		char json_str[512];

		for(int i = 0; i < MAX_NUM_NF; i++){
			if(active_nf[i].nf_id >= 0){
				instance_info.first = active_nf[i].instance_id;
				instance_info.second = active_nf[i].port;

				active_nf[i].nf_id = -1;

				stopNF(instance_info);
			}
		}

		r_root = json_object_new_object();
		json_object_object_add(r_root, "cmd", json_object_new_string("done"));

		string json_msg(json_object_to_json_string(r_root));
		strcpy(json_str, json_object_to_json_string(r_root));

		json_object_put(r_root);
		strcpy(ret_handle, json_str);

		return 0;

	}


	if(cmd.compare("disconnect_host") == 0){

		char json_str[512];

		r_root = json_object_new_object();
		json_object_object_add(r_root, "cmd", json_object_new_string("done"));

		string json_msg(json_object_to_json_string(r_root));
		strcpy(json_str, json_object_to_json_string(r_root));

		json_object_put(r_root);
		strcpy(ret_handle, json_str);

		return -1;
	}

}

void interface::set_graphic(){
	cout << "setting graphic" << endl;

	ofstream outputfile("/home/kent/public_html/3d_topo/js/data.json", ofstream::out);

	outputfile << "{" << endl;
	outputfile << "\"nodes\":[" << endl;
	outputfile << "  {\"name\":\"switch:1\", \"color\":\"#b2b19d\", \"shape\":\"dot\", \"alpha\":1}," << endl;

	outputfile << "  {\"name\":\"client:1\", \"color\":\"#a7af00\", \"shape\":\"dot\", \"alpha\":1}," << endl;
	outputfile << "  {\"name\":\"client:2\", \"color\":\"#a7af00\", \"shape\":\"dot\", \"alpha\":1}," << endl;
	outputfile << endl;
	outputfile << "  {\"name\":\"switch:2\", \"color\":\"#b2b19d\", \"shape\":\"dot\", \"alpha\":1}," << endl;
	outputfile << endl;

	outputfile << "  {\"name\":\"server:1\", \"color\":\"#0000ff\", \"shape\":\"dot\", \"alpha\":1}," << endl;
	if(host[0]->active_nf[0].nf_id >= 0)
		outputfile << "  {\"name\":\"video NF:1\", \"color\":\"#ff0000\", \"shape\":\"dot\", \"alpha\":1}," << endl;
	outputfile << endl;
	outputfile << "  {\"name\":\"server:2\", \"color\":\"#0000ff\", \"shape\":\"dot\", \"alpha\":1}";
	if(host[1]->active_nf[0].nf_id >= 0)
		outputfile << "," << endl << "  {\"name\":\"video NF:2\", \"color\":\"#ff0000\", \"shape\":\"dot\", \"alpha\":1}" << endl;
	else outputfile << endl;
	outputfile << endl;
	outputfile << "]," << endl;
	outputfile << endl;
	outputfile << "\"edges\":[" << endl;
	outputfile << "  {\"name\":\"switch:1\", \"subnodes\":[\"switch:1\", \"switch:2\", \"client:1\", \"client:2\"]}," << endl;
	outputfile << "  {\"name\":\"switch:2\", \"subnodes\":[\"switch:2\", \"server:1\", \"server:2\"]}";
	if(host[0]->active_nf[0].nf_id >= 0)
		outputfile << "," << endl << "  {\"name\":\"server:1\", \"subnodes\":[\"server:1\", \"video NF:1\"]}";
	else outputfile << endl;
	if(host[1]->active_nf[0].nf_id >= 0)
		outputfile << "," << endl << "  {\"name\":\"server:2\", \"subnodes\":[\"server:2\", \"video NF:2\"]}" << endl;
	else outputfile << endl;
	outputfile << "]" << endl;
	outputfile << "}" << endl;

	outputfile.close();
}
