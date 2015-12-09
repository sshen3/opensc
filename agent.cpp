#include "interface.h"
#include "func.h"

#include <unistd.h>
#include <string.h>

#include <json-c/json.h>

int server_handler(char* json_str){

	struct json_object *root, *cmd_j, *host_ID_j, *vm_ID_j;
	int host_ID, vm_ID;
	root = json_tokener_parse(json_str);

	json_object_object_get_ex(root, "cmd", &cmd_j);
	json_object_object_get_ex(root, "host ID", &host_ID_j);
	json_object_object_get_ex(root, "vm ID", &vm_ID_j);

	string cmd(json_object_get_string(cmd_j));
	host_ID = json_object_get_int(host_ID_j);
	vm_ID = json_object_get_int(vm_ID_j);

	cout << "cmd = " << cmd << ", host ID = " << host_ID << ", vm ID = " << vm_ID << endl;

	if(cmd.compare("start_nf") == 0){
		if(vm_ID == 0){
			cout << "starting Traffic Shaper" << endl;
			cout << "sudo docker run ubuntu:tshaper /root/click-2.0.1/userlevel/click -e \"FromDevice(eth0)->IPPrint(ok)->Queue()->BandwidthShaper(1Mbps)->Discard()\"" << endl;
			startNF(0, 0);
		}
		else if(vm_ID == 1){
			cout << "starting DHCP" << endl;
			cout << "sudo docker run ubuntu:dhcp /root/click-2.0.1/userlevel/click -e \"FromDevice(eth0)->IPPrint(ok)->Queue()->BandwidthShaper(1Mbps)->Discard()\"" << endl;
		}
		else if(vm_ID == 2){
			cout << "starting DNS" << endl;
			cout << "sudo docker run ubuntu:dns /root/click-2.0.1/userlevel/click -e \"FromDevice(eth0)->IPPrint(ok)->Queue()->BandwidthShaper(1Mbps)->Discard()\"" << endl;
		}

		return 0;
	}

	if(cmd.compare("disconnect_host") == 0){
		return 1;
	}

}

int main(int argc, char *argv[]){
	interface hostInterface;
	string msg;

	if(argc < 2){
		cout << "Usage: ./agent <controller IP>" << endl;
		exit(1);
	}


	//msg.vm_ID = 0;
	//strcpy(msg.cmd.name, "test");
	msg = "start_nf 1 1";

	hostInterface.agent_handler = server_handler;

	hostInterface.runner(false, true, "140.109.18.186", 1);
	//myAgent.send_msg(msg);
	//myAgent.agent_runner("140.109.21.50");
}
