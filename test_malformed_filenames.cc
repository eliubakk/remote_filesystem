#include <iostream>
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

    fs_clientinit(server, server_port);
    fs_session("user1", "password1", &session, seq++);

    fs_create("user1", "password1", session, seq++, "/user1Home", 'd');

    cout << "should be -1: " << fs_create("user1", "password1", session, seq++, "user1Home/noStartingSlash", 'f') << endl;
    cout << "should be -1: " << fs_create("user1", "password1", session, seq++, "/user1Home/endingSlash/", 'f') << endl;
    cout << "should be -1: " << fs_create("user1", "password1", session, seq++, "//user1Home/doubleStartingSlash", 'f') << endl;
    cout << "should be -1: " << fs_create("user1", "password1", session, seq++, "/user1Home/space inFilename", 'f') << endl;
    cout << "should be 0: " << fs_create("user1", "password1", session, seq++, "/user1Home/symbols!@#$%^&*()+-=?><,.~`{}[]|';:_", 'f') << endl;
    
    //This one should not work
    cout << "should be -1: " << fs_create("user1", "password1", session, seq++, "/user1Home/123456789012345678901234567890123456789012345678901234567890", 'f') << endl;
    //This one should work
    cout << "should be 0: " << fs_create("user1", "password1", session, seq++, "/user1Home/12345678901234567890123456789012345678901234567890123456789", 'f') << endl;
}