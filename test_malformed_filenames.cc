e <iostream>
#include <cstdlib>
#include <vector>
#include <cstring>
#include <string>
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

    char output[FS_BLOCKSIZE];
    memset(output, 0, FS_BLOCKSIZE);

    fs_clientinit(server, server_port);
    cout << "PRE_SESSION: " << session << endl;
    cout << "return val: " << fs_session("user1", "password1", &session, seq++) << endl;
    cout << "POST_SESSION: " << session << endl;

    char message[FS_BLOCKSIZE]; 
    memset(message, 0, FS_BLOCKSIZE);

    fs_create("user1", "password1", session, seq++, "/user1Home");

    fs_create("user1", "password1", session, seq++, "user1Home/noStartingSlash", 'f');
    fs_create("user1", "password1", session, seq++, "/user1Home/endingSlash/", 'f');
    fs_create("user1", "password1", session, seq++, "//user1Home/doubleStartingSlash", 'f');
    fs_create("user1", "password1", session, seq++, "/user1Home/space inFilename", 'f');
    fs_create("user1", "password1", session, seq++, "/user1Home/symbols!@#$%^&*()+-=/?><,.~`{}[]|';:_", 'f');
    
    //This one should not work
    fs_create("user1", "password1", session, seq++, "/user1Home/123456789012345678901234567890123456789012345678901234567890", 'f');
    //This one should work
    fs_create("user1", "password1", session, seq++, "/user1Home/12345678901234567890123456789012345678901234567890123456789", 'f');
}