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
#include <queue>
#include "fs_user.h"

#define READ 0
#define WRITE 1

#define FILE 0
#define DIR 1

// ----- For explicitly indexing into vector with request args ----- //
#define REQUEST_NAME 0
#define SESSION 1
#define SEQUENCE 2
#define PATH 3
#define BLOCK 4
#define INODE_TYPE 4
#define ACCESS_TYPE 5
#define DATA 6

class filesystem{
	private:
		std::unordered_map<std::string, fs_user*> users;
		std::queue<unsigned int> free_blocks;
		std::vector<pthread_rwlock_t> block_lock;
		void send_response(int client, const char *username, std::string response);
		std::vector<char *> split_request(char *request, const std::string &token);
		int new_session(const char* username, std::vector<char*>& args);
		int create_entry(const char* username, std::vector<char*>& args);
		int delete_entry(const char* username, std::vector<char*>& args);
		int access_entry(const char* username, std::vector<char*>& args);
		fs_direntry* recurse_filesystem(const char *username, char* path, fs_inode*& inode, char req_type);
		fs_direntry* recurse_filesystem_helper(const char *username, std::vector<char*> &split_path, unsigned int path_index, fs_direntry dir, fs_inode*& inode, char req_type);

	public:
		filesystem();
		~filesystem();

		bool add_user(const std::string& username, const std::string& password);

		bool user_exists(const std::string& username);

		const char* password(const std::string& username);

		void handle_request(int client, const char *username, char *request,
                      unsigned int request_size);
};

#endif