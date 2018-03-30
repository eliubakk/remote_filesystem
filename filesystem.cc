#include "filesystem.h"
#include <cassert>
#include <cstring>
#include <string>
#include <sstream>
#include <sys/socket.h>
#include <iostream>

using namespace std;

#define REQUEST_NAME 0
#define SESSION 1
#define SEQUENCE 2
#define PATH 3
#define BLOCK 4
#define TYPE 4

filesystem::entry::entry(unsigned int inode_block_in, unsigned int parent_blocks_index_in) 
: inode_block(inode_block_in), parent_blocks_index(parent_blocks_index_in) {}

filesystem::filesystem() {
	//Init root directory
	disk_blocks[0] = 1;
}

filesystem::~filesystem(){
	for(auto user : users)
		delete user.second;
}

bool filesystem::add_user(const string& username, const string& password){
	if(users.find(username) != users.end())
		return false;
	users[username] = new fs_user(username.c_str(), password.c_str());
	return true;
}

bool filesystem::user_exists(const string& username){
	return (users.find(username) != users.end());
}

const char* filesystem::password(const string& username){
	return users[username]->password();
}



void filesystem::session_response(int client, const char *username, char *request,
							unsigned int request_size){
	cout << "building session response..." << endl;
	//FS_SESSION <session> <sequence><NULL>
	vector<char *> request_items = split_request(request, " ");
	if(strcmp(request_items[REQUEST_NAME], "FS_SESSION"))
		return;
	cout << "request_name: " << request_items[REQUEST_NAME] << endl;	

	unsigned int sequence = stoi(request_items[SEQUENCE]);
	unsigned int session = users[username]->create_session(sequence);

	//<size><NULL><session> <sequence><NULL>
	ostringstream response;
	response << to_string(session) << " " << to_string(sequence);
	send_response(client, username, response.str());
	return;
}

void filesystem::readblock_response(int client, const char *username, char *request,
							unsigned int request_size){
	assert(false);
}

void filesystem::writeblock_response(int client, const char *username, char *request,
							unsigned int request_size){
	assert(false);
}


void filesystem::create_response(int client, const char *username, char *request,
							unsigned int request_size){
	vector<char *> request_items = split_request(request, " ");
	if(strcmp(request_items[REQUEST_NAME], "FS_CREATE"))
		return;
	cout << "request_name: " << request_items[REQUEST_NAME] << endl;

	//FS_CREATE <session> <sequence> <pathname> <type><NULL>
	if (!users[username]->session_request(stoi(request_items[SESSION]), stoi(request_items[SEQUENCE])))
		return;
	cout << "sequence valid" << endl;
	if (create_entry(username, request_items[PATH], request_items[TYPE]) == -1)
		return;

	cout << "entry created, sending response..." << endl;
	//<size><NULL><session> <sequence><NULL>
	ostringstream response;
	response << request_items[SESSION] << " " << request_items[SEQUENCE];
	send_response(client, username, response.str());
}

void filesystem::delete_response(int client, const char *username, char *request,
							unsigned int request_size){
	assert(false);
}

void filesystem::send_response(int client, const char *username, string response){
	cout << "response built" << endl;

	unsigned int cipher_size = 0;
	const char* cipher = (char*) fs_encrypt(
		users[username]->password(), (void*)response.c_str(),
		response.length() + 1, &cipher_size);

	cout << "response encrypted" << endl;
	cout << "cipher_size: " << cipher_size << endl;
	string header(to_string(cipher_size));
	send(client, header.c_str(), header.length() + 1, 0);
	send(client, cipher, cipher_size, 0);

	cout << "session response sent" << endl;
}

vector<char *> filesystem::split_request(char *request, const string &token) {
	vector<char *> split;
	split.push_back(strtok(request, token.c_str()));
	while (split.back()) {
		cout << split.back() << endl;
		split.push_back(strtok(0, token.c_str()));
	}
	split.pop_back();
	cout << "size: " << split.size() << endl;
	return split;
}


