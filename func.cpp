#include "func.h"
#include <stdlib.h>
#include <fstream>
#include <string.h>
#include <map>

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

int installSDNRules(vector < struct route > myRoute, struct match flow_match, int sn){
	int ret_sn = sn;

	cout << "Installing rules" << endl;
	for(int i = 0; i < myRoute.size(); i++){
		char cmd[512];

		//sprintf(cmd, "curl -d '{\"switch\": \"%s\", \"name\":\"flow-mod-%d\", \"eth_type\":\"0x0800\", \"ipv4_src\":\"%s\", \"ipv4_dst\":\"%s\", \"tp_src\":\"%d\", \"tp_dst\":\"%d\", \"cookie\":\"0\", \"priority\":\"32768\", \"in_port\":\"%d\",\"active\":\"true\", \"actions\":\"output=%d\"}' http://localhost:8080/wm/staticflowpusher/json", myRoute[i].dpid.c_str(), sn, flow_match.src_ip.c_str(), flow_match.dst_ip.c_str(), flow_match.src_port, flow_match.dst_port, myRoute[i].in_port, myRoute[i].out_port);

		if(myRoute[i].in_port == 0)
		sprintf(cmd, "curl -d '{\"switch\": \"%s\", \"name\":\"flow-mod-%d\",\"eth_src\":\"%s\",\"eth_dst\":\"%s\", \"eth_type\":\"0x0800\", \"ipv4_src\":\"%s\", \"ipv4_dst\":\"%s\", \"cookie\":\"0\", \"priority\":\"1\", \"in_port\":\"LOCAL\",\"active\":\"true\", \"actions\":\"output=%d\"}' http://localhost:8080/wm/staticflowpusher/json"
			, myRoute[i].dpid.c_str(), ret_sn, flow_match.src_mac.c_str(), flow_match.dst_mac.c_str(), flow_match.src_ip.c_str(), flow_match.dst_ip.c_str(), myRoute[i].out_port);

		else
		sprintf(cmd, "curl -d '{\"switch\": \"%s\", \"name\":\"flow-mod-%d\",\"eth_src\":\"%s\",\"eth_dst\":\"%s\", \"eth_type\":\"0x0800\", \"ipv4_src\":\"%s\", \"ipv4_dst\":\"%s\", \"cookie\":\"0\", \"priority\":\"1\", \"in_port\":\"%d\",\"active\":\"true\", \"actions\":\"output=%d\"}' http://localhost:8080/wm/staticflowpusher/json"
			, myRoute[i].dpid.c_str(), ret_sn, flow_match.src_mac.c_str(), flow_match.dst_mac.c_str(), flow_match.src_ip.c_str(), flow_match.dst_ip.c_str(), myRoute[i].in_port, myRoute[i].out_port);







		cout << "[" << cmd << "]" << endl;
		system(cmd);
		ret_sn++;
	}

	return ret_sn;
}

	
vector < struct route > getRoute(string src, string dst){
	string src_switch = src;
	int src_port = 0;
	string dst_switch = dst;
	int dst_port = 0;

	map < string, pair < int, int > > ret;
	vector < struct route > ret_vector;
	string current_switch;
	int current_index = -1;


	char cmd[256];
	sprintf(cmd, "curl http://localhost:8080/wm/topology/route/%s/%d/%s/%d/json > out", src_switch.c_str(), src_port
		, dst_switch.c_str(), dst_port);

	cout << "[" << cmd << "]" << endl;

	system(cmd);

	ifstream inputFile("out", ifstream::in);

	char line[2048];
	memset(line, 0, 2048);


	while(!inputFile.eof()){
		vector < string > token;
		inputFile.getline(line, 2048);
		string linestr = line;


		token = cutToken(linestr, "\"");

		for(int i = 0; i < token.size(); i++){
			if(token[i].compare("switch") == 0){
				current_switch = token[i+ 2];
				if(ret.find(current_switch) == ret.end()){
					current_index++;
					struct route newRoute;

					ret[current_switch].first = -1;
					ret[current_switch].second = -1;

					newRoute.dpid = current_switch;
					newRoute.in_port = -1;
					newRoute.out_port = -1;

					ret_vector.push_back(newRoute);
				}
			}

			else if(token[i].compare("portNumber") == 0){
				if(ret_vector[current_index].in_port == -1)
					ret_vector[current_index].in_port = atoi(token[i + 1].substr(1,token[i + 1].length() - 2).c_str());
				else
					ret_vector[current_index].out_port = atoi(token[i + 1].substr(1,token[i + 1].length() - 2).c_str());

				if(ret[current_switch].first == -1)					
					ret[current_switch].first = atoi(token[i + 1].substr(1,token[i + 1].length() - 2).c_str());
				else
					ret[current_switch].second = atoi(token[i + 1].substr(1,token[i + 1].length() - 2).c_str());

			}
		}
	}


	for(int i = 0; i < ret_vector.size(); i++){
		//cout << ret_vector[i].in_port << " ---> " << ret_vector[i].dpid << " ---> " << ret_vector[i].out_port << endl;
	}

	return ret_vector;
}

