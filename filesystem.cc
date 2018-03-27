#include "filesystem.h"
#include <cassert>

std::unordered_map<std::string, fs_user*> filesystem::users;

filesystem::entry::entry(uint32_t inode_in) 
	: inode_block(inode_in), entries(nullptr) {}

void filesystem::session_response(const char *username, const char *password,
                      unsigned int *session_ptr, unsigned int sequence){
	assert(false);
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