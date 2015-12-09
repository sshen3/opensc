#include <iostream>
#include <fstream>
#include <stdint.h>
#include <map>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <vector>


#define MAX_NUM_HOST 10
#define MAX_NUM_NF 10
#define MAX_NUM_SERVICE 10
#define MAX_CHAIN_LEN 10
#define VALUE_SIZE 512

//decltype int (*server_handler)(VM_msg);


using namespace std;

struct vm_cmd{
	char name[128];
	char value[VALUE_SIZE];
};

struct VM_msg{
	uint32_t vm_ID;
	unsigned long host_addr;
	bool active;
	struct vm_cmd cmd;
};

struct nf_info{
	int nf_id;
	char instance_id[128];
	char port[32];
	int portNum;
};


struct host_info{
	unsigned long host_addr;
	char mac_addr[64];
	int is_ovs;
	struct nf_info active_nf[MAX_NUM_NF];
};


struct service_info{
	int flow_id;
	int host_chain[MAX_CHAIN_LEN]; /* the ID of the servers that host NFs in the service chain. */
	int nf_chain[MAX_CHAIN_LEN];  /* the ID of the NFs in the service chain. */
	int len;
};

struct flow_info{
	struct sockaddr_in src_ip;
	struct sockaddr_in dst_ip;
	uint32_t src_port;
	uint32_t dst_port;
	char src_mac[64];
	char dst_mac[64];
};

typedef int (*handler)(char* json_str); // function pointer type
typedef map<unsigned long, handler> handler_map;


/*
typedef enum{
	START,
	STOP
} vm_cmd;
*/

class interface{
public:
	interface();
	~interface();
	void agent_runner(string server_ip, int is_ovs);  /* Open the socket at host side. */
	void server_runner();	/* Open the socket at controller side. */
	void runner(bool is_server, bool is_client, string addr, int is_ovs); 
	int send_msg(string msg); /* Send the message to the socket process. */ 
	int (*agent_handler)(char* json_str);
	handler_map server_handler_map;
private:
	//struct VM_msg* msg_to_server[MAX_NUM_HOST];
	//struct VM_msg* msg_from_server[MAX_NUM_HOST];
	//struct VM_msg* msg_to_host[MAX_NUM_HOST];
        //struct VM_msg* msg_from_host[MAX_NUM_HOST];
	struct host_info* host[MAX_NUM_HOST];   /* the information of the hosts that have ever connected to the controller */
	bool* active_client[MAX_NUM_HOST]; /* which host is active */
	struct service_info* service[MAX_NUM_SERVICE];  /* service chain information */
	int fd_down[MAX_NUM_HOST][2];   /* the tunnel to/from the socket process */
	pid_t pid, pid2;
	struct nf_info active_nf[MAX_NUM_NF];  /* the information of active NFs */

	vector < struct flow_info > flows;

	void shell();    /* active the command line shell, and may be removed later */
	void save_flow(string flow_info);   /* save the flow information to a file */
	void load_flow();   /*load flow information from a file */
	int host_handler(char* json_str, char* ret_handle);
	void set_graphic();
};

