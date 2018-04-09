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

template <typename T>
static void print_debug_helper(T t) 
{
    cout << t << endl;
}

template<typename T, typename... Args>
static void print_debug_helper(T t, Args... args) {
    cout << t << ", ";
    print_debug_helper(args...);
}

template<typename T, typename... Args>
static void print_debug(T t, Args... args) {
    cout_lock.lock();
    cout << "\nDEBUG: " << t << ", ";
    print_debug_helper(args...);
    cout_lock.unlock();
}

template <typename T>
static void print_debug(T t) 
{
	cout_lock.lock();
    cout << "\nDEBUG: " << t << endl;
    cout_lock.unlock();
}

#define MAX_REQUEST_NAME 13

//Filesystem constructor
filesystem::filesystem() {
	bitset<FS_DISKSIZE> used(0);
	index_disk(used);
	
	//Init free blocks, block zero is used for root
	for(unsigned int i = 1; i < FS_DISKSIZE; ++i){
		if(!used[i])
			free_blocks.push(i);
	}
	print_debug("FREE DISK BLOCKS:", free_blocks.size());
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
	//Split apart request in a vector with " " as delimiter
	handler_func_t handler = nullptr;
	vector<string> request_args = split_request(request, " ");
	char request_name[MAX_REQUEST_NAME + 1];
	unsigned int num_request_args = 0;

	// ----- HANDLE REQUEST ----- //
	switch(request[3]){
		case 'S':
			strcpy(request_name, "FS_SESSION");
			handler = &filesystem::new_session;
			num_request_args = 3;
			break;
		case 'C':
			strcpy(request_name, "FS_CREATE");
			handler = &filesystem::create_entry;
			num_request_args = 5;
			break;
		case 'D':
			strcpy(request_name, "FS_DELETE");
			handler = &filesystem::delete_entry;
			num_request_args = 4;
			break;
		case 'R':
			strcpy(request_name, "FS_READBLOCK");
			handler = &filesystem::access_entry;
			request_args.push_back(to_string(READ));
			num_request_args = 6;
			break;
		case 'W':
			strcpy(request_name, "FS_WRITEBLOCK");
			handler = &filesystem::access_entry;
			num_request_args = 7;

			//Append write data to 'request_args' vector
			request_args.push_back(to_string(WRITE));
			char data[FS_BLOCKSIZE];
			memcpy(data, request + request_size - FS_BLOCKSIZE, FS_BLOCKSIZE);
			request_args.emplace_back(data, FS_BLOCKSIZE);
			break;

		default:
			return;
	};
	
	if(request_args.size() != num_request_args)
		return;

	//Verify the request we handled was the one we thought it was
	if(strcmp(request_args[REQUEST_NAME].c_str(), request_name))
		return;

	if((this->*handler)(username, request_args) == -1)
		return;

	//Send response back to client on success
	ostringstream response;
	response << request_args[SESSION] << " " << request_args[SEQUENCE];
	send_response(client, username, request_args);
	return;
}





void filesystem::index_disk(bitset<FS_DISKSIZE>& used){
	//root-node
	fs_direntry root;
	strcpy(root.name, " ");
	root.inode_block = 0;

	vector<fs_direntry> dirs;
	dirs.push_back(root);
	index_disk_helper(used, dirs, true);
}


