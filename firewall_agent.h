#include "interface.h"

class firewall_agent{
public:
	firewall_agent();
private:
	interface firewallInterface;
	static int firewall_handler(char* json_str);
};
