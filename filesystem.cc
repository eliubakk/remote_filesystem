#include "filesystem.h"
#include "fs_server.h"
#include <cassert>
#include <cstring>
#include <string>
#include <sstream>
#include <sys/socket.h>
#include <iostream>
#include <stdexcept>

using namespace std;

typedef int (filesystem::*handler_func_t)(const char*, vector<string>&);

#define MAX_REQUEST_NAME 13

//Filesystem constructor
filesystem::filesystem() {
	//Init free blocks, block zero is used for root
	for(unsigned int i = 1; i < FS_DISKSIZE; ++i){
		free_blocks.push(i);
	}
}


//Filesystem destructor
filesystem::~filesystem(){
	for(auto user : users)
		delete user.second;
}


//Add user to internal filesystem
bool filesystem::add_user(const string& username, const string& password){
	if(users.find(username) != users.end())
		return false;
	users[username] = new fs_user(username.c_str(), password.c_str());
	return true;
}


//Check if user exists in filesystem
bool filesystem::user_exists(const string& username){
	return (users.find(username) != users.end());
}


//Return password for given username
const char* filesystem::password(const string& username){
	return users[username]->password();
}

void filesystem::handle_request(int client, const char *username, char *request,
							unsigned int request_size){
	cout_lock.lock();
	cout << "building session response..." << endl;
	cout_lock.unlock();

	handler_func_t handler = nullptr;
	vector<string> request_args = split_request(request, " ");
	char request_name[MAX_REQUEST_NAME + 1];
	//Handle request
	switch(request[3]){
		case 'S':
			strcpy(request_name, "FS_SESSION");
			handler = &filesystem::new_session;
			break;
		case 'C':
			strcpy(request_name, "FS_CREATE");
			handler = &filesystem::create_entry;
			break;
		case 'D':
			strcpy(request_name, "FS_DELETE");
			handler = &filesystem::delete_entry;
			break;
		case 'R':
			strcpy(request_name, "FS_READBLOCK");
			handler = &filesystem::access_entry;
			request_args.push_back(to_string(READ));
			break;
		case 'W':
			strcpy(request_name, "FS_WRITEBLOCK");
			handler = &filesystem::access_entry;
			request_args.push_back(to_string(WRITE));
			char data[FS_BLOCKSIZE];
			memcpy(data, request + request_size - FS_BLOCKSIZE, FS_BLOCKSIZE);
			request_args.emplace_back(data, FS_BLOCKSIZE);
			cout_lock.lock();
			cout << "write data: ";
			for (unsigned int i = 0; i < FS_BLOCKSIZE; ++i){
				cout << data[i];
			}
			cout << endl;
			cout_lock.unlock();
			break;
		default:
			return;
	};

	if(strcmp(request_args[REQUEST_NAME].c_str(), request_name))
		return;

	cout_lock.lock();
	cout << "request_name: " << request_args[REQUEST_NAME] << endl;
	cout_lock.unlock();	

	if((this->*handler)(username, request_args) == -1)
		return;

	//Send response back to client on success
	ostringstream response;
	response << request_args[SESSION] << " " << request_args[SEQUENCE];
	send_response(client, username, request_args);
	return;
}


// ----- FS_SESSION() RESPONSE ----- //
//Request format:  FS_SESSION <session> <sequence><NULL>
//Response format: <size><NULL><session> <sequence><NULL>
int filesystem::new_session(const char *username, vector<string>& args){
	//Given session # is meaningless, create new session ID with given sequence #
	try{
		unsigned int sequence = stoul(args[SEQUENCE]);
		args[SESSION] = to_string(users[username]->create_session(sequence));
	} catch(...){
		return -1;
	}
	return 0;
}

