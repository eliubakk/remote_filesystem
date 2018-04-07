#include "fs_user.h"
#include <cassert>
#include <cstring>

unsigned int fs_user::SESSION_ID = 0;
std::mutex fs_user::session_lock;


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
	session_lock.lock();
	unsigned int session = SESSION_ID++;
	session_lock.unlock();
	user_lock.lock();
	sessions[session] = seq;
	user_lock.unlock();
	return session;
}


//Check if sequence # for this ID is valid, if so update the sequence # of this session
bool fs_user::session_request(unsigned int ID, unsigned int seq){
	user_lock.lock();
	if (sessions[ID] >= seq){
		user_lock.unlock();
		return false;
	}
	sessions[ID] = seq;
	user_lock.unlock();
	return true;
}