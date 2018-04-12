#include <iostream>
#include <cstdlib>
#include <cstring>
#include <vector>
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

    vector<string>filename = {"/hw0","/hw1", "/hw2", "/hw3", "/hw4", "/hw5", "/hw6", "/hw7", "/hw8", "/hw9", "/hw10", "/hw11","/hw12", "/hw13", "/hw14"};

    char writedata[FS_BLOCKSIZE];
    memset(writedata, 0, FS_BLOCKSIZE); 
    strcpy(writedata, "Hello :)");
    char readdata[FS_BLOCKSIZE];
    memset(readdata, 0, FS_BLOCKSIZE);

    fs_clientinit(server, server_port);
    fs_session("user1", "password1", &session, seq++);

    for (int i = 0; i < 15; ++i){
        assert(fs_create("user1", "password1", session, seq++, filename[i].c_str(), 'f') == 0);
        assert(fs_writeblock("user1", "password1", session, seq++, filename[i].c_str(), 0, writedata) == 0);
    } 
    for (int i = 0; i < 8; ++i){
        assert(fs_delete("user1", "password1", session, seq++, filename[i].c_str()) == 0);
        assert(fs_readblock("user1", "password1", session, seq++, filename[11].c_str(), 0, readdata) == 0);
    }
    for (int i = 3; i < 15; ++i){
        if (i < 8){
            assert(fs_create("user1", "password1", session, seq++, filename[i].c_str(), 'f') == 0);        
        }
        if (i >= 8 && i < 13){
            assert(fs_create("user1", "password1", session, seq++, filename[i].c_str(), 'f') == -1);            
        }
        else{
            assert(fs_create("user1", "password1", session, seq++, filename[i].c_str(), 'f') == -1); 
        }
        assert(fs_writeblock("user1", "password1", session, seq++, filename[i].c_str(), 0, writedata) == 0);                 
    }
    assert(fs_delete("user1", "password1", session, seq++, filename[3].c_str()) == 0);
    assert(fs_delete("user1", "password1", session, seq++, filename[4].c_str()) == 0);
}
