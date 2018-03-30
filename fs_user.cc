#include "fs_user.h"
#include <cassert>
#include <cstring>

unsigned int fs_user::SESSION_ID = 0;


//Internal fs_user object constructor
fs_user::fs_user(const char* name_in, const char* pw_in) {
	strcpy(name, name_in);
	strcpy(pw, pw_in);
}


//Password getter
const char* fs_user::password(){
	return pw;
}


//Create new session for this user
unsigned int fs_user::create_session(unsigned int seq){
	sessions[SESSION_ID] = seq;
	return SESSION_ID++;
}


//Check if sequence # for this ID is valid, if so update the sequence # of this session
bool fs_user::session_request(unsigned int ID, unsigned int seq){
	if (sessions[ID] >= seq) 
		return false;
	sessions[ID] = seq;
	return true;
}