// ----- FS_CREATE() RESPONSE -----//
//Request Format:  FS_CREATE <session> <sequence> <pathname> <type><NULL>
//Response Format: <size><NULL><session> <sequence><NULL>
int filesystem::create_entry(const char *username, vector<string>& args){
	//Check validity of session/sequence
	try{
		if (!users[username]->session_request(stoul(args[SESSION]),
											  stoul(args[SEQUENCE])))
			return -1;
	} catch(...){
		return -1;
	}

	//Find smallest free disk block for new entry's inode, mark it as used
	if(free_blocks.empty())
		return -1;
	unsigned int new_entry_inode_block = free_blocks.front();
	free_blocks.pop();

	cout_lock.lock();
	cout << "have enough space for new entry" << endl;
	cout_lock.unlock();

	fs_inode* inode;
	unsigned int disk_block = 0;
	unsigned int block;
	fs_direntry folders[FS_DIRENTRIES];
	unsigned int folder_index;

	bool parent_dir_found = recurse_filesystem(username, args[PATH], inode, disk_block, block, folders, folder_index, 'c');
	cout_lock.lock();
	cout << "path after recurse: " << args[PATH] << endl;
	cout_lock.unlock();

	//Path provided in fs_create() is invalid
	if(!parent_dir_found){
		cout_lock.lock();
		cout << "path invalid" << endl;
		cout_lock.unlock();

		delete inode;
		return -1;
	}

	cout_lock.lock();
	cout << "parent_dir found, path valid" << endl;
	//cout << "folders[folder_index]->inode_block: " << folders[folder_index] << endl;
	cout_lock.unlock();

	//parent_directory[] represents the direntries in the block at the highest index in parent directory's blocks[]
	//fs_direntry parent_directory[FS_DIRENTRIES];
	
	//If new direntry for this new directory/file will require another data block in parent directory's inode...
	if (inode->size <= block) {
		cout_lock.lock();
		cout << "allocate disk block in parent_entry" << endl;
		cout_lock.unlock();

		unsigned int next_disk_block;
		//...find the smallest free disk block for parent directory's inode to use...
		if(free_blocks.empty() || inode->size == FS_MAXFILEBLOCKS){
			free_blocks.push(new_entry_inode_block);
			delete inode;
			return -1;
		}
		else{
			next_disk_block = free_blocks.front();
			free_blocks.pop();
		}

		cout_lock.lock();
		cout << "have enough space for new parent_entry block" << endl;
		cout_lock.unlock();

		//Add new data block to inode, increment size of inode (# blocks in inode)
		inode->blocks[inode->size] = next_disk_block;
		inode->size++;

		//Initialize array of direntries at this new block in parent directory's blocks[] to be empty
		memset(folders, 0, FS_BLOCKSIZE);
	}

	//Create new external direntry for directory/file being created
	fs_direntry new_direntry;
	new_direntry.inode_block = new_entry_inode_block;
	strcpy(new_direntry.name, args[PATH].c_str());

	cout_lock.lock();
	cout << "sizeof(new_direntry): " << sizeof(new_direntry) << endl;
	cout << "parent_blocks_index: " << block << endl;
	cout_lock.unlock();

	//Place new_direntry in next unoccupied index 
	//(parent_directory[] represents the direntries in the block at the highest index in parent directory's blocks[])
	folders[folder_index] = new_direntry;
	cout_lock.lock();
	cout << "new_direntry copied into parent_entry" << endl;
	cout_lock.unlock();

	//Create new inode for directory/file being created
	fs_inode new_inode;
	new_inode.type = args[INODE_TYPE][0];
	strcpy(new_inode.owner, username);
	new_inode.size = 0;
	cout_lock.lock();
	cout << "new entry created" << endl;
	cout_lock.unlock();

	//will need to change this, probably create a function that shadows.

	//Update the blocks[] array of the parent directory of the new directory/file on disk 
	//to now include the direntry/data block # of new directory/file
	cout << "block: " << block << endl;
	disk_writeblock(inode->blocks[block], folders);
	cout_lock.lock();
	cout << "folders written" << endl;
	cout_lock.unlock();
	//Write inode of parent directory to disk
	disk_writeblock(disk_block, inode);
	cout_lock.lock();
	cout << "parent inode written" << endl;
	cout_lock.unlock();

	//Write inode of new directory/file to reserved disk block
	disk_writeblock(new_entry_inode_block, &new_inode);
	cout_lock.lock();
	cout << "entry written to disk" << endl;
	cout_lock.unlock();

	//Delete new inode object created above
	delete inode;
	return 0;

}

// ----- FS_DELETE() RESPONSE -----//
//Request Format:  FS_DELETE <session> <sequence> <pathname><NULL>
//Response Format: <size><NULL><session> <sequence><NULL>
int filesystem::delete_entry(const char *username, vector<string>& args){
	assert(false);
}

