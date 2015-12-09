#include <iostream>
#include <vector>

#include "interface.h"

using namespace std;

class opensc{
public:
	opensc(string _server_ip);
	vector < struct host_info > get_host_list();
	void disconnect_host(int host_ID);
	void add_nf(int host_num, int nf_num);
	void add_flow(string src_mac, string dst_mac, string src_ip, string dst_ip
		, string src_port, string dst_port);
	vector < struct flow_info > get_flow();
	void add_service(int service_i, int host_i, int nf_i);
private:
	string server_ip;
	int openSock();
	int closeSock(int sock);

};
