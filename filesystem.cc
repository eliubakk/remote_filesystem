#include "filesystem.h"
#include "fs_server.h"
#include <cassert>
#include <cstring>
#include <cctype>
#include <string>
#include <sstream>
#include <sys/socket.h>
#include <iostream>
#include <stdexcept>

using namespace std;

typedef int (filesystem::*handler_func_t)(const char*, vector<string>&);
#define MAX_REQUEST_NAME 13


// ------------- For thread-safe output ------------- //
template <typename T>
static void print_debug_helper(T t) 
{
    cout << t << endl;
}
template<typename T, typename... Args>
static void print_debug_helper(T t, Args... args) {
    cout << t;
    print_debug_helper(args...);
}
template<typename T, typename... Args>
static void print_debug(T t, Args... args) {
    cout_lock.lock();
    cout << "\nDEBUG: " << t;
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
// ------------- For thread-safe output ------------- //


//Filesystem constructor
filesystem::filesystem() {
	bitset<FS_DISKSIZE> used(0);

	//In case pre-existing filesystem existed, index new filesystem
	index_disk(used);
	
	//Initialize free blocks, block zero is used for root
	internal_lock.lock();
	for(unsigned int i = 1; i < FS_DISKSIZE; ++i){
		if(!used[i])
			free_blocks.push_back(i);
		pthread_rwlock_init(&block_lock[i], nullptr);
	}
	internal_lock.unlock();
}


//Filesystem destructor
filesystem::~filesystem(){
	for(auto user : users)
		delete user.second;
}


//Add user to internal filesystem
bool filesystem::add_user(const string& username, const string& password){
	if(user_exists(username))
		return false;
	internal_lock.lock();
	users[username] = new fs_user(username.c_str(), password.c_str());
	internal_lock.unlock();
	return true;
}


//Check if user exists in filesystem
bool filesystem::user_exists(const string& username){
	internal_lock.lock();
	bool exists = (users.find(username) != users.end());
	internal_lock.unlock();
	return exists; 
}


//Return password for given username
const char* filesystem::password(const string& username){
	internal_lock.lock();
	const char * pw = users[username]->password();
	internal_lock.unlock();
	return pw;
}


// ----------------- REQUEST HANDLER ----------------- //
// --- Makes proper handling call based on request --- //
void filesystem::handle_request(int client, const char *username, char *request,
							unsigned int request_size){
	//Split apart request in a vector with " " as delimiter
	handler_func_t handler = nullptr;
	unsigned int req_size = request_size;
	vector<string> request_args = split_request(request, ' ', req_size);

	if (strcmp(request_args[REQUEST_NAME].c_str(), "FS_SESSION")) {
		try{
			internal_lock.lock();
			if (!users[username]->session_request(stoul(request_args[SESSION]),
											  stoul(request_args[SEQUENCE]))) {
				internal_lock.unlock();
				return;
			}
		} catch(...){
			internal_lock.unlock();
			return;
		}
		internal_lock.unlock();
		//Error checking length of pathname
		if (request_args[PATH].size() > FS_MAXPATHNAME)
			return;
	}

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

			//On write request, append write data to 'request_args' vector
			if (request_size - req_size != FS_BLOCKSIZE) 
				return;

			request_args.push_back(to_string(WRITE));
			char data[FS_BLOCKSIZE];
			memcpy(data, request + req_size, FS_BLOCKSIZE);
			request_args.emplace_back(data, FS_BLOCKSIZE);
			break;

		default:
			return;
	};
	
	//If invalid request format, return
	//print_debug("requestsize: ", request_args.size(), " ", num_request_args);
	if(request_args.size() != num_request_args)
		return;

	//Verify the request we handled was the one we thought it was
	if(strcmp(request_args[REQUEST_NAME].c_str(), request_name))
		return;

	//Call correct handler function, return without sending response on failure
	if((this->*handler)(username, request_args) == -1)
		return;

	//Send response back to client on success
	ostringstream response;
	response << request_args[SESSION] << " " << request_args[SEQUENCE];
	send_response(client, username, request_args);
	return;
}


// ----- INDEX FILESYSTEM ----- //
//Recursively read in pre-existing filesystem data
void filesystem::index_disk(bitset<FS_DISKSIZE>& used){
	//root-node
	fs_direntry root;
	root.inode_block = 0;

	vector<fs_direntry> dirs;
	dirs.push_back(root);
	index_disk_helper(used, dirs, true);
}