// ----- FS_READBLOCK() RESPONSE -----//
//Request Format:  FS_READBLOCK <session> <sequence> <pathname> <block><NULL>
//Response Format: <size><NULL><session> <sequence><NULL><data>
// ----- FS_WRITEBLOCK() RESPONSE -----//
//Request Format:  FS_WRITEBLOCK <session> <sequence> <pathname> <block><NULL><data>
//Response Format: <size><NULL><session> <sequence><NULL>
int filesystem::access_entry(const char *username, vector<string>& args){
	//Check validity of session/sequence
	try{
		if (!users[username]->session_request(stoul(args[SESSION]),
											  stoul(args[SEQUENCE])))
			return -1;
	} catch(...){
		return -1;
	}
	fs_inode* inode;
	unsigned int disk_block = 0;
	unsigned int block;
	fs_direntry folders[FS_DIRENTRIES];
	unsigned int folder_index;
	char type = stoul(args[ACCESS_TYPE]) == READ ? 'r' : 'w';
	bool parent_dir_found = recurse_filesystem(username, args[PATH], inode, disk_block, block, folders, folder_index, type);
	if (!parent_dir_found){
		delete inode;
		return -1;
	}
	fs_inode file;
	disk_readblock(folders[folder_index].inode_block, (void*)&file);
	if (type == 'r'){
		if (stoul(args[BLOCK]) < file.size){
			char data[FS_BLOCKSIZE];
			disk_readblock(file.blocks[stoul(args[BLOCK])], (void*)data);
			cout_lock.lock();
			cout << "reading data: ";
			for (unsigned int i = 0; i < FS_BLOCKSIZE; ++i){
				cout << data[i];
			}
			cout << endl;
			cout_lock.unlock();
			args.emplace_back(data, FS_BLOCKSIZE);
		}
		else
			return -1;
	}
	else{
		if (stoul(args[BLOCK]) < file.size)
			disk_writeblock(file.blocks[stoul(args[BLOCK])], (void*)args[DATA].c_str());
		else if (stoul(args[BLOCK]) == file.size){
			cout_lock.lock();
			cout << "growing file" << endl;
			cout_lock.unlock();
			if(free_blocks.empty() || file.size == FS_MAXFILEBLOCKS)
				return -1;
			cout_lock.lock();
			cout << "new file block allocated" << endl;
			cout_lock.unlock();
			file.blocks[stoul(args[BLOCK])] = free_blocks.front();
			free_blocks.pop();
			file.size++;
			cout_lock.lock();
			cout << "args[BLOCK]: " << args[BLOCK] << endl;
			cout << "file.blocks[args[]]: " << file.blocks[stoul(args[BLOCK])] << endl;
			cout << "args[DATA]: " << args[DATA] << endl;
			cout_lock.unlock();
			disk_writeblock(file.blocks[stoul(args[BLOCK])], (void*)args[DATA].c_str());	
			disk_writeblock(folders[folder_index].inode_block, (void*)&file);	
		}
		else
			return -1;
	}
	cout_lock.lock();
	cout << "successfully accessed data from the block" << endl;
	cout_lock.unlock();
	return 0;
}

//Helper function, sends responses back to client after successfully handling request
void filesystem::send_response(int client, const char *username, vector<string>& request_args){
	cout_lock.lock();
	cout << "response built" << endl;
	cout_lock.unlock();

	string response =  request_args[SESSION] + " " + request_args[SEQUENCE];
	unsigned int raw_response_size = response.size() + 1;
	char raw_response[response.size() + 1 + FS_BLOCKSIZE]; 
	memcpy(raw_response, response.c_str(), response.size() + 1);
	if (request_args[REQUEST_NAME][3] == 'R'){
		memcpy(raw_response + response.size() + 1, request_args[DATA].c_str(), FS_BLOCKSIZE);
		raw_response_size += FS_BLOCKSIZE;
	}

	unsigned int cipher_size = 0;
	const char* cipher = (char*) fs_encrypt(
		users[username]->password(), (void*)raw_response,
		raw_response_size, &cipher_size);

	cout_lock.lock();
	cout << "response encrypted" << endl;
	cout_lock.unlock();

	string header(to_string(cipher_size));
	send(client, header.c_str(), header.length() + 1, 0);
	send(client, cipher, cipher_size, 0);

	cout_lock.lock();
	cout << "session response sent" << endl;
	cout_lock.unlock();
}


//Helper function, splits char * into segments delimited by 'token'
vector<string> filesystem::split_request(const string& request, const string& token) {
	vector<string> split;
	char request_cpy[request.size() + 1];
	strcpy(request_cpy, request.c_str());
	char* next_arg = strtok(request_cpy, token.c_str());
	while (next_arg) {
		split.push_back(next_arg);
		next_arg = strtok(0, token.c_str());
	}
	cout_lock.lock();
	cout << "size: " << split.size() << endl;
	cout_lock.unlock();
	return split;
}

