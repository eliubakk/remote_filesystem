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

	//print("building session response...");

	//Split apart request in a vector with " " as delimiter
	handler_func_t handler = nullptr;
	vector<string> request_args = split_request(request, " ");
	char request_name[MAX_REQUEST_NAME + 1];

	// ----- HANDLE REQUEST ----- //
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

			//Append write data to 'request_args' vector
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

	//Verify the request we handled was the one we thought it was
	if(strcmp(request_args[REQUEST_NAME].c_str(), request_name))
		return;

	//print("request_name: ", false);
	//print(request_args[REQUEST_NAME]);

	if((this->*handler)(username, request_args) == -1)
		return;

	//Send response back to client on success
	ostringstream response;
	response << request_args[SESSION] << " " << request_args[SEQUENCE];
	send_response(client, username, request_args);
	return;
}


//Function for thread-safe output using 'cout_lock' mutex
void filesystem::print(const char to_output[], bool endl) {
	cout_lock.lock();
	cout << "request_name: " << to_output;
	if (endl) {cout << endl;}
	cout_lock.unlock();	
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

	//print("have enough space for new entry");

	filesystem::recurse_args recurse_fs_args;
	recurse_fs_args.disk_block = 0;
	bool parent_dir_found = recurse_filesystem(username, args[PATH], recurse_fs_args, 'c');

	//print("path after recurse: ", false); 
	//print(args[PATH]);

	//Path provided in fs_create() is invalid
	if(!parent_dir_found){
		//print("path invalid");

		delete recurse_fs_args.inode;
		return -1;
	}

	//print("parent_dir found, path valid");

	//parent_directory[] represents the direntries in the block at the highest index in parent directory's blocks[]
	//fs_direntry parent_directory[FS_DIRENTRIES];
	
	//If new direntry for this new directory/file will require another data block in parent directory's inode...
	if (recurse_fs_args.inode->size <= recurse_fs_args.blocks_index) {
		//print("allocate disk block in parent_entry");

		unsigned int next_disk_block;
		//...find the smallest free disk block for parent directory's inode to use...
		if(free_blocks.empty() || recurse_fs_args.inode->size == FS_MAXFILEBLOCKS){
			free_blocks.push(new_entry_inode_block);
			delete recurse_fs_args.inode;
			return -1;
		}
		else{
			next_disk_block = free_blocks.front();
			free_blocks.pop();
		}

		//print("have enough space for new parent_entry block");

		//Add new data block to inode, increment size of inode (# blocks in inode)
		recurse_fs_args.inode->blocks[recurse_fs_args.inode->size] = next_disk_block;
		recurse_fs_args.inode->size++;

		//Initialize array of direntries at this new block in parent directory's blocks[] to be empty
		memset(recurse_fs_args.folders, 0, FS_BLOCKSIZE);
	}

	//Create new external direntry for directory/file being created
	fs_direntry new_direntry;
	new_direntry.inode_block = new_entry_inode_block;
	strcpy(new_direntry.name, args[PATH].c_str());

	//print("sizeof(new_direntry): ", false);
	//print(sizeof(new_direntry));
	//print("parent_blocks_index: ", false);
	//print(recurse_fs_args.blocks_index);

	//Place new_direntry in next unoccupied index 
	//(parent_directory[] represents the direntries in the block at the highest index in parent directory's blocks[])
	recurse_fs_args.folders[recurse_fs_args.folder_index] = new_direntry;
	//print("new_direntry copied into parent_entry");

	//Create new inode for directory/file being created
	fs_inode new_inode;
	new_inode.type = args[INODE_TYPE][0];
	strcpy(new_inode.owner, username);
	new_inode.size = 0;
	//print("new entry created");

	//will need to change this, probably create a function that shadows.

	//Update the blocks[] array of the parent directory of the new directory/file on disk 
	//to now include the direntry/data block # of new directory/file
	//print("block: ", false);
	//print(recurse_fs_args.blocks_index);
	disk_writeblock(recurse_fs_args.inode->blocks[recurse_fs_args.blocks_index], recurse_fs_args.folders);
	//print("folders written");

	//Write inode of parent directory to disk
	disk_writeblock(recurse_fs_args.disk_block, recurse_fs_args.inode);
	//print("parent inode written");

	//Write inode of new directory/file to reserved disk block
	disk_writeblock(new_entry_inode_block, &new_inode);
	//print("entry written to disk");

	//Delete new inode object created above
	delete recurse_fs_args.inode;
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
	char type = stoul(args[ACCESS_TYPE]) == READ ? 'r' : 'w';

	//Recurse filesystem to make sure parent directory of file being read/written to exists
	filesystem::recurse_args recurse_fs_args;
	recurse_fs_args.disk_block = 0;
	bool parent_dir_found = recurse_filesystem(username, args[PATH], recurse_fs_args, type);
	if (!parent_dir_found){
		delete recurse_fs_args.inode;
		return -1;
	}

	//Read in inode of file being read/written to
	fs_inode file;
	disk_readblock(recurse_fs_args.folders[recurse_fs_args.folder_index].inode_block, (void*)&file);

	//If fs_readblock() request...
	if (type == 'r'){
		//...and requested read block is valid...
		if (stoul(args[BLOCK]) < file.size){
			//...then read file data at requested block into buffer...
			char data[FS_BLOCKSIZE];
			disk_readblock(file.blocks[stoul(args[BLOCK])], (void*)data);

			//print("reading data: ", false);
			for (unsigned int i = 0; i < FS_BLOCKSIZE; ++i){
				//print(data[i], false);
			}
			//print("");

			//...and save it for response back to client
			args.emplace_back(data, FS_BLOCKSIZE);
		}
		//Invalid block number given
		else
			return -1;
	}

	//If fs_writeblock() request...
	else{
		//...and requested write block already valid in file...
		if (stoul(args[BLOCK]) < file.size)
			//...then write the write data to that block on disk
			disk_writeblock(file.blocks[stoul(args[BLOCK])], (void*)args[DATA].c_str());

		//...and we must allocate a new block for write data...
		else if (stoul(args[BLOCK]) == file.size){
			//print("growing file");

			//Check to see if 1) there are any more free disk blocks and 2) this file can hold another block
			if(free_blocks.empty() || file.size == FS_MAXFILEBLOCKS)
				return -1;
			//print("new file block allocated");
			file.blocks[stoul(args[BLOCK])] = free_blocks.front();
			free_blocks.pop();
			file.size++;

			//print("args[BLOCK]: ", false);
			//print(args[BLOCK]);
			//print("file.blocks[args[]]: ", false);
			//print(file.blocks[stoul(args[BLOCK])]);
			//print("args[DATA]: ", false);
			//print(args[DATA]);

			//...and finally write the write data, and also the new block in this file's inode's blocks[]
			disk_writeblock(file.blocks[stoul(args[BLOCK])], (void*)args[DATA].c_str());	
			disk_writeblock(recurse_fs_args.folders[recurse_fs_args.folder_index].inode_block, (void*)&file);	
		}
		else
			return -1;
	}
	//print("successfully accessed data from the block");
	return 0;
}