void filesystem::index_disk_helper(bitset<FS_DISKSIZE>& used, vector<fs_direntry> &dirs, bool is_root){
	if(dirs.empty())
		return;

	fs_direntry dir = dirs.back();
	dirs.pop_back();
	if(dir.inode_block == 0 && !is_root){
		return index_disk_helper(used, dirs, false);
	}

	if (used[dir.inode_block] == 1 || dir.inode_block >= FS_DISKSIZE)
		return;

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
		internal_lock.lock();
		unsigned int sequence = stoul(args[SEQUENCE]);
		args[SESSION] = to_string(users[username]->create_session(sequence));
		internal_lock.unlock();
	} catch(...){
		internal_lock.unlock();
		return -1;
	}
	return 0;
}


// ----- FS_CREATE() RESPONSE -----//
//Request Format:  FS_CREATE <session> <sequence> <pathname> <type><NULL>
//Response Format: <size><NULL><session> <sequence><NULL>
int filesystem::create_entry(const char *username, vector<string>& args){
	if (args[INODE_TYPE][0] != 'd' && args[INODE_TYPE][0] != 'f')
		return -1;

	internal_lock.lock();

	//Find smallest free disk block for new entry's inode, mark it as used
	if(free_blocks.empty()){
		internal_lock.unlock();
		return -1;
	}
	unsigned int new_entry_inode_block = free_blocks.front();
	free_blocks.pop_front();
	internal_lock.unlock();

	//Recurse filesystem to find directory in which create will occur
	filesystem::recurse_args recurse_fs_args;
	recurse_fs_args.disk_block = 0;
	////print_debug(username, " trying to create entry: ", args[PATH]);
	bool parent_dir_found = recurse_filesystem(username, args[PATH], recurse_fs_args, 'c');

	//Path provided in fs_create() is invalid
	if(!parent_dir_found || args[PATH].size() > FS_MAXFILENAME){
		internal_lock.lock();
		pthread_rwlock_t* rwlock = &block_lock[recurse_fs_args.disk_block];
		internal_lock.unlock();
		//print_debug("277 unlock: ", recurse_fs_args.disk_block);
		pthread_rwlock_unlock(rwlock);
		delete recurse_fs_args.inode;
		return -1;
	}
	////print_debug(username, " creating entry: ", args[PATH]);

	//If new direntry for this new directory/file will require another data block in parent directory's inode...
	bool added_new_block = false;
	if (recurse_fs_args.inode->size <= recurse_fs_args.blocks_index) {
		internal_lock.lock();
		unsigned int next_disk_block;

		//...find the smallest free disk block for parent directory's inode to use...
		if(free_blocks.empty() || recurse_fs_args.inode->size == FS_MAXFILEBLOCKS){
			free_blocks.push_back(new_entry_inode_block);
			pthread_rwlock_t* rwlock = &block_lock[recurse_fs_args.disk_block];
			internal_lock.unlock();
			//print_debug("295 unlock: ", recurse_fs_args.disk_block);
			pthread_rwlock_unlock(rwlock);
			delete recurse_fs_args.inode;
			return -1;
		}
		else{
			next_disk_block = free_blocks.front();
			free_blocks.pop_front();
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

	//Update directory block to now include the new direntry
	//NOTE: this will be a brand new directory block if parent inode size increased
	//		so we do not need to shadow.
	disk_writeblock(recurse_fs_args.inode->blocks[recurse_fs_args.blocks_index], recurse_fs_args.folders);

	//Write inode of parent directory to disk if we added a new directory block
	if(added_new_block)
		disk_writeblock(recurse_fs_args.disk_block, recurse_fs_args.inode);
	//Delete new inode object created above
	internal_lock.lock();
	pthread_rwlock_t* rwlock = &block_lock[recurse_fs_args.disk_block];
	internal_lock.unlock();
	//print_debug("345 unlock: ", recurse_fs_args.disk_block);
	pthread_rwlock_unlock(rwlock);
	delete recurse_fs_args.inode;
	return 0;
}


// ----- FS_DELETE() RESPONSE -----//
//Request Format:  FS_DELETE <session> <sequence> <pathname><NULL>
//Response Format: <size><NULL><session> <sequence><NULL>
int filesystem::delete_entry(const char *username, vector<string>& args){
	//print_debug("Entered delete_entry: ", args[PATH]);

	//Find file or folder to delete
	filesystem::recurse_args recurse_fs_args;
	recurse_fs_args.disk_block = 0;
	bool parent_dir_found = recurse_filesystem(username, args[PATH], recurse_fs_args, 'd');
	//print_debug("Returned from recurse");
	//Path provided in fs_delete() is invalid
	if(!parent_dir_found){
		internal_lock.lock();
		pthread_rwlock_t* rwlock = &block_lock[recurse_fs_args.disk_block];
		internal_lock.unlock();
		//print_debug("368 unlock: ", recurse_fs_args.disk_block);
		pthread_rwlock_unlock(rwlock);
		delete recurse_fs_args.inode;
		return -1;
	}
	//Read in dir inode
	fs_direntry dir_to_delete = recurse_fs_args.folders[recurse_fs_args.folder_index];
	fs_inode inode_to_delete;
	//print_debug("Before internal lock line 372");

	internal_lock.lock();
	pthread_rwlock_t* wlock = &block_lock[dir_to_delete.inode_block];
	internal_lock.unlock();

	//print_debug("After internal lock line 376 block: ", dir_to_delete.inode_block);
	//print_debug("382 wrlock: ", dir_to_delete.inode_block);

	pthread_rwlock_wrlock(wlock);

	//print_debug("After pthread rwlock line 378");
	disk_readblock(dir_to_delete.inode_block, &inode_to_delete);
	//print_debug("After readblock line 380");

	//If inode is a non-empty directory or user is not owner, don't delete
	if((inode_to_delete.type == 'd' && inode_to_delete.size != 0) ||
		strcmp(inode_to_delete.owner, username)){
		//print_debug("Before internal lock line 383");
		internal_lock.lock();
		pthread_rwlock_t* rwlock2 = &block_lock[recurse_fs_args.disk_block];
		internal_lock.unlock();
		//print_debug("After internal lock line 388");
		//print_debug("397 unlock: ", dir_to_delete.inode_block);
		//print_debug("398 unlock: ", recurse_fs_args.disk_block);
		pthread_rwlock_unlock(wlock);
		pthread_rwlock_unlock(rwlock2);
		delete recurse_fs_args.inode;
		return -1;
	}

	deque<unsigned int> blocks_to_free;

	//if inode is a file, free all file blocks
	if(inode_to_delete.type == 'f'){
		for(unsigned int i = 0; i < inode_to_delete.size; ++i){
			blocks_to_free.push_back(inode_to_delete.blocks[i]);
		}
	}
	//free inode block
	blocks_to_free.push_back(dir_to_delete.inode_block);

	//remove direntry from directory
	fs_direntry empty_dir;
	empty_dir.inode_block = 0;
	memset(empty_dir.name, 0, FS_MAXFILENAME + 1);
	memcpy((void*)&(recurse_fs_args.folders[recurse_fs_args.folder_index]), (void*)&empty_dir, sizeof(fs_direntry));

	//check if directory is empty
	bool folder_block_empty = true;
	for(unsigned int i = 0; i < FS_DIRENTRIES; ++i){
		if (recurse_fs_args.folders[i].inode_block != 0){
			folder_block_empty = false;
			break;
		}
	}
	//print_debug("Before if statement 420");
	if(folder_block_empty){
		//if parent_inode size can be shrunk, update blocks
		--(recurse_fs_args.inode->size);
		blocks_to_free.push_back(recurse_fs_args.inode->blocks[recurse_fs_args.blocks_index]);
		for(unsigned int i = recurse_fs_args.blocks_index; i < recurse_fs_args.inode->size; ++i){
			recurse_fs_args.inode->blocks[i] = recurse_fs_args.inode->blocks[i + 1];
		}
		//write parent inode, to complete delete.
		disk_writeblock(recurse_fs_args.disk_block, recurse_fs_args.inode);
	}else{
		//write updated directory to disk
		disk_writeblock(recurse_fs_args.inode->blocks[recurse_fs_args.blocks_index], recurse_fs_args.folders);
	}
	//print_debug("Before internal lock 434");
	internal_lock.lock();
	free_blocks.insert(free_blocks.begin(), blocks_to_free.begin(), blocks_to_free.end());
	pthread_rwlock_t* rwlock2 = &block_lock[recurse_fs_args.disk_block];
	internal_lock.unlock();
	//print_debug("After internal lock 440");
	//print_debug("450 unlock: ", dir_to_delete.inode_block);
	//print_debug("451 unlock: ", recurse_fs_args.disk_block);
	pthread_rwlock_unlock(wlock);
	pthread_rwlock_unlock(rwlock2);
	//print_debug("After pthread lock 443");
	delete recurse_fs_args.inode;
	return 0;
}


// ----- FS_READBLOCK() RESPONSE -----//
//Request Format:  FS_READBLOCK <session> <sequence> <pathname> <block><NULL>
//Response Format: <size><NULL><session> <sequence><NULL><data>
// ----- FS_WRITEBLOCK() RESPONSE -----//
//Request Format:  FS_WRITEBLOCK <session> <sequence> <pathname> <block><NULL><data>
//Response Format: <size><NULL><session> <sequence><NULL>
int filesystem::access_entry(const char *username, vector<string>& args){
	char type = stoul(args[ACCESS_TYPE]) == READ ? 'r' : 'w';

	try {
		if(stoul(args[BLOCK]) >= FS_MAXFILEBLOCKS){
			return -1;
		}
	} catch(...){
		return -1;
	}
	//Recurse filesystem to make sure parent directory of file being read/written to exists
	filesystem::recurse_args recurse_fs_args;
	recurse_fs_args.disk_block = 0;
	bool parent_dir_found = recurse_filesystem(username, args[PATH], recurse_fs_args, type);

	if (!parent_dir_found){
		internal_lock.lock();
		pthread_rwlock_t* rwlock = &block_lock[recurse_fs_args.disk_block];
		internal_lock.unlock();
		pthread_rwlock_unlock(rwlock);
		delete recurse_fs_args.inode;
		return -1;
	}

	//Read in inode of file being read/written to
	fs_inode file;
	fs_direntry dir = recurse_fs_args.folders[recurse_fs_args.folder_index];
	internal_lock.lock();
	pthread_rwlock_t* dir_lock = &block_lock[dir.inode_block];
	internal_lock.unlock();
	if(type == 'r'){
		pthread_rwlock_rdlock(dir_lock);
	}else{
		pthread_rwlock_wrlock(dir_lock);
	}
	internal_lock.lock();
	pthread_rwlock_t* rwlock = &block_lock[recurse_fs_args.disk_block];
	internal_lock.unlock();
	pthread_rwlock_unlock(rwlock);

	disk_readblock(dir.inode_block, (void*)&file);

	if(file.type == 'd' || strcmp(file.owner, username)){
		pthread_rwlock_unlock(dir_lock);
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
			pthread_rwlock_unlock(dir_lock);
			delete recurse_fs_args.inode;
			return -1;
		}
	}
	//If fs_writeblock() request...
	else{
		//...and requested write block already valid in file...
		if (stoul(args[BLOCK]) < file.size){
			//...then write the write data to that block on disk
			//TODO: is calling .c_str() possibly wrong?
			disk_writeblock(file.blocks[stoul(args[BLOCK])], (void*)args[DATA].c_str());
		}
		//...and we must allocate a new block for write data...
		else if (stoul(args[BLOCK]) == file.size){
			internal_lock.lock();
			//Check to see if 1) there are any more free disk blocks and 2) this file can hold another block
			if(free_blocks.empty() || file.size == FS_MAXFILEBLOCKS){
				internal_lock.unlock();
				pthread_rwlock_unlock(dir_lock);
				delete recurse_fs_args.inode;
				return -1;
			}
			file.blocks[stoul(args[BLOCK])] = free_blocks.front();
			free_blocks.pop_front();
			internal_lock.unlock();
			file.size++;

			//...and finally write the write data, and also the new block in this file's inode's blocks[]
			//TODO: again, is calling .c_str() wrong?
			disk_writeblock(file.blocks[stoul(args[BLOCK])], (void*)args[DATA].c_str());	
			disk_writeblock(dir.inode_block, (void*)&file);		
		}
		else{
			pthread_rwlock_unlock(dir_lock);
			delete recurse_fs_args.inode;
			return -1;
		}
	}
	pthread_rwlock_unlock(dir_lock);
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
	internal_lock.lock();
	const char* cipher = (char*) fs_encrypt(
		users[username]->password(), (void*)raw_response,
		raw_response_size, &cipher_size);
	internal_lock.unlock();
	//Send response header and response
	string header(to_string(cipher_size));
	send(client, header.c_str(), header.length() + 1, 0);
	send(client, cipher, cipher_size, 0);
}


//Helper function, splits char * into segments delimited by 'token'
vector<string> filesystem::split_request(const string& request, char token, unsigned int& request_length) {

	vector<string> split;
	vector<string> empty;

	//If we are splitting a pathname, return empty vector if...
	if (token == '/'){
		//...the path does not begin with '/'.
		if (request.front() != '/')
			return split;

		//...the path ends with '/'.
		if (request.back() == '/')
			return split;

		//...the path contains any spaces.
		for (int i = 0; i < (int)request.size(); ++i){
			if (isspace(request[i]))
				return split;
		}

		//...the path contains "//"
		if (request.find("//") != string::npos)
			return split;
	}

	//print_debug("Request to be split: ", request);
	//print_debug("Request length: ", request.length());

	//Beginning and ending index of current entry
	//If we are splitting a path, ignore '/' at start of path
	int begin = (token == '/') ? 1 : 0;
	auto end = (token == '/') ? request.find(token, 1) : request.find(token);

	//If token does not appear in request (ignoring first '/' in a path)
	if (end == string::npos) {
		string only_entry = request.substr(begin, request.length());

		if (token == '/' && only_entry.size() > FS_MAXFILENAME)
			return empty;

		split.push_back(only_entry);
	}

	//Split request by the token
	while (end != string::npos) {
		string next_entry = request.substr(begin, end-begin);

		if (token == '/' && next_entry.size() > FS_MAXFILENAME)
			return empty;

		split.push_back(next_entry);

		begin = ++end;
		end = request.find(token, end);

		//If this is the last entry, push_back before loop exits
		if (end == string::npos) {
			string last_entry = request.substr(begin, request.length());
			if (token == '/' && last_entry.size() > FS_MAXFILENAME)
				return empty;

			request_length = request.size() + 1;
			////print_debug("Calculated request_length: ", request_length);
			split.push_back(last_entry);
		}
	}

	/*for (int i = 0; i < int(split.size()); ++i) {
		print_debug("Split request #", i, ": '", split[i], "'");
	}*/

	return split;
}


//Function to recursively search for valid parent directory of file/directory sent in request
bool filesystem::recurse_filesystem(const char *username, std::string& path, recurse_args &args, char req_type){
	args.inode = new fs_inode;

	//Split given path into individual directories
	unsigned int path_size = path.size();
	vector<string> split_path = split_request(path, '/', path_size);

	internal_lock.lock();
	pthread_rwlock_t* rwlock = &block_lock[args.disk_block];
	internal_lock.unlock();
	if(split_path.size() == 0){
		////print_debug("Splitpath size 0");
		//print_debug("685 rdlock: ", args.disk_block);
		pthread_rwlock_rdlock(rwlock);
		return false;
	}
	//update path to be the name of the file/dir at end of path
	path = split_path.back();

	if(split_path.size() == 1 && (req_type == 'c' || req_type == 'd')){
		//////print_debug("Getting writer lock in recurse fs ", req_type);
		//////print_debug("Writer lock is: ", &rwlock);
		//print_debug("695 wrlock: ", args.disk_block);
		pthread_rwlock_wrlock(rwlock);
	}else{
		//////print_debug("Getting readlock in recurse fs ", req_type);
		//print_debug("700 rdlock: ", args.disk_block);
		pthread_rwlock_rdlock(rwlock);
	}

	//Traverse filesystem to find the correct parent directory
	return recurse_filesystem_helper(username, split_path, 0, args, req_type);
}


//Helper for recurse_filesystem(), actually has recursive call to traverse given path, checking if it is valid
bool filesystem::recurse_filesystem_helper(const char* username, std::vector<std::string> &split_path, 
											unsigned int path_index, recurse_args &args, char req_type) {
	//read directory inode
	disk_readblock(args.disk_block, args.inode);

	//User trying to access directory they do not own
	//or trying to treat file as directory
	if (args.inode->type == 'f' || 
		(args.inode->owner[0] != '\0' && strcmp(args.inode->owner, username))) 
		return false;

	//Check to see if requested file/directory is in this directory
	bool found = search_directory(args, split_path[path_index].c_str());

	if (split_path.size()-1 == path_index){
		//If this is the last entry of path, return valid if:
		//(1) Create and Name/path of new directory/file doesn't exist
		//(2) Not create and Name of directory/file does exist
		if(!found ^ (req_type != 'c')){
			return true;
		}
		else{
			return false;
		}
	}

	//not at end of path, and next sub-dir exists
	if (found){
		//set next inode 
		unsigned int old_disk_block = args.disk_block;
		args.disk_block = args.folders[args.folder_index].inode_block;
		++path_index;

		//lock next inode: writer if at end of path and creating or deleteing
		// else reader 
		internal_lock.lock();
		pthread_rwlock_t* next_lock = &block_lock[args.disk_block], *last_lock = &block_lock[old_disk_block];
		internal_lock.unlock();
		if(split_path.size()-1 == path_index && (req_type == 'c' || req_type == 'd')){
			//print_debug("748 wrlock: ", args.disk_block);
			pthread_rwlock_wrlock(next_lock);
		}else{
			//print_debug("752 rdlock: ", args.disk_block);		
			pthread_rwlock_rdlock(next_lock);
		}

		//unlock last inode
		//print_debug("757 unlock: ", old_disk_block);
		pthread_rwlock_unlock(last_lock);
		return recurse_filesystem_helper(username, split_path, path_index, args, req_type);
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
			if (args.folders[j].inode_block == 0){
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