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
    fs_create("user1", "password1", session, seq++, "/hw_sol", 'f');

    char output[FS_BLOCKSIZE];
    memset(output, 0, FS_BLOCKSIZE);

    assert(fs_readblock("user1", "password1", session, seq++, "/hw_sol", 0, output) == -1);
}