bool filesystem::search_directory(fs_inode*& dir_inode, unsigned int &dir_block, 
									fs_direntry* folders, unsigned int &folder_index, const char* name){
	cout_lock.lock();
	cout << "\n\n\nSearching directory..." << endl;
	cout << "size of directory: " << dir_inode->size << endl;
	cout_lock.unlock();
	unsigned int first_empty_dir_block = 0;
	unsigned int first_empty_folder_index = 0;
	fs_direntry first_empty_folders[FS_DIRENTRIES];
	memset(folders, 0, FS_DIRENTRIES*sizeof(fs_direntry));
	bool first_empty = true;
	for(unsigned int i = 0; i < dir_inode->size; ++i){
		disk_readblock(dir_inode->blocks[i], folders);

		for(unsigned int j = 0; j < FS_DIRENTRIES; ++j){
			if (((char*)folders + (j * sizeof(fs_direntry)))[0] == '\0'){
				cout_lock.lock();
				cout << "found empty folder_index: " << folder_index << "...";
				cout_lock.unlock();
				if(first_empty){
					first_empty = false;
					first_empty_dir_block = i;
					first_empty_folder_index = j;
					memcpy(first_empty_folders, folders, FS_DIRENTRIES*sizeof(fs_direntry));
				}
				continue;
			}
			if(!strcmp(folders[j].name, name)){
				dir_block = i;
				folder_index = j;
				cout_lock.lock();
				cout << "folder found\n\n" << endl;
				cout_lock.unlock();
				return true;
			}
		}
	}
	cout_lock.lock();
	cout << "folder not found\n\n" << endl;
	cout_lock.unlock();
	dir_block = first_empty_dir_block;
	folder_index = first_empty_folder_index;
	memcpy(folders, first_empty_folders, FS_DIRENTRIES*sizeof(fs_direntry));
	return false;
}

bool filesystem::recurse_filesystem(const char *username, string& path, fs_inode*& inode, unsigned int &disk_block,
							unsigned int &block, fs_direntry* folders, unsigned int &folder_index, char req_type){
	//Split given path into individual directories
	vector<string> split_path = split_request(path, "/");
	path = split_path.back();
	cout_lock.lock();
	cout << "path split" << endl;
	cout_lock.unlock();

	//Read in inode of root directory from disk
	inode = new fs_inode;
	disk_readblock(disk_block, inode);
	cout_lock.lock();
	cout << split_path[0] << endl;
	cout_lock.unlock();

	//Traverse filesystem to find the correct parent directory
	cout_lock.lock();
	cout << "recurse_filesystem to find parent_entry" << endl;
	cout_lock.unlock();
	return recurse_filesystem_helper(username, split_path, 0, inode, disk_block, block, folders, folder_index, req_type);
}

//FIX THIS!!!!!!!!!!!
bool filesystem::recurse_filesystem_helper(const char* username, vector<string> &split_path, 
						  			unsigned int path_index, fs_inode*& inode, unsigned int &disk_block,
						  			unsigned int &block, 
									fs_direntry* folders, unsigned int &folder_index, char req_type) {
	cout_lock.lock();
	cout << "In directory: " << split_path[path_index] << endl;
	cout_lock.unlock();

	//User trying to access directory they do not own
	if (inode->owner[0] != '\0' && strcmp(inode->owner, username)) 
		return false;
	

	bool found = search_directory(inode, block, folders, folder_index, split_path[path_index].c_str());
	cout_lock.lock();
	cout << "folder_index: " << folder_index << endl;
	cout << "new_direntry inode block: " << folders[folder_index].inode_block << endl;
	cout << "parent_inode block: " << block << endl;
	cout_lock.unlock();

	if (split_path.size()-1 == path_index){
		//Create and Name/path of new directory/file doesn't exist
		//Not create and Name of directory/file does exist
		if(!found ^ (req_type != 'c')){
			return true;
		}
		else
			return false;
	}

	//This will probably need to change... this might be considered caching, which we should not do
	if (found) {
		disk_block = folders[folder_index].inode_block;
		disk_readblock(disk_block, inode);
		if (inode->type == 'f')
			return false;
		return recurse_filesystem_helper(username, split_path, path_index+1, inode, disk_block, block, folders, folder_index, req_type);
	}

	return false;
}