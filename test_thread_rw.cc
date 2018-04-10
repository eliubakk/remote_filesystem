#include <iostream>
#include <cstdlib>
#include <cstring>
#include <thread>
#include "fs_client.h"

using namespace std;

int main(int argc, char *argv[]){
    char *server;
    int server_port;
    unsigned int u1_session = 6, u2_session = 6, u3_session = 6, u4_session = 6, seq = 0, seq2 = 0, seq3 = 0, seq4 = 0;

    if (argc != 3) {
        cout << "error: usage: " << argv[0] << " <server> <serverPort>\n";
        exit(1);
    }
    server = argv[1];
    server_port = atoi(argv[2]);

    fs_clientinit(server, server_port);

    thread session1(fs_session, "user1", "password1", &u1_session, seq++);
    thread session2(fs_session, "user2", "password2", &u2_session, seq2++);
    thread session3(fs_session, "user3", "password3", &u2_session, seq3++);
    thread session4(fs_session, "user4", "password4", &u4_session, seq4++);
    session1.join();
    session2.join();
    session3.join();
    session4.join();

    thread session5(fs_create, "user1", "password1", u1_session, seq++, "/hw1", 'f');
    thread session6(fs_create, "user2", "password2", u2_session, seq2++, "/hw2", 'f');
    thread session7(fs_create, "user3", "password3", u3_session, seq3++, "/hw3", 'f');
    thread session8(fs_create, "user4", "password4", u4_session, seq4++, "/hw4", 'f');
    session5.join();
    session6.join();
    session7.join();
    session8.join();
    
    /*char message[FS_BLOCKSIZE]; 
    memset(message, 0, FS_BLOCKSIZE);
    strcpy(message, "Hello :)");
    strcpy(message + 9, "Bye :)");

    char output1[FS_BLOCKSIZE];
    memset(output1, 0, FS_BLOCKSIZE);   
    char output2[FS_BLOCKSIZE];
    memset(output2, 0, FS_BLOCKSIZE);   
    char output3[FS_BLOCKSIZE];
    memset(output3, 0, FS_BLOCKSIZE); 
    char output4[FS_BLOCKSIZE];
    memset(output4, 0, FS_BLOCKSIZE);

    thread session37(fs_writeblock, "user1", "password1", u1_session, seq++, "/hw1", 0, message);
    thread session38(fs_writeblock, "user1", "password1", u1_session, seq++, "/hw1", 0, message);
    thread session39(fs_writeblock, "user1", "password1", u1_session, seq++, "/hw1", 0, message);
    thread session40(fs_writeblock, "user1", "password1", u1_session, seq++, "/hw1", 0, message);
    thread session41(fs_writeblock, "user1", "password1", u1_session, seq++, "/hw1", 0, message);
    thread session42(fs_writeblock, "user1", "password1", u1_session, seq++, "/hw1", 0, message);
    thread session43(fs_writeblock, "user1", "password1", u1_session, seq++, "/hw1", 0, message);
    thread session44(fs_writeblock, "user1", "password1", u1_session, seq++, "/hw1", 0, message);
    thread session45(fs_writeblock, "user1", "password1", u1_session, seq++, "/hw1", 0, message);
    thread session46(fs_writeblock, "user1", "password1", u1_session, seq++, "/hw1", 0, message);
    thread session47(fs_writeblock, "user1", "password1", u1_session, seq++, "/hw1", 0, message);
    thread session48(fs_writeblock, "user1", "password1", u1_session, seq++, "/hw1", 0, message);

    session37.detach();
    session38.detach();
    session39.detach();
    session40.detach();
    session41.detach();
    session42.detach();
    session43.detach();
    session44.detach();
    session45.detach();
    session46.detach();
    session47.detach();
    session48.detach();

    thread session49(fs_readblock, "user1", "password1", u1_session, seq++, "/hw1", 0, output1);
    thread session50(fs_readblock, "user1", "password1", u2_session, seq++, "/hw1", 0, output1);
    thread session51(fs_readblock, "user1", "password1", u3_session, seq++, "/hw1", 0, output1);
    thread session52(fs_readblock, "user1", "password1", u4_session, seq++, "/hw1", 0, output1);
    thread session53(fs_readblock, "user1", "password1", u1_session, seq++, "/hw1", 0, output1);
    thread session54(fs_readblock, "user1", "password1", u2_session, seq++, "/hw1", 0, output1);
    thread session55(fs_readblock, "user1", "password1", u3_session, seq++, "/hw1", 0, output1);
    thread session56(fs_readblock, "user1", "password1", u4_session, seq++, "/hw1", 0, output1);
    thread session57(fs_readblock, "user1", "password1", u1_session, seq++, "/hw1", 0, output1);
    thread session58(fs_readblock, "user1", "password1", u2_session, seq++, "/hw1", 0, output1);
    thread session59(fs_readblock, "user1", "password1", u3_session, seq++, "/hw1", 0, output1);
    thread session60(fs_readblock, "user1", "password1", u4_session, seq++, "/hw1", 0, output1);

    session37.detach();
    session38.detach();
    session39.detach();
    session40.detach();
    session41.detach();
    session42.detach();
    session43.detach();
    session44.detach();
    session45.detach();
    session46.detach();
    session47.detach();
    session48.detach();

    session49.detach();
    session50.detach();
    session51.detach();
    session52.detach();
    session53.detach();
    session54.detach();
    session55.detach();
    session56.detach();
    session57.detach();
    session58.detach();
    session59.detach();
    session60.detach();

    for (int i = 0; i < 0xffff; ++i){
    }  */
}