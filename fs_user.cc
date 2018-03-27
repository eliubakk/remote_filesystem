#include "fs_user.h"
#include <cassert>
#include <cstring>

uint64_t fs_user::SESSION_ID = 0;

fs_user::fs_user(const char* name_in, const char* pw_in) {
	strcpy(name, name_in);
	strcpy(pw, pw_in);
}

unsigned int fs_user::create_session(unsigned int seq){
	assert(false);
}

bool fs_user::session_request(unsigned int ID, unsigned int seq){
	assert(false);
}