vector < struct route > getRoute(string src, string dst, map < string, pair < string, int > > host_info, map < string, int > switch_info){
	string src_switch = host_info[src].first;
	int src_port = switch_info[src_switch];
	string dst_switch = host_info[dst].first;
	int dst_port = switch_info[dst_switch];

	map < string, pair < int, int > > ret;
	vector < struct route > ret_vector;
	string current_switch;
	int current_index = -1;


	char cmd[256];
	sprintf(cmd, "curl http://localhost:8080/wm/topology/route/%s/%d/%s/%d/json > out", src_switch.c_str(), src_port
		, dst_switch.c_str(), dst_port);


	system(cmd);

	ifstream inputFile("out", ifstream::in);

	char line[2048];
	memset(line, 0, 2048);


	while(!inputFile.eof()){
		vector < string > token;
		inputFile.getline(line, 2048);
		string linestr = line;


		token = cutToken(linestr, "\"");

		for(int i = 0; i < token.size(); i++){
			if(token[i].compare("switch") == 0){
				current_switch = token[i+ 2];
				if(ret.find(current_switch) == ret.end()){
					current_index++;
					struct route newRoute;

					ret[current_switch].first = -1;
					ret[current_switch].second = -1;

					newRoute.dpid = current_switch;
					newRoute.in_port = -1;
					newRoute.out_port = -1;

					ret_vector.push_back(newRoute);
				}
			}

			else if(token[i].compare("portNumber") == 0){
				if(ret_vector[current_index].in_port == -1)
					ret_vector[current_index].in_port = atoi(token[i + 1].substr(1,token[i + 1].length() - 2).c_str());
				else
					ret_vector[current_index].out_port = atoi(token[i + 1].substr(1,token[i + 1].length() - 2).c_str());

				if(ret[current_switch].first == -1)					
					ret[current_switch].first = atoi(token[i + 1].substr(1,token[i + 1].length() - 2).c_str());
				else
					ret[current_switch].second = atoi(token[i + 1].substr(1,token[i + 1].length() - 2).c_str());

			}
		}
	}


	ret_vector[0].in_port = host_info[src].second;
	ret_vector[ret_vector.size() - 1].out_port = host_info[dst].second;

	for(int i = 0; i < ret_vector.size(); i++){
		//cout << ret_vector[i].in_port << " ---> " << ret_vector[i].out_port << endl;
	}

	return ret_vector;
}




map < string, int > getSwitchPort(){
	system("curl http://localhost:8080/wm/topology/links/json > out");
	map < string, int > ret;
	string current_switch;
	int current_port;

	ifstream inputFile("out", ifstream::in);

	char line[2048];
	memset(line, 0, 2048);



	while(!inputFile.eof()){
		vector < string > token;
		inputFile.getline(line, 2048);

		string linestr = line;


		token = cutToken(linestr, "\"");

		for(int i = 0; i < token.size(); i++){
			if(token[i].compare("src-switch") == 0 || token[i].compare("dst-switch") == 0){
				current_switch = token[i+ 2];
			}

			else if(token[i].compare("src-port") == 0 || token[i].compare("dst-port") == 0){
				ret[current_switch] = atoi(token[i + 1].substr(1,token[i + 1].length() - 2).c_str());

			}
		}
	}

	return ret;
}


map < string, pair < string, int > >  getHost(){
	system("curl http://localhost:8080/wm/device/ > out");
	map < string, pair < string, int > > ret;
	string current_host;
	pair < string, int > current_attach;
	
	ifstream inputFile("out", ifstream::in);

	char line[2048];
	memset(line, 0, 2048);


	while(!inputFile.eof()){
		vector < string > token;
		inputFile.getline(line, 2048);
		string linestr = line;


		token = cutToken(linestr, "\"");

		for(int i = 0; i < token.size(); i++){
			if(token[i].compare("mac") == 0){
				current_host = token[i+ 2];
			}
			else if(token[i].compare("switchDPID") == 0){
				current_attach.first = token[i + 2];
			}

			else if(token[i].compare("port") == 0){
				current_attach.second = atoi(token[i + 1].substr(1,token[i + 1].length() - 2).c_str());
				ret[current_host] = current_attach;

			}
		}
	}

	return ret;
}

int getPortNum(string portName){
	system("ovs-ofctl show br0 > out");
        ifstream inputFile("out", ifstream::in);

        char line[256];
        while(!inputFile.eof()){
                vector < string > token;
                inputFile.getline(line, 256);
                string lineStr = line;

                token = cutToken(lineStr, " ");


                if(token[0].find(portName) != string::npos){
                        char port[128];
                        strcpy(port, token[0].c_str());
                        char* portNum = strtok(port, "(");
                        return atoi(portNum);
                }
        }

        return -1;
}

