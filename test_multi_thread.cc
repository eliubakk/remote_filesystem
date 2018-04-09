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
    thread session3(fs_session, "user3", "password3", &u3_session, seq3++);
    thread session4(fs_session, "user4", "password4", &u4_session, seq4++);
    session1.join();
    session2.join();
    session3.join();
    session4.join();

    thread session5(fs_create, "user1", "password1", u1_session, seq++, "/home", 'd');
    thread session6(fs_create, "user2", "password2", u2_session, seq2++, "/home2", 'd');
    thread session7(fs_create, "user3", "password3", u3_session, seq3++, "/home3", 'd');
    thread session8(fs_create, "user4", "password4", u4_session, seq4++, "/home4", 'd');
    session5.join();
    session6.join();
    session7.join();
    session8.join();
    
    thread session9(fs_create, "user1", "password1", u1_session, seq++, "/home/EECS482", 'd');
    thread session10(fs_create, "user2", "password2", u2_session, seq2++, "/home2/EECS482", 'd');
    thread session11(fs_create, "user3", "password3", u3_session, seq3++, "/home3/EECS482", 'd');
    thread session12(fs_create, "user4", "password4", u4_session, seq4++, "/home4/EECS482", 'd');
    session9.join();
    session10.join();
    session11.join();
    session12.join();
    
    thread session13(fs_create, "user1", "password1", u1_session, seq++, "/home/EECS482/project4.cc", 'f');
    thread session14(fs_create, "user2", "password2", u2_session, seq2++, "/home2/EECS482/project4.cc", 'f');
    thread session15(fs_create, "user3", "password3", u3_session, seq3++, "/home3/EECS482/project4.cc", 'f');
    thread session16(fs_create, "user4", "password4", u4_session, seq4++, "/home4/EECS482/project4.cc", 'f');
    session13.join();
    session14.join();
    session15.join();
    session16.join();
    
    thread session17(fs_create, "user1", "password1", u1_session, seq++, "/home/EECS482/garbage", 'f');
    thread session18(fs_create, "user2", "password2", u2_session, seq2++, "/home2/EECS482/garbage", 'f');
    thread session19(fs_create, "user3", "password3", u3_session, seq3++, "/home3/EECS482/garbage", 'f');
    thread session20(fs_create, "user4", "password4", u4_session, seq4++, "/home4/EECS482/garbage", 'f');
    session17.join();
    session18.join();
    session19.join();
    session20.join();
    
    char message[FS_BLOCKSIZE]; 
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

    thread session21(fs_writeblock, "user1", "password1", u1_session, seq++, "/home/EECS482/project4.cc", 0, message);
    thread session22(fs_writeblock, "user2", "password2", u2_session, seq2++, "/home2/EECS482/project4.cc", 0, message);
    thread session23(fs_writeblock, "user3", "password3", u3_session, seq3++, "/home3/EECS482/project4.cc", 0, message);
    thread session24(fs_writeblock, "user4", "password4", u4_session, seq4++, "/home4/EECS482/project4.cc", 0, message);
    session21.join();
    session22.join();
    session23.join();
    session24.join();

    thread session25(fs_writeblock, "user1", "password1", u1_session, seq++, "/home/EECS482/garbage", 0, message);
    thread session26(fs_writeblock, "user2", "password2", u2_session, seq2++, "/home2/EECS482/garbage", 0, message);
    thread session27(fs_writeblock, "user3", "password3", u3_session, seq3++, "/home3/EECS482/garbage", 0, message);
    thread session28(fs_writeblock, "user4", "password4", u4_session, seq4++, "/home4/EECS482/garbage", 0, message);
    session25.join();
    session26.join();
    session27.join();
    session28.join();
    
    thread session29(fs_delete, "user1", "password1", u1_session, seq++, "/home/EECS482/garbage");
    thread session30(fs_delete, "user2", "password2", u2_session, seq2++, "/home2/EECS482/garbage");
    thread session31(fs_delete, "user3", "password3", u3_session, seq3++, "/home3/EECS482/garbage");
    thread session32(fs_delete,"user4", "password4", u4_session, seq4++, "/home4/EECS482/garbage");
    session29.join();
    session30.join();
    session31.join();
    session32.join();

    thread session33(fs_readblock, "user1", "password1", u1_session, seq++, "/home/EECS482/project4.cc", 0, output1);
    thread session34(fs_readblock, "user2", "password2", u2_session, seq2++, "/home2/EECS482/project4.cc", 0, output2);
    thread session35(fs_readblock, "user3", "password3", u3_session, seq3++, "/home3/EECS482/project4.cc", 0, output3);
    thread session36(fs_readblock, "user4", "password4", u4_session, seq4++, "/home4/EECS482/project4.cc", 0, output4);
    session33.join();
    session34.join();
    session35.join();
    session36.join();
}