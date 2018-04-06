#include <iostream>
#include <cstdlib>
#include <cassert>
#include <cstring>
#include "fs_client.h"

using namespace std;

int main(int argc, char *argv[]){
	char *server;
	int server_port;
	unsigned int session = 6, seq = 0;

	if (argc != 3) {
		cout << "error: usage: " << argv[0] << " <server> <serverPort>\n";
		exit(1);
	}
	server = argv[1];
	server_port = atoi(argv[2]);

	fs_clientinit(server, server_port);
	fs_session("user1", "password1", &session, seq++);
	fs_create("user1", "password1", session, seq++, "/home", 'd');
	fs_create("user1", "password1", session, seq++, "/home/Documents", 'd');
	fs_create("user1", "password1", session, seq++, "/bin", 'd');
	fs_create("user1", "password1", session, seq++, "/home/Music", 'd');

	char writedata[FS_BLOCKSIZE]; 
	memset(writedata, 0, FS_BLOCKSIZE);
	char readdata[FS_BLOCKSIZE];
	memset(readdata, 0, FS_BLOCKSIZE);
	fs_create("user1", "password1", session, seq++, "/home/Documents/spaces", 'f');
	for(unsigned int i = 0; i < 10; ++i){
		writedata[i] = ' ';
	}
	strcpy(writedata + 10, "spaces");
	for(unsigned int i = 16; i < FS_BLOCKSIZE; ++i){
		writedata[i] = ' ';
	}
	fs_writeblock("user1", "password1", session, seq++, "/home/Documents/spaces", 0, writedata);
	fs_readblock("user1", "password1", session, seq++, "/home/Documents/spaces", 0, readdata);
	for(unsigned int i = 0; i < 10; ++i){
		assert(writedata[i] == readdata[i]);
	}
	assert(readdata[10] == 's');
	assert(readdata[11] == 'p');
	assert(readdata[12] == 'a');
	assert(readdata[13] == 'c');
	assert(readdata[14] == 'e');
	assert(readdata[15] == 's');
	for(unsigned int i = 16; i < FS_BLOCKSIZE; ++i){
		assert(writedata[i] == readdata[i]);
	}

	fs_create("user1", "password1", session, seq++, "/nulls", 'f');
	for(unsigned int i = 0; i < 10; ++i){
		writedata[i] = '\0';
	}
	strcpy(writedata + 10, "nulls");
	for(unsigned int i = 16; i < FS_BLOCKSIZE; ++i){
		writedata[i] = '\0';
	}
	fs_writeblock("user1", "password1", session, seq++, "/nulls", 0, writedata);
	fs_readblock("user1", "password1", session, seq++, "/nulls", 0, readdata);
	for(unsigned int i = 0; i < 10; ++i){
		assert(writedata[i] == readdata[i]);
	}
	assert(readdata[10] == 'n');
	assert(readdata[11] == 'u');
	assert(readdata[12] == 'l');
	assert(readdata[13] == 'l');
	assert(readdata[14] == 's');
	for(unsigned int i = 15; i < FS_BLOCKSIZE; ++i){
		assert(writedata[i] == readdata[i]);
	}
}