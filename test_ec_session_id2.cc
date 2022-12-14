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

    fs_create("user2", "password2", session, seq2++, "/Music", 'd');
    fs_create("user2", "password2", session, seq2++, "/Music/Snoop", 'f');

    //DIRECTORY INSIDE A FILE
    cout << "should be -1: " << fs_create("user2", "password2", session, seq2++, "/Music/Snoop/Hi", 'd') << endl;

    //FILE INSIDE A FILE
    cout << "should be 0: " << fs_create("user2", "password2", session, seq2++, "/Music/Hi.mp3", 'f') << endl;
    cout << "should be -1: " << fs_create("user2", "password2", session, seq2++, "/Music/Hi.mp3/Hello.mp3", 'f') << endl;

    //DON'T INCREMENT SEQ2 IN ERROR
    cout << "should be -1: " << fs_create("user2", "password2", session, seq2, "/Music/Snoop", 'd') << endl;
    //SHOULD FAIL BECAUSE OF WRONG SEQ2
    cout << "should be -1: " << fs_create("user2", "password2", session, seq2++, "/Music/Dre.mp3", 'f') << endl;
    seq2++;

    //SAME FILENAME
    cout << "should be 0: " << fs_create("user2", "password2", session, seq2++, "/Music/Dre.mp3", 'f') << endl;
    cout << "should be -1: " << fs_create("user2", "password2", session, seq2++, "/Music/Dre.mp3", 'f') << endl;

    //SAME DIRECTORY NAME
    cout << "should be 0: " << fs_create("user1", "password1", session, seq++, "/Video", 'd') << endl;
    cout << "should be -1: " << fs_create("user2", "password2", session, seq2++, "/Video", 'd') << endl;

    //SAME FILENAME IN ROOT
    cout << "should be 0: " << fs_create("user1", "password1", session, seq++, "/midterm.doc", 'f') << endl;
    cout << "should be -1: " << fs_create("user2", "password2", session, seq2++, "/midterm.doc", 'f') << endl;

    //INCREMENT SEQ2 ONCE MORE
    ++seq2;
    cout << "should be 0: " << fs_create("user2", "password2", session, seq2++, "/midterm_sol.doc", 'f') << endl;

    //SAME NAME DIRECTORY WITHIN
    cout << "should be 0: " << fs_create("user2", "password2", session, seq2++, "/Music/Music", 'd') << endl;

    //SAME NAME FILE
    cout << "should be 0: " << fs_create("user2", "password2", session, seq2++, "/Music/Music/Music", 'f') << endl;

    //CREATE MULTIPLE DIRECTORIES
    cout << "should be -1: " << fs_create("user2", "password2", session, seq2++, "/Music/Music/HipHop/Dre", 'd') << endl;    
}