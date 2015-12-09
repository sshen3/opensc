#include "controller.h"

#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int controller::server_handler(char* json_str){
	cout << "get a message = " << json_str << endl;
}

controller::controller(){
	struct sockaddr_in match_addr;
	string msg;

	msg = "start_nf 0 2";

	inet_aton("140.109.21.50", &match_addr.sin_addr);
	controllerInterface.server_handler_map[match_addr.sin_addr.s_addr] = controller::server_handler;
	controllerInterface.runner(true, false, "", 0);

	//myAgent.send_msg(msg);

	//sleep(10);
}

int main(){
	controller myController;

	return 0;
}