//Helper function, sends responses back to client after successfully handling request
void filesystem::send_response(int client, const char *username, vector<string>& request_args){
	//print("response built");

	//Correctly format response
	string response =  request_args[SESSION] + " " + request_args[SEQUENCE];
	unsigned int raw_response_size = response.size() + 1;
	char raw_response[response.size() + 1 + FS_BLOCKSIZE]; 
	memcpy(raw_response, response.c_str(), response.size() + 1);

	//If responding to a read, append read data to response
	if (request_args[REQUEST_NAME][3] == 'R'){
		memcpy(raw_response + response.size() + 1, request_args[DATA].c_str(), FS_BLOCKSIZE);
		raw_response_size += FS_BLOCKSIZE;
	}

	//Encrypt response
	unsigned int cipher_size = 0;
	const char* cipher = (char*) fs_encrypt(
		users[username]->password(), (void*)raw_response,
		raw_response_size, &cipher_size);

	//print("response encrypted");

	//Send response header and response
	string header(to_string(cipher_size));
	send(client, header.c_str(), header.length() + 1, 0);
	send(client, cipher, cipher_size, 0);

	//print("session response sent");
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

	//print("size: ", false);
	//print(split.size());
	return split;
}


//Function to recursively search for valid parent directory of file/directory sent in request
bool filesystem::recurse_filesystem(const char *username, std::string& path, recurse_args &args, char req_type){
	//Split given path into individual directories
	vector<string> split_path = split_request(path, "/");
	path = split_path.back();

	//print("path split");

	//FIX THIS????
	//Read in inode of root directory from disk
	args.inode = new fs_inode;
	disk_readblock(args.disk_block, args.inode);

	//print(split_path[0]);

	//Traverse filesystem to find the correct parent directory
	//print("recurse_filesystem to find parent_entry");
	return recurse_filesystem_helper(username, split_path, 0, args, req_type);
}


