#include "fs_server.h"
#include "fs_user.h"
#include "filesystem.h"

#include <iostream>
#include <pthread.h>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

using namespace std;

int main(int argc, char **argv){
	unsigned int port = 0;

	if(argc > 2){
		return -1;
	}else if(argc == 2){
		port = stoi(argv[1]);
	}

	//read in users
	string username, password;
	while(cin >> username >> password){
		filesystem::users[username] = new fs_user(username.c_str(), password.c_str());
	}

	//make socket and allow it to reuse addresses
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, NULL, 0);

	//assign socket to a port
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port);
	if(bind(sock, (struct sockaddr*) &addr, sizeof(addr)) == -1){
		exit(1);
	}

	//get port number and print
	unsigned int addr_length =  sizeof(addr);
	if(getsockname(sock, (struct sockaddr*) &addr, &addr_length) == -1){
		exit(1);
	}
	cout << "\n@@@ port " << ntohs(addr.sin_port) << endl;

	listen(sock, 10);
}


