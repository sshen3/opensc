#include "opensc.h"

int main(){
	opensc sc("localhost");

	vector < struct host_info > hosts = sc.get_host_list();

	for(int i = hosts.size() - 1; i >= 0; i--){
		sc.disconnect_host(i);
	}
}

