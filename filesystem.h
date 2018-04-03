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
#include <vector>
#include <bitset>
#include "fs_user.h"

class filesystem{
	private:
		struct entry{
			pthread_rwlock_t lock;
			unsigned int inode_block;
			unsigned int parent_blocks_index;
			std::unordered_map<std::string, entry*> entries; 

			entry(unsigned int inode_block_in = 0, unsigned int parent_blocks_index_in = 0);
		};

		entry root;
		std::unordered_map<std::string, fs_user*> users;
		std::bitset<FS_DISKSIZE> disk_blocks;

		void send_response(int client, const char *username, std::string response);
		std::vector<char *> split_request(char *request, const std::string &token);
		int create_entry(const char* username, char *path, char* type);
		entry* recurse_filesystem(const char *username, char* path, fs_inode*& inode, char req_type);
		entry* recurse_filesystem_helper(const char *username, std::vector<char*> &split_path, unsigned int path_index, entry* dir, fs_inode*& inode, char req_type);
		unsigned int next_free_disk_block();

	public:
		filesystem();
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