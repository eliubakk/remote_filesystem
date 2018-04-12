#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include "fs_client.h"
#include "fs_param.h"

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

    char writedata[FS_BLOCKSIZE]; 
    char data = 'A';
    memset(writedata, data, FS_BLOCKSIZE);

    fs_clientinit(server, server_port);
    fs_session("user1", "password1", &session, seq++);

    assert(fs_create("user1", "password1", session, seq++, "/msg", 'l') == -1);
    assert(fs_create("user1", "password1", session, seq++, "/msg", ' ') == -1);       

    char dirname[14] = "/message0";
    for (unsigned int i = 0; i < FS_MAXFILEBLOCKS * 8; ++i){
	strcpy(dirname + 9, to_string(i).c_str());
    	fs_create("user1", "password1", session, seq++, dirname, 'd');
    }
    assert(fs_create("user1", "password1", session, seq++, "/shouldn'twork", 'd') == -1); 
}
