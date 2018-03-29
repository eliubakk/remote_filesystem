#include "filesystem.h"
#include <cassert>
#include <cstring>
#include <string>
#include <sstream>
#include <sys/socket.h>
#include <iostream>

using namespace std;

filesystem::entry::entry(uint32_t inode_in) 
: inode_block(inode_in), entries(nullptr) {}

filesystem::~filesystem(){
	for(auto user : users)
		delete user.second;
}

bool filesystem::add_user(const string& username, const string& password){
	if(users.find(username) != users.end())
		return false;
	users[username] = new fs_user(username.c_str(), password.c_str());
	return true;
}

bool filesystem::user_exists(const string& username){
	return (users.find(username) != users.end());
}

const char* filesystem::password(const string& username){
	return users[username]->password();
}

void filesystem::session_response(int client, const char *username, char *request,
							unsigned int request_size){
	cout << "building session response..." << endl;
	//FS_SESSION <session> <sequence><NULL>
	char *request_name = strtok(request, " ");
	if(strcmp(request_name, "FS_SESSION"))
		return;
	cout << "request_name: " << request_name << endl;	

	strtok(nullptr, " ");
	char* request_sequence = strtok(nullptr, "\0");
	unsigned int sequence = stoi(request_sequence);

	unsigned int session = users[username]->create_session(sequence);

	//<size><NULL><session> <sequence><NULL>
	ostringstream response;
	response << to_string(session) << " " << to_string(sequence);
	send_response(client, username, response.str());
	return;
}

void filesystem::readblock_response(int client, const char *username, char *request,
							unsigned int request_size){
	assert(false);
}

void filesystem::writeblock_response(int client, const char *username, char *request,
							unsigned int request_size){
	assert(false);
}

void filesystem::create_response(int client, const char *username, char *request,
							unsigned int request_size){
	assert(false);
}

void filesystem::delete_response(int client, const char *username, char *request,
							unsigned int request_size){
	assert(false);
}

void filesystem::send_response(int client, const char *username, string response){
	cout << "response built" << endl;

	unsigned int cipher_size = 0;
	const char* cipher = (char*) fs_encrypt(
		users[username]->password(), (void*)response.c_str(),
		response.length() + 1, &cipher_size);

	cout << "response encrypted" << endl;
	cout << "cipher_size: " << cipher_size << endl;
	string header(to_string(cipher_size));
	send(client, header.c_str(), header.length() + 1, 0);
	send(client, cipher, cipher_size, 0);

	cout << "session response sent" << endl;
}