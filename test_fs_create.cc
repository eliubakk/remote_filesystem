#include <iostream>
#include <cstdlib>
#include "fs_client.h"
#include "fs_server.h"

using namespace std;

int main(int argc, char *argv[]){
    char *server;
    int server_port;
    unsigned int session = 6, seq = 0;

    if (argc != 3) {
        cout_lock.lock();
        cout << "error: usage: " << argv[0] << " <server> <serverPort>\n";
        cout_lock.unlock();
        exit(1);
    }
    server = argv[1];
    server_port = atoi(argv[2]);

    fs_clientinit(server, server_port);
    cout_lock.lock();
    cout << "PRE_SESSION: " << session << endl;
    cout << "return val: " << fs_session("user1", "password1", &session, seq++) << endl;
    cout << "POST_SESSION: " << session << endl;
    cout_lock.unlock();
    fs_create("user1", "password1", session, seq++, "/home", 'd');
}