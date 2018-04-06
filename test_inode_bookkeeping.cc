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

    char message[FS_BLOCKSIZE]; 
    memset(message, 0, FS_BLOCKSIZE);
    strcpy(message, "Season of mists and mellow fruitfulness...");
    vector<string> paths = {"/user1Home/message1",
                            "/user1Home/message2", 
                            "/user1Home/message3", 
                            "/user1Home/thisIsGettingDeleted", 
                            "/user1Home/message5", 
                            "/user1Home/message6", 
                            "/user1Home/message7", 
                            "/user1Home/message8",
                            "/user1Home/message9",
                            "/user1Home/message10",
                            "/user1Home/message11",
                            "/user1Home/message12" 
                            };

    fs_create("user1", "password1", session, seq++, "/user1Home", 'd');

    for (int i = 0; i < 8; ++i) {
        fs_create("user1", "password1", session, seq++, paths[i].c_str(), 'f');
        fs_writeblock("user1", "password1", session, seq++, paths[i].c_str(), 0, message);
    }

    fs_delete("user1", "password1", session, seq++, "/user1Home/thisIsGettingDeleted");
    fs_create("user1", "password1", session, seq++, "/user1Home/message4", 'f');



    for (int i = 8; i < 12; ++i) {
        fs_create("user1", "password1", session, seq++, paths[i].c_str(), 'f');
        fs_writeblock("user1", "password1", session, seq++, paths[i].c_str(), 0, message);
    }

    fs_delete("user1", "password1", session, seq++, "/user1Home/message5");

    fs_delete("user1", "password1", session, seq++, "/user1Home/message10");
    fs_delete("user1", "password1", session, seq++, "/user1Home/message12");
    fs_delete("user1", "password1", session, seq++, "/user1Home/message9");
    fs_delete("user1", "password1", session, seq++, "/user1Home/message11");
}