pair < string, string > startNF(string name, string exec_cmd, int num_nf){
	char cmd[1024];
	char container_ID[512];
	char port_name[512];


	pair < string, string > ret;

	sprintf(cmd, "docker run --net=none --privileged=true -itd %s > out", name.c_str());
	system(cmd);

	ifstream inputfile("out", ifstream::in);

	inputfile >> container_ID;


	inputfile.close();

	memset(cmd, 0, 1024);
	sprintf(cmd, "./ovs-docker add-port br0 eth0 %s > out", container_ID);
	system(cmd);

	inputfile.open("out", ifstream::in);
	inputfile >> port_name;
	inputfile.close();

	memset(cmd, 0, 1024);
	//sprintf(cmd, "docker exec %s ifconfig eth0 172.17.0.%d/16", container_ID, (rand() % 255));
	sprintf(cmd, "docker exec %s ifconfig eth0 10.1.1.%d/24", container_ID, (num_nf + 6));
	system(cmd);


	memset(cmd, 0, 1024);
	sprintf(cmd, "docker exec %s route add default gw 10.1.1.6", container_ID);
	system(cmd);


	ret.first = container_ID;
	ret.second = port_name;

	if(exec_cmd.length() > 0){
		exec_cmd += "&";
		sprintf(cmd, "docker exec %s %s", container_ID, exec_cmd.c_str());

		cout << "The command to docker : " << cmd << endl;

		system(cmd);
	}

	return ret;
}

pair < string, string > startNF(int vm_ID, int num_nf){
	ifstream inputFile("nf.conf", ifstream::in);
	char line[256];
	pair < string, string > container_info;

	while(!inputFile.eof()){
		vector < string > token;
		inputFile.getline(line, 256);
		string lineStr = line;

		token = cutToken(lineStr, "\t ");

		if(token.size() < 2) break;


		if(atoi(token[0].c_str()) == vm_ID && atoi(token[1].c_str()) == 0){
			string exec_cmd;
			string name = token[3];


			for(int i = 4; i < token.size(); i++){
				exec_cmd += token[i];
				exec_cmd += " ";
			}

			container_info = startNF(name, exec_cmd, num_nf);
		}
	}

	return container_info;
}
			

int stopNF(pair < string, string > container_info){
	char cmd[1024];
	
	sprintf(cmd, "docker stop %s", container_info.first.c_str());
	system(cmd);

	memset(cmd, 0, 1024);

	sprintf(cmd, "ovs-vsctl del-port br0 %s", container_info.second.c_str());
	system(cmd);
}

map < string, string > getDpidMap(){
	map < string, string > ret;
	string currentIP;

	system("curl http://localhost:8080/wm/core/controller/switches/json > out");

	ifstream inputFile("out", ifstream::in);

	char line[2048];
	memset(line, 0, 2048);


	while(!inputFile.eof()){
		vector < string > token;
		inputFile.getline(line, 2048);
		string linestr = line;


		token = cutToken(linestr, "\"");

		for(int i = 0; i < token.size(); i++){
			if(token[i].compare("inetAddress") == 0){
				currentIP = token[i + 2].substr(1, token[i + 2].find_first_of(":", 0) - 1);
			}
			if(token[i].compare("switchDPID") == 0){
				ret[currentIP] = token[i + 2];
			}
				
		}
	}


	return ret;
}

/* set routing for a single hop. flow_match is the flow we want to match, dpid is the mapping from
switch's IP to its DPID (mac address), src_data_mac/dst_data_mac are the mac address of host in
data plane, dst_control_ip/dst_control_ip are the IP address of the host in control plane, and
src_container/dst_container are the container ID of the NF running in the hosts.*/

/*src_container.first = container ID, and src_container.second = port name. */

int setHop(struct match flow_match
	, string src_data_mac
	, string dst_data_mac
	, int src_nf_port, int dst_nf_port){

	vector < struct route > myRoute;


	if(src_data_mac.compare(dst_data_mac) == 0){
		struct route newRoute;
		newRoute.dpid = src_data_mac;
		newRoute.in_port = 9;
		newRoute.out_port = dst_nf_port;

		myRoute.push_back(newRoute);
		installSDNRules(myRoute, flow_match, 2);
		return 0;
		
	}

	
	myRoute = getRoute(src_data_mac, dst_data_mac);


		
	if(src_nf_port >= 0){
		myRoute[0].in_port = src_nf_port;
	}
	else myRoute[0].in_port = 0;

	if(dst_nf_port >= 0){
		myRoute[myRoute.size() - 1].out_port = dst_nf_port;
	}
	else myRoute[myRoute.size() - 1].out_port = 0;
	
	installSDNRules(myRoute, flow_match, 1);
	return 0;
}

string getMacAddr(string interface){
	char cmd[128];
	string ret;
	sprintf(cmd, "/sys/class/net/%s/address", interface.c_str());

	ifstream inputFile(cmd, ifstream::in);

	inputFile >> ret;
	return ret;
}

