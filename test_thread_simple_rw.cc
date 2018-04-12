#include <iostream>
#include <cstdlib>
#include <cstring>
#include <thread>
#include "fs_client.h"

using namespace std;

int main(int argc, char *argv[]){
    char *server;
    int server_port;
    unsigned int u1_session = 6, u2_session = 7, u3_session = 8, u4_session = 9, seq = 0;

    if (argc != 3) {
        cout << "error: usage: " << argv[0] << " <server> <serverPort>\n";
        exit(1);
    }
    server = argv[1];
    server_port = atoi(argv[2]);

    fs_clientinit(server, server_port);

    thread session1(fs_session, "user1", "password1", &u1_session, seq++);
    session1.join();
    thread two(fs_session, "user1", "password1", &u2_session, seq++);
    two.join();
    thread three(fs_session, "user1", "password1", &u3_session, seq++);
    three.join();
    thread four(fs_session, "user1", "password1", &u4_session, seq++);
    four.join();

    thread session2(fs_create, "user1", "password1", u1_session, seq++, "/hw1", 'f');
    session2.join();
    
    char message[FS_BLOCKSIZE]; 
    char message2[FS_BLOCKSIZE];
    char message3[FS_BLOCKSIZE]; 
    char message4[FS_BLOCKSIZE];
    memset(message, 0, FS_BLOCKSIZE);
    strcpy(message, "Hello :)");
    memset(message2, 0, FS_BLOCKSIZE);
    strcpy(message2, "I'm in the middle");
    memset(message3, 0, FS_BLOCKSIZE);
    strcpy(message3, "I'm after the other guy");
    memset(message4, 0, FS_BLOCKSIZE);
    strcpy(message4, "Bye :)");

    thread session3(fs_writeblock, "user1", "password1", u1_session, seq++, "/hw1", 0, message);
    thread session4(fs_writeblock, "user1", "password1", u2_session, seq++, "/hw1", 1, message2);
    thread session5(fs_writeblock, "user1", "password1", u3_session, seq++, "/hw1", 2, message3);
    thread session6(fs_writeblock, "user1", "password1", u4_session, seq++, "/hw1", 3, message4);

    session3.join();
    session4.join();
    session5.join();
    session6.join();     
}