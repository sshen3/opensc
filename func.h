#include <iostream>
#include <string>
#include <vector>
#include <map>

using namespace std;

struct route{
	string dpid;
	int in_port;
	int out_port;
};

struct match{
	string src_mac;
	string dst_mac;
	string src_ip;
	string dst_ip;
	int src_port;
	int dst_port;
};

vector < string > cutToken(string input, string ss);
int getPortNum(string portName);
pair < string, string > startNF(string name, string exec_cmd, int num_nf);
pair < string, string > startNF(int vm_ID, int num_nf);
int stopNF(pair < string, string > container_info);
map < string, pair < string, int > > getHost();
map < string, int > getSwitchPort();
vector < struct route > getRoute(string src, string dst, map < string, pair < string, int > > host_info, map < string, int > switch_info);
vector < struct route > getRoute(string src, string dst);
int installSDNRules(vector < struct route > myRoute, struct match flow_match, int sn);
map < string, string > getDpidMap();
int setHop(struct match flow_match
	, string src_data_mac
	, string dst_data_mac
	, int src_nf_port, int dst_nf_port);
string getMacAddr(string interface);
