#include <iostream>
#include <cstdlib>
#include <cstring>
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

    char message[FS_BLOCKSIZE]; 
    memset(message, 0, FS_BLOCKSIZE);
    strcpy(message, "Hello :)");
    strcpy(message + 9, "Bye :)");

    char output[FS_BLOCKSIZE];
    memset(output, 0, FS_BLOCKSIZE);

    fs_clientinit(server, server_port);
    cout << "PRE_SESSION: " << session << endl;
    cout << "return val: " << fs_session("user1", "password1", &session, seq++) << endl;
    cout << "POST_SESSION: " << session << endl;
    fs_create("user1", "password1", session, seq++, "/message", 'f');
    fs_writeblock("user1", "password1", session, seq++, "/message", 0, message);
    fs_readblock("user1", "password1", session, seq++, "/message", 0, output);
    for (unsigned int i = 0; i < FS_BLOCKSIZE; ++i){
                cout << output[i];
    }
    cout << endl;
}