///FILES, check if direntrys inode # is zero, (it's in use if it is zero)
void filesystem::index_disk_helper(bitset<FS_DISKSIZE>& used, vector<fs_direntry> &dirs, bool is_root){
	if(dirs.empty())
		return;

	fs_direntry dir = dirs.back();
	dirs.pop_back();
	if(dir->inode_block == 0 && !is_root){
		return index_disk_helper(used, dirs, false);
	}

	used[dir.inode_block] = 1;
	fs_inode node;
	disk_readblock(dir.inode_block, &node);
	for(unsigned int i = 0; i < node.size; ++i){
		used[node.blocks[i]] = 1;
		if(node.type == 'd'){
			fs_direntry folders[FS_DIRENTRIES];
			disk_readblock(node.blocks[i], folders);
			dirs.insert(dirs.end(), folders, folders + FS_DIRENTRIES);
		}
	}
	return index_disk_helper(used, dirs, false);
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

	internal_lock.lock();
	//Find smallest free disk block for new entry's inode, mark it as used
	if(free_blocks.empty()){
		internal_lock.unlock();
		return -1;
	}
	unsigned int new_entry_inode_block = free_blocks.front();
	free_blocks.pop();
	internal_lock.unlock();

	filesystem::recurse_args recurse_fs_args;
	recurse_fs_args.disk_block = 0;
	print_debug(args[PATH]);
	bool parent_dir_found = recurse_filesystem(username, args[PATH], recurse_fs_args, 'c');
	print_debug(args[PATH]);

	//Path provided in fs_create() is invalid
	if(!parent_dir_found || args[PATH].size() > FS_MAXFILENAME){
		pthread_rwlock_unlock(&block_lock[recurse_fs_args.disk_block]);
		delete recurse_fs_args.inode;
		return -1;
	}
	
	bool added_new_block = false;
	print_debug("Blocks index:",recurse_fs_args.blocks_index);
	//If new direntry for this new directory/file will require another data block in parent directory's inode...
	if (recurse_fs_args.inode->size <= recurse_fs_args.blocks_index) {
		internal_lock.lock();
		unsigned int next_disk_block;
		//...find the smallest free disk block for parent directory's inode to use...
		if(free_blocks.empty() || recurse_fs_args.inode->size == FS_MAXFILEBLOCKS){
			free_blocks.push(new_entry_inode_block);
			internal_lock.unlock();
			pthread_rwlock_unlock(&block_lock[recurse_fs_args.disk_block]);
			delete recurse_fs_args.inode;
			return -1;
		}
		else{
			next_disk_block = free_blocks.front();
			free_blocks.pop();
		}
		internal_lock.unlock();
		//Add new data block to inode, increment size of inode (# blocks in inode)
		recurse_fs_args.inode->blocks[recurse_fs_args.inode->size] = next_disk_block;
		recurse_fs_args.inode->size++;

		//Initialize array of direntries at this new block in parent directory's blocks[] to be empty
		memset(recurse_fs_args.folders, 0, FS_BLOCKSIZE);
		added_new_block = true;
	}

	//Create new external direntry for directory/file being created
	fs_direntry new_direntry;
	new_direntry.inode_block = new_entry_inode_block;
	strcpy(new_direntry.name, args[PATH].c_str());

	//Place new_direntry in next unoccupied index 
	//(parent_directory[] represents the direntries in the block at the highest index in parent directory's blocks[])
	recurse_fs_args.folders[recurse_fs_args.folder_index] = new_direntry;

	//Create new inode for directory/file being created
	fs_inode new_inode;
	new_inode.type = args[INODE_TYPE][0];
	strcpy(new_inode.owner, username);
	new_inode.size = 0;


	//Write inode of new directory/file to reserved disk block
	disk_writeblock(new_entry_inode_block, &new_inode);

	//update directory block to now include the new direntry
	//NOTE: this will be a brand new directory block if parent inode size increased
	//		so we do not need to shadow.
	disk_writeblock(recurse_fs_args.inode->blocks[recurse_fs_args.blocks_index], recurse_fs_args.folders);

	//Write inode of parent directory to disk if we added a new directory block
	if(added_new_block)
		disk_writeblock(recurse_fs_args.disk_block, recurse_fs_args.inode);

	pthread_rwlock_unlock(&block_lock[recurse_fs_args.disk_block]);
	//Delete new inode object created above
	delete recurse_fs_args.inode;
	return 0;
}


