/*
 * fs_user.h
 *
 * Header file for users in the filesystem server
 */

#ifndef _FS_USER_H_
#define _FS_USER_H_

#include <sys/types.h>
#include <string>
#include <unordered_map>
#include "fs_server.h"
#include "fs_param.h"

class fs_user{
	private:
		char name[FS_MAXUSERNAME + 1];
		char pw[FS_MAXPASSWORD + 1];
		std::unordered_map<unsigned int, unsigned int> sessions;

		static uint64_t SESSION_ID ;

	public:
		fs_user(const char* name_in, const char* pw_in);

		unsigned int create_session(unsigned int seq);

		bool session_request(unsigned int ID, unsigned int seq);
};

#endif