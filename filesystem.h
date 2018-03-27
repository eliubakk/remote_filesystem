/*
 * filesystem.h
 *
 * Header file filesystem 
 */

#ifndef _FILESYSTEM_H_
#define _FILESYSTEM_H_

#include <sys/types.h>
#include <mutex>
#include <string>
#include <unordered_map>
#include "fs_user.h"

class filesystem{
	public:
		struct entry{
			std::mutex lock;
			uint32_t inode_block;
			std::unordered_map<std::string, entry*> *entries; 

			entry(uint32_t inode_in = 0);
		};

		static entry root;

		static std::unordered_map<std::string, fs_user*> users;

		static void session_response(const char *username, const char *password,
                      unsigned int *session_ptr, unsigned int sequence);

		static void readblock_response(const char *username, const char *password,
                   unsigned int session, unsigned int sequence,
                   const char *pathname, unsigned int offset, void *buf);

		static void writeblock_response(const char *username, const char *password,
                         unsigned int session, unsigned int sequence,
                         const char *pathname, unsigned int offset,
                         const void *buf);

		static void create_response(const char *username, const char *password,
                     unsigned int session, unsigned int sequence,
                     const char *pathname, char type);

		static void delete_response(const char *username, const char *password,
                     unsigned int session, unsigned int sequence,
                     const char *pathname);
};

#endif