int filesystem::create_entry(const char* username, char *path, char* type) {
	cout << "checking if path is valid..." << endl;

	vector<char *> split_path = split_request(path, "/");
	cout << "path split" << endl;


	unsigned int new_entry_inode_block = next_free_disk_block();
	if (new_entry_inode_block == FS_DISKSIZE)
		return -1;
	cout << "have enough space for new entry" << endl;
	disk_blocks[new_entry_inode_block] = 1;

	entry* parent_entry = &root;
	fs_inode* inode = new fs_inode;
	disk_readblock(0, inode);
	cout << split_path[0] << endl;
	if (root.entries.find(split_path[0]) != root.entries.end()) {
		cout << "recurse_filesystem to find parent_entry" << endl;
		parent_entry = recurse_filesystem(username, split_path, 0, root.entries[split_path[1]], inode, 'c');
	}else{
		cout << "path is in root" << endl;
	}
	
	if(parent_entry == nullptr){
		cout << "path invald" << endl;
		delete inode;
		return -1;
	}

	cout << "parent_entry found, path valid" << endl;
	cout << "parent_entry->inode_block: " << parent_entry->inode_block << endl;
	fs_direntry parent_directory[FS_DIRENTRIES];
	
	if (inode->size * FS_DIRENTRIES == parent_entry->entries.size()) {
		cout << "allocate disk block in parent_entry" << endl;
		unsigned int next_disk_block = next_free_disk_block();
		if (next_disk_block == FS_DISKSIZE && inode->size < FS_MAXFILEBLOCKS) {
			disk_blocks[new_entry_inode_block] = 0;
			delete inode;
			return -1;
		}
		cout << "have enough space for new parent_entry block" << endl;
		disk_blocks[next_disk_block] = 1;
		inode->blocks[inode->size] = next_disk_block;
		inode->size += 1;
		memset(parent_directory, 0, FS_BLOCKSIZE);
	}else{
		disk_readblock(inode->blocks[inode->size - 1], parent_directory);
	}

	parent_entry->entries[split_path.back()] = new entry(new_entry_inode_block, parent_entry->entries.size());
	fs_direntry new_direntry;
	new_direntry.inode_block = new_entry_inode_block;
	strcpy(new_direntry.name, split_path.back());
	cout << "sizeof(new_direntry): " << sizeof(new_direntry) << endl;
	cout << "parent_blocks_index: " << parent_entry->entries[split_path.back()]->parent_blocks_index << endl;
	cout << "sizeof(fs_direntry): " << sizeof(fs_direntry) << endl;
	cout << "inode->blocks: " << inode->blocks << endl;
	
	//memcpy((void*)(inode->blocks + ((parent_entry->entries[split_path.back()]->parent_blocks_index)*sizeof(fs_direntry))), 
	//		(void*)&new_direntry, sizeof(new_direntry));
	parent_directory[parent_entry->entries[split_path.back()]->parent_blocks_index] = new_direntry;
	cout << "new_direntry copied into parent_entry" << endl;

	fs_inode new_inode;
	new_inode.type = type[0];
	strcpy(new_inode.owner, username);
	new_inode.size = 0;
	cout << "new entry created" << endl;

	disk_writeblock(inode->blocks[inode->size - 1], parent_directory);
	disk_writeblock(new_entry_inode_block, &new_inode);
	disk_writeblock(parent_entry->inode_block, inode);
	cout << "entry written to disk" << endl;

	delete inode;
	return 0;
}


filesystem::entry* filesystem::recurse_filesystem(const char* username, vector<char*> &split_path, 
						  unsigned int path_index, entry* dir, 
						  fs_inode* &inode, char req_type) {
	//User trying to access directory they do not own
	if (strcmp(inode->owner, username)) 
		return nullptr;
	
	//Directory is empty
	if (inode->size == 0) {
		//Name/path of new directory/file is valid
		if (split_path.size()-1 == path_index && req_type == 'c') 
			return dir;

		//Directory/file read/write/delete is invalid
		return nullptr;
	}

	//Directory/file exists, read/write/delete valid, return parent directory
	if (split_path.size()-1 == path_index && req_type != 'c' && dir->entries.find(split_path[path_index]) != dir->entries.end()) {
		return dir;
	}

	if (dir->entries.find(split_path[path_index]) != dir->entries.end()) {
		disk_readblock(dir->entries[split_path[path_index]]->inode_block, inode);
		return recurse_filesystem(username, split_path, path_index+1, dir->entries[split_path[path_index]], inode, req_type);
	}

	return nullptr;
}


unsigned int filesystem::next_free_disk_block() {
	for (unsigned int i = 0; i < FS_DISKSIZE; ++i) {
		if (!disk_blocks[i]) {return i;}
	}
	return FS_DISKSIZE;
}



/*
//Internal entry does not mirror disk
	unsigned int num_direntries = fs_inode->size * FS_DIRENTRIES;
	if (num_direntries != dir->entries.size()) {
		for (int i = 0; i < num_direntries; ++i) {
			fs_direntry temp;
			memcpy((void*)&temp, (void*)inode->blocks+(i*sizeof(fs_direntry)), sizeof(fs_direntry));
			dir->entries[temp->name] = new entry(temp->inode_block);
		}
	}
*/













