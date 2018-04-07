#include <iostream>
#include <cstdlib>
#include <cstring>
#include <thread>
#include "fs_client.h"

using namespace std;

int main(int argc, char *argv[]){
    char *server;
    int server_port;
    unsigned int session = 6, seq = 0, seq2 = 0, seq3 = 0, seq4 = 0;

    if (argc != 3) {
        cout << "error: usage: " << argv[0] << " <server> <serverPort>\n";
        exit(1);
    }
    server = argv[1];
    server_port = atoi(argv[2]);

    fs_clientinit(server, server_port);

    thread session1(fs_session, "user1", "password1", &session, seq++);
    session1.detach();
    thread session2(fs_session, "user2", "password2", &session, seq2++);
    session2.detach();
    thread session3(fs_session, "user3", "password3", &session, seq3++);
    session3.detach();
    thread session4(fs_session, "user4", "password4", &session, seq4++);
    session4.detach();

    thread session5(fs_create, "user1", "password1", session, seq++, "/home", 'd');
    session5.detach();
    thread session6(fs_create, "user2", "password2", session, seq2++, "/home2", 'd');
    session6.detach();
    thread session7(fs_create, "user3", "password3", session, seq3++, "/home3", 'd');
    session7.detach();
    thread session8(fs_create, "user4", "password4", session, seq4++, "/home4", 'd');
    session8.detach();

    thread session9(fs_create, "user1", "password1", session, seq++, "/home/EECS482", 'd');
    session9.detach();
    thread session10(fs_create, "user2", "password2", session, seq2++, "/home2/EECS482", 'd');
    session10.detach();
    thread session11(fs_create, "user3", "password3", session, seq3++, "/home3/EECS482", 'd');
    session11.detach();
    thread session12(fs_create, "user4", "password4", session, seq4++, "/home4/EECS482", 'd');
    session12.detach();

    thread session13(fs_create, "user1", "password1", session, seq++, "/home/EECS482/project4.cc", 'f');
    session13.detach();
    thread session14(fs_create, "user2", "password2", session, seq2++, "/home2/EECS482/project4.cc", 'f');
    session14.detach();
    thread session15(fs_create, "user3", "password3", session, seq3++, "/home3/EECS482/project4.cc", 'f');
    session15.detach();
    thread session16(fs_create, "user4", "password4", session, seq4++, "/home4/EECS482/project4.cc", 'f');
    session16.detach();

    thread session17(fs_create, "user1", "password1", session, seq++, "/home/EECS482/garbage", 'f');
    session17.detach();
    thread session18(fs_create, "user2", "password2", session, seq2++, "/home2/EECS482/garbage", 'f');
    session18.detach();
    thread session19(fs_create, "user3", "password3", session, seq3++, "/home3/EECS482/garbage", 'f');
    session19.detach();
    thread session20(fs_create, "user4", "password4", session, seq4++, "/home4/EECS482/garbage", 'f');
    session20.detach();

    char message[FS_BLOCKSIZE]; 
    memset(message, 0, FS_BLOCKSIZE);
    strcpy(message, "Hello :)");
    strcpy(message + 9, "Bye :)");

    char output[FS_BLOCKSIZE];
    memset(output, 0, FS_BLOCKSIZE);   

    thread session21(fs_writeblock, "user1", "password1", session, seq++, "/home/EECS482/project4.cc", 0, message);
    session21.detach();
    thread session22(fs_writeblock, "user2", "password2", session, seq2++, "/home2/EECS482/project4.cc", 0, message);
    session22.detach();
    thread session23(fs_writeblock, "user3", "password3", session, seq3++, "/home3/EECS482/project4.cc", 0, message);
    session23.detach();
    thread session24(fs_writeblock, "user4", "password4", session, seq4++, "/home4/EECS482/project4.cc", 0, message);
    session24.detach();    

    thread session25(fs_writeblock, "user1", "password1", session, seq++, "/home/EECS482/garbage", 0, message);
    session25.detach();
    thread session26(fs_writeblock, "user2", "password2", session, seq2++, "/home2/EECS482/garbage", 0, message);
    session26.detach();
    thread session27(fs_writeblock, "user3", "password3", session, seq3++, "/home3/EECS482/garbage", 0, message);
    session27.detach();
    thread session28(fs_writeblock, "user4", "password4", session, seq4++, "/home4/EECS482/garbage", 0, message);
    session28.detach();

    thread session29(fs_delete, "user1", "password1", session, seq++, "/home/EECS482/garbage");
    session29.detach();
    thread session30(fs_delete, "user2", "password2", session, seq2++, "/home2/EECS482/garbage");
    session30.detach();
    thread session31(fs_delete, "user3", "password3", session, seq3++, "/home3/EECS482/garbage");
    session31.detach();
    thread session32(fs_delete,"user4", "password4", session, seq4++, "/home4/EECS482/garbage");
    session32.detach();

    thread session33(fs_readblock, "user1", "password1", session, seq++, "/home/EECS482/project4.cc", 0, output);
    session33.detach();
    thread session34(fs_readblock, "user2", "password2", session, seq2++, "/home2/EECS482/project4.cc", 0, output);
    session34.detach();
    thread session35(fs_readblock, "user3", "password3", session, seq3++, "/home3/EECS482/project4.cc", 0, output);
    session35.detach();
    thread session36(fs_readblock, "user4", "password4", session, seq4++, "/home4/EECS482/project4.cc", 0, output);
    session36.detach();
}