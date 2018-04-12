#include <iostream>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <unistd.h>
#include "fs_client.h"

using namespace std;

int main(int argc, char *argv[]){
    char *server;
    int server_port;
    unsigned int u1_session = 6, seq = 0;

    if (argc != 3) {
        cout << "error: usage: " << argv[0] << " <server> <serverPort>\n";
        exit(1);
    }
    server = argv[1];
    server_port = atoi(argv[2]);

    fs_clientinit(server, server_port);

    thread session1(fs_session, "user1", "password1", &u1_session, seq++);
    session1.join();
    
    char message[FS_BLOCKSIZE]; 
    char message2[FS_BLOCKSIZE];
    memset(message, 0, FS_BLOCKSIZE);
    strcpy(message, "Hello :)");
    memset(message2, 0, FS_BLOCKSIZE);
    strcpy(message2, "I'm in the middle");

    /*thread session2(fs_create, "user1", "password1", u1_session, seq++, "/hw1", 'f');
    session2.join();
    thread session3(fs_writeblock, "user1", "password1", u1_session, seq++, "/hw1", 0, message);
    thread session4(fs_readblock, "user1", "password1", u1_session, seq++, "/hw1", 0, message2);

    session3.join();
    session4.join();*/
    thread session2(fs_create, "user1", "password1", u1_session, seq++, "/home", 'd');
    session2.join();
    thread session3(fs_create, "user1", "password1", u1_session, seq++, "/home/music", 'd');
    session3.join();
    thread session4(fs_delete, "user1", "password1", u1_session, seq++, "/home/music");
    usleep(1000);
    thread session5(fs_create, "user1", "password1", u1_session, seq++, "/home/project", 'f');
    session4.join();
    session5.join();
}