// ----- FS_DELETE() RESPONSE -----//
//Request Format:  FS_DELETE <session> <sequence> <pathname><NULL>
//Response Format: <size><NULL><session> <sequence><NULL>
int filesystem::delete_entry(const char *username, vector<string>& args){
	//Check validity of session/sequence
	try{
		if (!users[username]->session_request(stoul(args[SESSION]),
											  stoul(args[SEQUENCE])))
			return -1;
	} catch(...){
		return -1;
	}

	//find file or folder to delete
	filesystem::recurse_args recurse_fs_args;
	recurse_fs_args.disk_block = 0;
	bool parent_dir_found = recurse_filesystem(username, args[PATH], recurse_fs_args, 'd');

	//Path provided in fs_delete() is invalid
	if(!parent_dir_found){
		pthread_rwlock_unlock(&block_lock[recurse_fs_args.disk_block]);
		delete recurse_fs_args.inode;
		return -1;
	}

	//read in dir inode
	fs_direntry dir_to_delete = recurse_fs_args.folders[recurse_fs_args.folder_index];
	fs_inode inode_to_delete;
	pthread_rwlock_wrlock(&block_lock[dir_to_delete.inode_block]);
	disk_readblock(dir_to_delete.inode_block, &inode_to_delete);

	//if inode is a non-empty directory, don't delete
	if((inode_to_delete.type == 'd' && inode_to_delete.size != 0) ||
		strcmp(inode_to_delete.owner, username)){
		pthread_rwlock_unlock(&block_lock[dir_to_delete.inode_block]);
		pthread_rwlock_unlock(&block_lock[recurse_fs_args.disk_block]);
		delete recurse_fs_args.inode;
		return -1;
	}

	//if inode is a file, free all file blocks
	if(inode_to_delete.type == 'f'){
		internal_lock.lock();
		for(unsigned int i = 0; i < inode_to_delete.size; ++i){
			free_blocks.push(inode_to_delete.blocks[i]);
		}
	}

	//free inode block
	free_blocks.push(dir_to_delete.inode_block);
	internal_lock.unlock();

	//remove direntry from directory
	memset((void*)&(recurse_fs_args.folders[recurse_fs_args.folder_index]), 0, sizeof(fs_direntry));

	//check if directory is empty
	bool folder_block_empty = true;
	for(unsigned int i = 0; i < FS_DIRENTRIES; ++i){
		if (((char*)recurse_fs_args.folders + (i * sizeof(fs_direntry)))[0] != '\0'){
			folder_block_empty = false;
			break;
		}
	}

	if(folder_block_empty){
		//if parent_inode size can be shrunk, update blocks
		--(recurse_fs_args.inode->size);
		for(unsigned int i = recurse_fs_args.blocks_index; i < recurse_fs_args.inode->size; ++i){
			recurse_fs_args.inode->blocks[i] = recurse_fs_args.inode->blocks[i + 1];
		}
		//write parent inode, to complete delete.
		disk_writeblock(recurse_fs_args.disk_block, recurse_fs_args.inode);
	}else{
		//write updated directory to disk
		disk_writeblock(recurse_fs_args.inode->blocks[recurse_fs_args.blocks_index], recurse_fs_args.folders);
	}
	pthread_rwlock_unlock(&block_lock[dir_to_delete.inode_block]);
	pthread_rwlock_unlock(&block_lock[recurse_fs_args.disk_block]);
	return 0;
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
		pthread_rwlock_unlock(&block_lock[recurse_fs_args.disk_block]);
		delete recurse_fs_args.inode;
		return -1;
	}

	//Read in inode of file being read/written to
	fs_inode file;
	fs_direntry dir = recurse_fs_args.folders[recurse_fs_args.folder_index];
	if(type == 'r'){
		pthread_rwlock_rdlock(&block_lock[dir.inode_block]);
	}else{
		pthread_rwlock_wrlock(&block_lock[dir.inode_block]);
	}
	disk_readblock(recurse_fs_args.folders[recurse_fs_args.folder_index].inode_block, (void*)&file);
	pthread_rwlock_unlock(&block_lock[recurse_fs_args.disk_block]);
	if(file.type == 'd' || strcmp(file.owner, username)){
		pthread_rwlock_unlock(&block_lock[dir.inode_block]);
		delete recurse_fs_args.inode;
		return -1;
	}

	//If fs_readblock() request...
	if (type == 'r'){
		//...and requested read block is valid...
		if (stoul(args[BLOCK]) < file.size){
			//...then read file data at requested block into buffer...
			char data[FS_BLOCKSIZE];
			disk_readblock(file.blocks[stoul(args[BLOCK])], (void*)data);

			//...and save it for response back to client
			args.emplace_back(data, FS_BLOCKSIZE);
		}
		//Invalid block number given
		else{
			pthread_rwlock_unlock(&block_lock[dir.inode_block]);
			delete recurse_fs_args.inode;
			return -1;
		}
	}

	//If fs_writeblock() request...
	else{
		//...and requested write block already valid in file...
		if (stoul(args[BLOCK]) < file.size){
			//...then write the write data to that block on disk
			disk_writeblock(file.blocks[stoul(args[BLOCK])], (void*)args[DATA].c_str());
		}
		//...and we must allocate a new block for write data...
		else if (stoul(args[BLOCK]) == file.size){
			internal_lock.lock();
			//Check to see if 1) there are any more free disk blocks and 2) this file can hold another block
			if(free_blocks.empty() || file.size == FS_MAXFILEBLOCKS){
				internal_lock.unlock();
				pthread_rwlock_unlock(&block_lock[dir.inode_block]);
				delete recurse_fs_args.inode;
				return -1;
			}
			file.blocks[stoul(args[BLOCK])] = free_blocks.front();
			free_blocks.pop();
			internal_lock.unlock();
			file.size++;

			//...and finally write the write data, and also the new block in this file's inode's blocks[]
			disk_writeblock(file.blocks[stoul(args[BLOCK])], (void*)args[DATA].c_str());	
			disk_writeblock(dir.inode_block, (void*)&file);
		}
		else{
			pthread_rwlock_unlock(&block_lock[dir.inode_block]);
			delete recurse_fs_args.inode;
			return -1;
		}
	}
	pthread_rwlock_unlock(&block_lock[dir.inode_block]);
	delete recurse_fs_args.inode;
	return 0;
}


