#include <iostream>
#include <string>	
#include "ServerTCP.h"

int main(int argc, char** argv) {
	
	ServerTCP main;
	std::string ip = "";
	
	std::cout << "Enter Server IP: ";
	std::getline(std::cin, ip);

	if (main.Start(ip.c_str(),"3995",20)) {
		main.Run();
	}

	return 0;
}