#include <iostream>
#include <cstdlib>
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
    fs_create("user1", "password1", session, seq++, "/home/EECS482", 'd');
    fs_create("user1", "password1", session, seq++, "/home/EECS482/project4", 'd');
    fs_create("user1", "password1", session, seq++, "/home/EECS482/test_case", 'f');
    fs_create("user1", "password1", session, seq++, "/home/EECS482/test_case2", 'f');

    cout << "Should be 0: " << fs_delete("user1", "password1", session, seq++, "/home/EECS482/test_case") << endl;
    cout << "Should be 0: " << fs_delete("user1", "password1", session, seq++, "/home/EECS482") << endl;
    cout << "Should be -1: " << fs_delete("user1", "password1", session, seq++, "/home/EECS482/project4/test_case2") << endl;
    cout << "Should be -1: " << fs_delete("user1", "password1", session, seq++, "/home/EECS482/project4") << endl;     
}