//Helper function, sends responses back to client after successfully handling request
void filesystem::send_response(int client, const char *username, vector<string>& request_args){
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

	//Send response header and response
	string header(to_string(cipher_size));
	send(client, header.c_str(), header.length() + 1, 0);
	send(client, cipher, cipher_size, 0);
}


//Helper function, splits char * into segments delimited by 'token'
vector<string> filesystem::split_request(const string& request, const string& token) {
	vector<string> empty;
	vector<string> split;
	if(request.size() <= 1){
		return empty;
	}
	if(token == "/" && (request.front() != '/' || request.back() == '/' 
		|| (request[0] == '/' && request[1] == '/'))){
		return empty;
	}
	print_debug("spliting request:", request, "by token:", token);
	ostringstream entry;
	unsigned int count = 0;
	for(unsigned int i = (token == "/")? 1 : 0; i < request.size(); ++i){
		//path can't contain spaces
		if(token == "/" && request[i] == ' '){
			return empty;
		}
		if(request[i] == '\0'){
			break;
		}
		if(token.find_first_of(request[i]) != string::npos){
			if(count == 0 || (token == "/" && count > FS_MAXFILENAME)){
				//two delimiters in a row, return
				return empty;
			}
			split.push_back(entry.str());
			print_debug(split.back());
			entry.clear();
			entry.str("");
			count = 0;
		}else{
			entry << request[i];
			++count;
		}
	}

	if(count == 0 || (token == "/" && count > FS_MAXFILENAME)){
		return empty;
	}else{
		split.push_back(entry.str());
	}
	return split;
}


//Function to recursively search for valid parent directory of file/directory sent in request
bool filesystem::recurse_filesystem(const char *username, std::string& path, recurse_args &args, char req_type){
	//Read in inode of root directory from disk
	args.inode = new fs_inode;

	//Split given path into individual directories
	vector<string> split_path = split_request(path, "/");
	if(split_path.size() == 0){
		return false;
	}
	path = split_path.back();
	pthread_rwlock_rdlock(&block_lock[args.disk_block]);
	disk_readblock(args.disk_block, args.inode);

	//Traverse filesystem to find the correct parent directory
	return recurse_filesystem_helper(username, split_path, 0, args, req_type);
}


//Helper for recurse_filesystem(), actually has recursive call to traverse given path, checking if it is valid
bool filesystem::recurse_filesystem_helper(const char* username, std::vector<std::string> &split_path, 
											unsigned int path_index, recurse_args &args, char req_type) {
	//User trying to access directory they do not own
	if (args.inode->owner[0] != '\0' && strcmp(args.inode->owner, username)) 
		return false;
	
	//Check to see if requested file/directory is in this directory (e.g. is this the parent directory?)
	bool found = search_directory(args, split_path[path_index].c_str());

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
		unsigned int old_disk_block = args.disk_block;
		args.disk_block = args.folders[args.folder_index].inode_block;
		pthread_rwlock_rdlock(&block_lock[args.disk_block]);
		pthread_rwlock_unlock(&block_lock[old_disk_block]);
		disk_readblock(args.disk_block, args.inode);
		if (args.inode->type == 'f')
			return false;
		return recurse_filesystem_helper(username, split_path, path_index+1, args, req_type);
	}

	return false;
}

//WILL COMMMENT LATER
bool filesystem::search_directory(recurse_args &args, const char* name){
	unsigned int first_empty_dir_block = 0;
	unsigned int first_empty_folder_index = 0;
	fs_direntry first_empty_folders[FS_DIRENTRIES];
	memset(args.folders, 0, FS_DIRENTRIES*sizeof(fs_direntry));
	bool first_empty = true;
	for(unsigned int i = 0; i < args.inode->size; ++i){
		disk_readblock(args.inode->blocks[i], args.folders);

		for(unsigned int j = 0; j < FS_DIRENTRIES; ++j){
			if (((char*)args.folders + (j * sizeof(fs_direntry)))[0] == '\0'){
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
				return true;
			}
		}
	}
	if(first_empty){
		args.blocks_index = args.inode->size;
	}else{
		args.blocks_index = first_empty_dir_block;
	}
	args.folder_index = first_empty_folder_index;
	memcpy(args.folders, first_empty_folders, FS_DIRENTRIES*sizeof(fs_direntry));
	return false;
}