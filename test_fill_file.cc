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

    char readdata[FS_BLOCKSIZE];
    memset(readdata, 0, FS_BLOCKSIZE);

    fs_clientinit(server, server_port);
    fs_session("user1", "password1", &session, seq++);
    fs_create("user1", "password1", session, seq++, "/message", 'f');
    for(unsigned int block = 0; block < FS_MAXFILEBLOCKS; ++block){
        fs_writeblock("user1", "password1", session, seq++, "/message", block, writedata);
        memset(writedata, data, FS_BLOCKSIZE);
    }
    
    data = 'A';
    for(unsigned int block = 0; block < FS_MAXFILEBLOCKS; ++block){
        fs_readblock("user1", "password1", session, seq++, "/message", block, readdata);
        for(unsigned int i = 0; i < FS_BLOCKSIZE; ++i){
            assert(readdata[i] == data);
        }
    }

    assert(fs_writeblock("user1", "password1", session, seq++, "/message", FS_MAXFILEBLOCKS, writedata) == -1);
}