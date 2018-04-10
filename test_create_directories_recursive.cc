#include <iostream>
#include <cstdlib>
#include <cstring>
#include "fs_client.h"

using namespace std;

int main(int argc, char *argv[]){
    char *server;
    int server_port;
    unsigned int user1_sessions[2], user1_seq[2] = {0, 20};
    unsigned int user2_sessions[2], user2_seq[2] = {0, 5};

    if (argc != 3) {
        cout << "error: usage: " << argv[0] << " <server> <serverPort>\n";
        exit(1);
    }
    server = argv[1];
    server_port = atoi(argv[2]);

    fs_clientinit(server, server_port);

    //create sessions
    fs_session("user1", "password1", &user1_sessions[0], user1_seq[0]++);
    fs_session("user1", "password1", &user1_sessions[1], user1_seq[1]++);
    fs_session("user2", "password2", &user2_sessions[0], user2_seq[0]++);
    fs_session("user2", "password2", &user2_sessions[1], user2_seq[1]++);

    //create fs
    fs_create("user1", "password1", user1_sessions[0], user1_seq[0]++, "/home", 'd');
    fs_create("user1", "password1", user1_sessions[1], user1_seq[1]++, "/home/Documents", 'd');
    fs_create("user2", "password2", user2_sessions[0], user2_seq[0]++, "/user2_home", 'd');
    fs_create("user2", "password2", user2_sessions[0], user2_seq[0]++, "/stuff.txt", 'f');
    fs_create("user1", "password1", user1_sessions[0], user1_seq[0]++, "/bin", 'd');
    fs_create("user1", "password1", user1_sessions[0], user1_seq[0]++, "/home/secret_stuff.txt", 'f');
    fs_create("user1", "password1", user1_sessions[1], user1_seq[1]++, "/home/Videos", 'd');
    fs_create("user1", "password1", user1_sessions[0], user1_seq[0]++, "/home/Pictures", 'd');
    fs_create("user1", "password1", user1_sessions[1], user1_seq[1]++, "/home/Documents/eecs", 'd');
    fs_create("user1", "password1", user1_sessions[0], user1_seq[0]++, "/home/Documents/eecs/482", 'd');
    fs_create("user1", "password1", user1_sessions[0], user1_seq[0]++, "/home/Documents/eecs/461_prelab1.txt", 'f');
    fs_create("user1", "password1", user1_sessions[1], user1_seq[1]++, "/home/Documents/eecs/482/p1", 'd');
    fs_create("user1", "password1", user1_sessions[1], user1_seq[1]++, "/home/Documents/eecs/482/p2", 'd');
    fs_create("user1", "password1", user1_sessions[0], user1_seq[0]++, "/home/Documents/eecs/482/p3", 'f');
    fs_create("user1", "password1", user1_sessions[0], user1_seq[0]++, "/home/Documents/eecs/482/p4", 'd');

    fs_create("user2", "password2", user2_sessions[1], user2_seq[1]++, "/user2_home/dir1", 'd');
    fs_create("user2", "password2", user2_sessions[1], user2_seq[1]++, "/user2_home/dir2", 'd');
    fs_create("user2", "password2", user2_sessions[1], user2_seq[1]++, "/user2_home/dir3", 'd');
    fs_create("user2", "password2", user2_sessions[1], user2_seq[1]++, "/user2_home/dir4", 'd');
    fs_create("user2", "password2", user2_sessions[1], user2_seq[1]++, "/user2_home/dir5", 'd');
    fs_create("user2", "password2", user2_sessions[1], user2_seq[1]++, "/user2_home/dir6", 'd');
    fs_create("user2", "password2", user2_sessions[1], user2_seq[1]++, "/user2_home/dir7", 'd');
    fs_create("user2", "password2", user2_sessions[1], user2_seq[1]++, "/user2_home/dir8", 'd');
    fs_create("user2", "password2", user2_sessions[1], user2_seq[1]++, "/user2_home/dir9", 'd');
    fs_create("user2", "password2", user2_sessions[1], user2_seq[1]++, "/user2_home/dir10", 'd');

    fs_delete("user2", "password2", user2_sessions[0], user2_seq[0]++, "/user2_home/dir2");
    fs_delete("user2", "password2", user2_sessions[0], user2_seq[0]++, "/user2_home/dir5");
    fs_create("user2", "password2", user2_sessions[1], user2_seq[1]++, "/user2_home/dir4/dir4.txt", 'f');

    char writedata[FS_BLOCKSIZE]; 
    memset(writedata, 0, FS_BLOCKSIZE);
    memset(writedata, 'a', 25);
    fs_writeblock("user1", "password1", user1_sessions[0], user1_seq[0]++, "/home/secret_stuff.txt", 0, writedata);
    memset(writedata, 'b', 25);
    fs_writeblock("user1", "password1", user1_sessions[0], user1_seq[0]++, "/home/secret_stuff.txt", 1, writedata);
    memset(writedata, 'c', 25);
    fs_writeblock("user1", "password1", user1_sessions[0], user1_seq[0]++, "/home/secret_stuff.txt", 2, writedata);

    memset(writedata, 0, FS_BLOCKSIZE);
    strcpy(writedata, "This text file is in user2_home/dir4");
    fs_writeblock("user2", "password2", user2_sessions[0], user2_seq[0]++, "/user2_home/dir4/dir4.txt", 0, writedata);
    memset(writedata, 0, FS_BLOCKSIZE);
    strcpy(writedata, "This text is in the second block of this file...");
    fs_writeblock("user2", "password2", user2_sessions[0], user2_seq[0]++, "/user2_home/dir4/dir4.txt", 1, writedata);
}