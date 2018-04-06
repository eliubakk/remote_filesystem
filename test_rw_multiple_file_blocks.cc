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

	fs_create("user1", "password1", session, seq++, "/home/Music/edm.flac", 'f');
	assert(fs_readblock("user1", "password1", session, seq++, "/home/Music/edm.flac", 3, writedata) == -1);
	assert(fs_writeblock("user1", "password1", session, seq++, "/home/Music/edm.flac", 1, writedata) == -1);
	memset(writedata, 0xC, FS_BLOCKSIZE);
	fs_writeblock("user1", "password1", session, seq++, "/home/Music/edm.flac", 0, writedata);
	memset(writedata, 0xB, FS_BLOCKSIZE);
	fs_writeblock("user1", "password1", session, seq++, "/home/Music/edm.flac", 1, writedata);
	fs_readblock("user1", "password1", session, seq++, "/home/Music/edm.flac", 1, readdata);
	for(unsigned int i = 0; i < FS_BLOCKSIZE; ++i){
		assert(readdata[i] == 0xB);
	}
	fs_readblock("user1", "password1", session, seq++, "/home/Music/edm.flac", 0, readdata);
	for(unsigned int i = 0; i < FS_BLOCKSIZE; ++i){
		assert(readdata[i] == 0xC);
	}
}