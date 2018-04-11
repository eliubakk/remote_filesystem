#include <iostream>
#include <cstdlib>
#include <cstring>
#include <thread>
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

    thread session2(fs_create, "user1", "password1", u1_session, seq++, "/hw1", 'f');
    session2.join();
    
    char message[FS_BLOCKSIZE]; 
    char message2[FS_BLOCKSIZE];
    memset(message, 0, FS_BLOCKSIZE);
    strcpy(message, "Hello :)");
    memset(message2, 0, FS_BLOCKSIZE);
    strcpy(message2, "Bye :)");

    thread session3(fs_writeblock, "user1", "password1", u1_session, seq++, "/hw1", 0, message);
    thread session4(fs_writeblock, "user1", "password1", u1_session, seq++, "/hw1", 1, message2);
    //thread session5(fs_writeblock, "user1", "password1", u1_session, seq++, "/hw1", 2, message);
    //thread session6(fs_writeblock, "user1", "password1", u1_session, seq++, "/hw1", 3, message2);

    session3.join();
    session4.join();
    //session5.join();
    //session6.join();     
}