//Helper for recurse_filesystem(), actually has recursive call to traverse given path, checking if it is valid
bool filesystem::recurse_filesystem_helper(const char* username, std::vector<std::string> &split_path, 
											unsigned int path_index, recurse_args &args, char req_type) {
	//print("In directory: ", false);
	//print(split_path[path_index]);

	//User trying to access directory they do not own
	if (args.inode->owner[0] != '\0' && strcmp(args.inode->owner, username)) 
		return false;
	

	//Check to see if requested file/directory is in this directory (e.g. is this the parent directory?)
	bool found = search_directory(args, split_path[path_index].c_str());

	//print("folder_index: ", false);
	//print(args.folder_index);
	//print("new_direntry inode block: ", false);
	//print(args.folders[folder_index].inode_block);
	//print("parent_inode block: ", false);
	//print(args.disk_block);

	if (split_path.size()-1 == path_index){
		//If this is the last entry of path, return valid if:
		//(1) Create and Name/path of new directory/file doesn't exist
		//(2) Not create and Name of directory/file does exist
		if(!found ^ (req_type != 'c')){
			return true;
		}
		else
			return false;
	}

	//WILL COMMENT LATER
	if (found) {
		args.disk_block = args.folders[args.folder_index].inode_block;
		disk_readblock(args.disk_block, args.inode);
		if (args.inode->type == 'f')
			return false;
		return recurse_filesystem_helper(username, split_path, path_index+1, args, req_type);
	}

	return false;
}

//WILL COMMMENT LATER
bool filesystem::search_directory(recurse_args &args, const char* name){
	//print("\n\n\nSearching directory...");
	//print("size of directory: ", false);
	//print(args.inode->size);

	unsigned int first_empty_dir_block = 0;
	unsigned int first_empty_folder_index = 0;
	fs_direntry first_empty_folders[FS_DIRENTRIES];
	memset(args.folders, 0, FS_DIRENTRIES*sizeof(fs_direntry));
	bool first_empty = true;
	for(unsigned int i = 0; i < args.inode->size; ++i){
		disk_readblock(args.inode->blocks[i], args.folders);

		for(unsigned int j = 0; j < FS_DIRENTRIES; ++j){
			if (((char*)args.folders + (j * sizeof(fs_direntry)))[0] == '\0'){
				//print("found empty folder_index: ", false);
				//print(args.folder_index);

				if(first_empty){
					first_empty = false;
					first_empty_dir_block = i;
					first_empty_folder_index = j;
					memcpy(first_empty_folders, args.folders, FS_DIRENTRIES*sizeof(fs_direntry));
				}
				continue;
			}
			if(!strcmp(args.folders[j].name, name)){
				args.blocks_index = i;
				args.folder_index = j;
				//print("folder found\n\n");
				return true;
			}
		}
	}
	//print("folder not found\n\n");

	args.blocks_index = first_empty_dir_block;
	args.folder_index = first_empty_folder_index;
	memcpy(args.folders, first_empty_folders, FS_DIRENTRIES*sizeof(fs_direntry));
	return false;
}