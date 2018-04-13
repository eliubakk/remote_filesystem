#include <iostream>
#include <cstdlib>
#include <cassert>
#include <cstring>
#include <string>
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
    fs_create("user1", "password1", session, seq++, "/ho\0me", 'd');
    assert(fs_create("user1", "password1", session, seq++, "/EECS482EECS482EECS482EECS482EECS482EECS482EECS482", 'd') == 0);
    assert(fs_create("user1", "password1", session, seq++, "/EECS482EECS482EECS482EECS482EECS482EECS482EECS482/EECS482EECS482EECS482EECS482EECS482EECS482EECS482", 'd') == 0);
    string path = "/EECS482EECS482EECS482EECS482EECS482EECS482EECS482/EECS482EECS482EECS482EECS482EECS482EECS482EECS482/EECS482EECS482EECS482EECS48";
    cout << "string length: " << path.size() << endl;
    assert(fs_create("user1", "password1", session, seq++, "/EECS482EECS482EECS482EECS482EECS482EECS482EECS482/EECS482EECS482EECS482EECS482EECS482EECS482EECS482/EECS482EECS482EECS482EECS48", 'd') == 0);
    assert(fs_create("user1", "password1", session, seq++, "/EECS482EECS482EECS482EECS482EECS482EECS482EECS482/EECS482EECS482EECS482EECS482EECS482EECS482EECS482/EECS482EECS482EECS482EECS482", 'd') == -1);
    assert(fs_create("user1", "password1", session, seq++, "/ho me", 'd') == -1);

    assert(fs_create("user1", "password1", session, seq++, "/msg", 'l') == -1);
    assert(fs_create("user1", "password1", session, seq++, "/msg", ' ') == -1); 

    char message[FS_BLOCKSIZE];
    char output[FS_BLOCKSIZE];
    memset(message, 'A', FS_BLOCKSIZE);
    memset(output, 0, FS_BLOCKSIZE);
    message[3] = '\0';

    fs_create("user1", "password1", session, seq++, "/hw", 'f');
    fs_writeblock("user1", "password1", session, seq++, "/hw", 0, message);
    fs_readblock("user1", "password1", session, seq++, "/hw", 0, output);

    for (unsigned int i = 0; i < FS_BLOCKSIZE; ++i){
        //cout << output[i];
    }
    //cout << endl;
    cout << output << endl;
}