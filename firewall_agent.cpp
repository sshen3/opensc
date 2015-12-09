#include "firewall_agent.h"

#include <unistd.h>

int firewall_agent::firewall_handler(char* json_str){
	cout << "my test" << json_str << endl;
	return 0;
}

firewall_agent::firewall_agent(){
	VM_msg test_msg;
	firewallInterface.agent_handler = firewall_agent::firewall_handler;
	//myAgent.handler(test_msg);
	firewallInterface.runner(false, true, "140.109.21.50", 1);
	sleep(10);
}


