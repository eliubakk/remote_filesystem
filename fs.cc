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
#define FS_MAXREQUESTNAME 13

// ------ Function Declarations ------ //
void request_handler(filesystem* fs, int client);
int read_bytes(int client, char* buf, 
				unsigned int length, 
				bool use_delim = false,
				char delim = '\0');

int main(int argc, char **argv){
	unsigned int port = 0;

	//Check validity of command line args, set port # if given
	if(argc > 2){
		return -1;
	}else if(argc == 2){
		port = stoi(argv[1]);
	}

	filesystem fs;

	//Read in users
	string username, password;
	while(cin >> username >> password){
		fs.add_user(username, password);
	}

	//Make socket and allow it to reuse addresses
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, NULL, 0);

	//Assign socket to a port
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port);
	if(bind(sock, (struct sockaddr*) &addr, sizeof(addr)) == -1){
		exit(1);
	}

	//Get port number and print
	unsigned int addr_length =  sizeof(addr);
	if(getsockname(sock, (struct sockaddr*) &addr, &addr_length) == -1){
		exit(1);
	}
	cout_lock.lock();
	cout << "\n@@@ port " << ntohs(addr.sin_port) << endl;
	cout_lock.unlock();

	listen(sock, 10);

	socklen_t adder_len = sizeof(addr);
	//Continuously listen for, accept, and handle requests
	while(true){
		int client = accept(sock, (struct sockaddr*) &addr, (socklen_t*)&adder_len);
		if(client == -1){
			exit(EXIT_FAILURE);
		}
		thread request(request_handler, &fs, client);
		request.detach();
	}
}

//Request handler
void request_handler(filesystem* fs, int client){
	//Read username from cleartext request header
	char username[FS_MAXUSERNAME + 1];
	if(!read_bytes(client, username, sizeof(username), true, ' ')){
		close(client);
		return;
	}

	//Close connection if user does not exist in filesystem
	if(!fs->user_exists(username)){
		close(client);
		return;
	}

	//Read size of encrypted request from cleartext request header
	char request_size[INT_MAXDIGITS + 1];
	if(!read_bytes(client, request_size, sizeof(request_size), true)){
		close(client);
		return;
	}
	//? probably make sure is digit?
	if(request_size[INT_MAXDIGITS] != '\0'){
		close(client);
		return;
	}
	
	//try catch if fail, also unsigned?
	unsigned int encrypted_size = 0;
	try{
		encrypted_size = stoul(request_size);
	} catch(...){
		close(client);
		return;
	}

	//Read in encrypted request
	char encrypted_message[encrypted_size + 1];
	read_bytes(client, encrypted_message, encrypted_size);

	//Decrypt request
	unsigned int decrypted_size = 0;
	char* decrypted_message = 
				(char*)fs_decrypt(fs->password(username), 
							(void*)encrypted_message, 
							encrypted_size, &decrypted_size);
	if(decrypted_message == nullptr){
		close(client);
		return;
	}

	//Call function to actually perform the create, read, write, or delete on the filesystem
	fs->handle_request(client, username, decrypted_message, decrypted_size);
	close(client);
}


//Helper function, read data from client into a buffer
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