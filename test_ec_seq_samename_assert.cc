#include <iostream>
#include <cstdlib>
#include <cassert>
#include "fs_client.h"

using namespace std;

int main(int argc, char *argv[]){
    char *server;
    int server_port;
    unsigned int session = 6, session2 = 7, seq = 0, seq2 = 0;

    if (argc != 3) {
        cout << "error: usage: " << argv[0] << " <server> <serverPort>\n";
        exit(1);
    }
    server = argv[1];
    server_port = atoi(argv[2]);

    fs_clientinit(server, server_port);

    fs_session("user1", "password1", &session, seq++);
    fs_session("user2", "password2", &session2, seq2++);

    fs_create("user2", "password2", session2, seq2++, "/Music", 'd');
    fs_create("user2", "password2", session2, seq2++, "/Music/Snoop", 'f');

    //DIRECTORY INSIDE A FILE
    assert(fs_create("user2", "password2", session2, seq2++, "/Music/Snoop/Hi", 'd') == -1);

    //FILE INSIDE A FILE
    assert(fs_create("user2", "password2", session2, seq2++, "/Music/Hi.mp3", 'f') == 0);
    assert(fs_create("user2", "password2", session2, seq2++, "/Music/Hi.mp3/Hello.mp3", 'f') == -1);

    //DON'T INCREMENT SEQ2 IN ERROR
    assert(fs_create("user2", "password2", session2, seq2, "/Music/Snoop", 'd') == -1);
    //SHOULD FAIL BECAUSE OF WRONG SEQ2
    assert(fs_create("user2", "password2", session2, seq2++, "/Music/Dre.mp3", 'f') == -1);
    seq2++;

    //SAME FILENAME
    assert(fs_create("user2", "password2", session2, seq2++, "/Music/Dre.mp3", 'f') == 0);
    assert(fs_create("user2", "password2", session2, seq2++, "/Music/Dre.mp3", 'f') == -1);

    //SAME DIRECTORY NAME
    assert(fs_create("user1", "password1", session, seq++, "/Video", 'd') == 0);
    assert(fs_create("user2", "password2", session2, seq2++, "/Video", 'd') == -1);

    //SAME FILENAME IN ROOT
    assert(fs_create("user1", "password1", session, seq++, "/midterm.doc", 'f') == 0);
    assert(fs_create("user2", "password2", session2, seq2++, "/midterm.doc", 'f') == -1);

    //INCREMENT SEQ2 ONCE MORE
    ++seq2;
    assert(fs_create("user2", "password2", session2, seq2++, "/midterm_sol.doc", 'f') == 0);

    //SAME NAME DIRECTORY WITHIN
    assert(fs_create("user2", "password2", session2, seq2++, "/Music/Music", 'd') == 0);

    //SAME NAME FILE
    assert(fs_create("user2", "password2", session2, seq2++, "/Music/Music/Music", 'f') == 0);

    //CREATE MULTIPLE DIRECTORIES
    assert(fs_create("user2", "password2", session2, seq2++, "/Music/Music/HipHop/Dre", 'd') == -1);    
}