#include <iostream>
#include <cstdlib>
#include "fs_client.h"

using namespace std;

int main(int argc, char *argv[]){
    char *server;
    int server_port;
    unsigned int session = 6, seq = 0, seq2 = 0;

    if (argc != 3) {
        cout << "error: usage: " << argv[0] << " <server> <serverPort>\n";
        exit(1);
    }
    server = argv[1];
    server_port = atoi(argv[2]);

    fs_clientinit(server, server_port);

    fs_session("user1", "password1", &session, seq++);
    fs_session("user2", "password2", &session, seq2++);

    //WRONG OWNER ERROR CHECK
    cout << "Should be 0: " << fs_create("user1", "password1", session, seq++, "/home", 'd') << endl;
    cout << "Should be 0: " << fs_create("user1", "password1", session, seq++, "/hw_sol", 'f') << endl;
    cout << "Should be -1: " << fs_create("user2", "password2", session, seq2++, "/home/EECS482", 'd') << endl;
    cout << "Should be -1: " << fs_create("user2", "password2", session, seq2++, "/home/homework", 'f') << endl;

    cout << "Should be -1: " << fs_delete("user2", "password2", session, seq2++, "/home") << endl;
    cout << "Should be -1: " << fs_delete("user2", "password2", session, seq2++, "/hw_sol") << endl;

    char message[FS_BLOCKSIZE]; 
    memset(message, 0, FS_BLOCKSIZE);
    strcpy(message, "Hello :)");
    strcpy(message + 9, "Bye :)");

    char output[FS_BLOCKSIZE];
    memset(output, 0, FS_BLOCKSIZE);

    cout << "Should be -1: " << fs_writeblock("user2", "password2", session, seq2++, "/hw_sol", 0, message) << endl;
    cout << "Should be -1 " << fs_readblock("user2", "password2", session, seq2++, "/hw_sol", 0, output) << endl;
    cout << "seq2 should be 7: " << seq2 << endl;
}