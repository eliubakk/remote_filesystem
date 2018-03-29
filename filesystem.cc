#include "filesystem.h"
#include <cassert>
#include <cstring>
#include <string>
#include <sstream>
#include <sys/socket.h>
#include <iostream>

using namespace std;

unordered_map<string, fs_user*> filesystem::users;

filesystem::entry::entry(uint32_t inode_in) 
	: inode_block(inode_in), entries(nullptr) {}

void filesystem::session_response(int client, const char *username, char *request,
                      unsigned int request_size){
  cout << "building session response..." << endl;
  //FS_SESSION <session> <sequence><NULL>
  char *request_name = strtok(request, " ");
  if(strcmp(request_name, "FS_SESSION"))
    return;
  cout << "request_name: " << request_name << endl;	

  strtok(nullptr, " ");
  char* request_sequence = strtok(nullptr, "\0");
  unsigned int sequence = stoi(request_sequence);

  unsigned int session = users[username]->create_session(sequence);

  //<size><NULL><session> <sequence><NULL>
  ostringstream response;
  response << to_string(session) << " " << to_string(sequence);

  string clear_response = response.str();
  cout << "clear_response: " << clear_response << endl;

  unsigned int cypher_size = 0;
  const char* cypher_response = (char*) fs_encrypt(
                users[username]->password(), (void*)clear_response.c_str(),
                clear_response.length() + 1, &cypher_size);

  cout << "cypher_response: " << cypher_response << endl;
  cout << "cypher_size: " << cypher_size << endl;
  response.str("");
  response.clear();
  response << to_string(cypher_size);// << "\0" << cypher_response;
  string header_response = response.str();
  cout << "full_response: " << header_response << cypher_response << endl;
  send(client, header_response.c_str(), header_response.length() + 1, 0);
  send(client, cypher_response, cypher_size, 0);

  cout << "session response sent" << endl;
  return;
}

void filesystem::readblock_response(const char *username, const char *password,
                   unsigned int session, unsigned int sequence,
                   const char *pathname, unsigned int offset, void *buf){
	assert(false);
}

void filesystem::writeblock_response(const char *username, const char *password,
                         unsigned int session, unsigned int sequence,
                         const char *pathname, unsigned int offset,
                         const void *buf){
	assert(false);
}

void filesystem::create_response(const char *username, const char *password,
                     unsigned int session, unsigned int sequence,
                     const char *pathname, char type){
	assert(false);
}

void filesystem::delete_response(const char *username, const char *password,
                     unsigned int session, unsigned int sequence,
                     const char *pathname){
	assert(false);
}