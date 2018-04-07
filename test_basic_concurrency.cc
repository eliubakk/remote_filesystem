#include <iostream>
#include <thread>
#include <cstdlib>
#include <cstring>
#include "fs_client.h"
#include "fs_param.h"

using namespace std;

void thread_func() {
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

    thread first(thread_func);
    first.detach();
    thread second(thread_func);
    second.detach();
    thread third(thread_func);
    third.detach();

    cout << "threads finished" << endl;
    return 0;
}