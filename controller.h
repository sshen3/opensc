#include "interface.h"

class controller{
public:
	controller();
	static int server_handler(char* json_str);
private:
	interface controllerInterface; 
};
