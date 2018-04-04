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
#include "fs_server.h"

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
		/*struct recurse_args{
			fs_inode* inode,
			unsigned int disk_block,
			unsigned int blocks_index,
			fs_direntry folders[FS_DIRENTRIES],
			unsigned int folder_index
		};*/

		std::unordered_map<std::string, fs_user*> users;
		std::queue<unsigned int> free_blocks;
		pthread_rwlock_t block_lock[FS_DISKSIZE];
		void send_response(int client, const char *username, std::vector<std::string>& request_args);
		std::vector<std::string> split_request(const std::string &request, const std::string& token);
		int new_session(const char* username, std::vector<std::string>& args);
		int delete_entry(const char* username, std::vector<std::string>& args);
		int create_entry(const char* username, std::vector<std::string>& args);
		int access_entry(const char* username, std::vector<std::string>& args);
		bool search_directory(fs_inode*& dir_inode, unsigned int &dir_block, fs_direntry* folders,  unsigned int &folder_index, const char* name);
		bool recurse_filesystem(const char *username, std::string& path, fs_inode*& inode, unsigned int &disk_block,
							unsigned int &block, fs_direntry* folders, unsigned int &folder_index, char req_type);
		bool recurse_filesystem_helper(const char* username, std::vector<std::string> &split_path, 
						  			unsigned int path_index, fs_inode*& inode, unsigned int &disk_block,
						  			unsigned int &block, 
									fs_direntry* folders, unsigned int &folder_index, char req_type);

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