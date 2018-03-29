/*
 * filesystem.h
 *
 * Header file filesystem 
 */

#ifndef _FILESYSTEM_H_
#define _FILESYSTEM_H_

#include <sys/types.h>
#include <pthread.h>
#include <string>
#include <unordered_map>
#include "fs_user.h"

class filesystem{
	private:
		struct entry{
			pthread_rwlock_t lock;
			uint32_t inode_block;
			std::unordered_map<std::string, entry*> *entries; 

			entry(uint32_t inode_in = 0);
		};

		entry root;
		std::unordered_map<std::string, fs_user*> users;

		void send_response(int client, const char *username, std::string response);
	public:
		~filesystem();

		bool add_user(const std::string& username, const std::string& password);

		bool user_exists(const std::string& username);

		const char* password(const std::string& username);
		
		void session_response(int client, const char *username, char *request,
                      unsigned int request_size);

		void readblock_response(int client, const char *username, char *request,
                      unsigned int request_size);

		void writeblock_response(int client, const char *username, char *request,
                      unsigned int request_size);

		void create_response(int client, const char *username, char *request,
                      unsigned int request_size);

		void delete_response(int client, const char *username, char *request,
                      unsigned int request_size);
};

#endif