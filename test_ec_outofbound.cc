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

    fs_create("user1", "password1", session, seq++, "/home", 'd');
    fs_create("user1", "password1", session, seq++, "/home/hw", 'f');

    //WRITING AN EMPTY STRING TO A BLOCK
    const char *message = "";
    char read_message[FS_BLOCKSIZE];
    memset(read_message, 0, FS_BLOCKSIZE);
    fs_writeblock("user1", "password1", session, ++seq, "/home/hw", 0, message);
    fs_readblock("user1", "password1", session, ++seq, "/home/hw", 0, read_message);

    //READING AND WRITING OUT OF BOUNDS
    assert(fs_writeblock("user1", "password1", session, ++seq, "/home/hw", 125, message) == -1);
    assert(fs_writeblock("user1", "password1", session, ++seq, "/home/hw", 125, read_message) == -1);

    //WRITING AND READING A DIRECTORY
    const char *message2 = "Hi :)";
    char read_message2[FS_BLOCKSIZE];
    memset(read_message2, 0, FS_BLOCKSIZE);
    assert(fs_writeblock("user1", "password1", session, ++seq, "/home", 0, message2) == -1);
    assert(fs_readblock("user1", "password1", session, ++seq, "/home", 0, read_message2) == -1);

    //WRONG PASSWORD
    assert(fs_create("user1", "password2", session, seq++, "/home/hw2", 'f') == -1);
}