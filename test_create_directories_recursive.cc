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
    cout << "PRE_SESSION: " << session << endl;
    cout << "return val: " << fs_session("user1", "password1", &session, seq++) << endl;
    cout << "POST_SESSION: " << session << endl;
    fs_create("user1", "password1", session, seq++, "/home", 'd');
    fs_create("user1", "password1", session, seq++, "/home/Documents", 'd');
    fs_create("user1", "password1", session, seq++, "/bin", 'd');
    fs_create("user1", "password1", session, seq++, "/home/Music", 'd');
    fs_create("user1", "password1", session, seq++, "/home/Videos", 'd');
    fs_create("user1", "password1", session, seq++, "/home/Pictures", 'd');
    fs_create("user1", "password1", session, seq++, "/home/Documents/eecs", 'd');
    fs_create("user1", "password1", session, seq++, "/home/Documents/eecs/482", 'd');
    fs_create("user1", "password1", session, seq++, "/home/Documents/eecs/461", 'd');
    fs_create("user1", "password1", session, seq++, "/home/Documents/eecs/482/p1", 'd');
    fs_create("user1", "password1", session, seq++, "/home/Documents/eecs/482/p2", 'd');
    fs_create("user1", "password1", session, seq++, "/home/Documents/eecs/482/p3", 'd');
    fs_create("user1", "password1", session, seq++, "/home/Documents/eecs/482/p4", 'd');
}