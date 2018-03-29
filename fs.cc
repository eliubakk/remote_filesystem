#include "fs_server.h"
#include "fs_user.h"
#include "filesystem.h"

#include <iostream>
#include <thread>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>

using namespace std;

#define INT_MAXDIGITS 10
//FS_WRITEBLOCK <session> <sequence> <pathname> <block><NULL><data>
#define FS_MAXDECRYPT 13 + 1 + INT_MAXDIGITS + 1 + INT_MAXDIGITS + 1 + FS_MAXPATHNAME + 1 + 3 + 1 + 512;
#define FS_MAXREQUESTNAME 13

void request_handler(int client);

int read_bytes(int client, char* buf, unsigned int length, bool use_delim = false, char delim = '\0');

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

	while(true){
		int client = accept(sock, (struct sockaddr*) nullptr, (socklen_t*)nullptr);
		thread request(request_handler, client);
		request.detach();
	}
}

void request_handler(int client){
	cout << "New thread client: " << client << endl;
	char username[FS_MAXUSERNAME + 1];
	if(!read_bytes(client, username, sizeof(username), true, ' ')){
		close(client);
		return;
	}
	cout << "username: " << username << endl;

	if(filesystem::users.find(username) == filesystem::users.end()){
		close(client);
		return;
	}

	char request_size[INT_MAXDIGITS + 1];
	if(!read_bytes(client, request_size, sizeof(request_size), true)){
		close(client);
		return;
	}
	cout << "request size: " << request_size << endl;
	unsigned int encrypted_size = stoi(request_size);
	cout << "encrypted_size: " << encrypted_size << endl; 
	char encrypted_message[encrypted_size + 1];
	read_bytes(client, encrypted_message, sizeof(encrypted_message) - 1);
	cout << "bytes read" << endl;
	//close(client);

	cout << "encrypted_message: " << encrypted_message << endl;

	unsigned int decrypted_size = 0;
	char* decrypted_message = 
				(char*)fs_decrypt(filesystem::users[username]->password(), 
				(void*)encrypted_message, 
				encrypted_size, &decrypted_size);
	if(decrypted_message == nullptr){
		return;
	}
	//cout << "decrypted_message: " << decrypted_message << endl;

	char request[decrypted_size + 1];
	strncpy(request, decrypted_message, decrypted_size);
	request[decrypted_size] = '\0';
	//char* request_name = strtok(decrypted_message, ' ');
	cout << "request: " << request << endl;

	switch(request[3]){
		case 'S':
			filesystem::session_response(client, username, request, decrypted_size + 1);
			break;
		//TODO: other requests.
	};
	close(client);
}

int read_bytes(int client, char* buf, unsigned int length, bool use_delim, char delim){
	memset(buf, 0, length);
	int return_val = -1;
	for (unsigned int i = 0; i < length && return_val != 0; ++i){
		return_val = read(client, buf + i, 1);
		if (use_delim && buf[i] == delim){
			buf[i] = '\0';
			break;
		}
	}
	return return_val;
}