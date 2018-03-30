#include "filesystem.h"
#include <cassert>
#include <cstring>
#include <string>
#include <sstream>
#include <sys/socket.h>
#include <iostream>

using namespace std;

// ----- For explicitly indexing into vector with request args ----- //
#define REQUEST_NAME 0
#define SESSION 1
#define SEQUENCE 2
#define PATH 3
#define BLOCK 4
#define TYPE 4


//Internal entry constructor
filesystem::entry::entry(unsigned int inode_block_in, unsigned int parent_blocks_index_in) 
: inode_block(inode_block_in), parent_blocks_index(parent_blocks_index_in) {}


//Filesystem constructor
filesystem::filesystem() {
	//Init root directory
	disk_blocks[0] = 1;
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


// ----- FS_SESSION() RESPONSE ----- //
//Request format:  FS_SESSION <session> <sequence><NULL>
//Response format: <size><NULL><session> <sequence><NULL>
void filesystem::session_response(int client, const char *username, char *request,
							unsigned int request_size){
	cout << "building session response..." << endl;
	vector<char *> request_items = split_request(request, " ");
	if(strcmp(request_items[REQUEST_NAME], "FS_SESSION"))
		return;
	cout << "request_name: " << request_items[REQUEST_NAME] << endl;	

	//Given session # is meaningless, create new session ID with given sequence #
	unsigned int sequence = stoi(request_items[SEQUENCE]);
	unsigned int session = users[username]->create_session(sequence);

	//Send response back to client on success
	ostringstream response;
	response << to_string(session) << " " << to_string(sequence);
	send_response(client, username, response.str());
	return;
}


// ----- FS_READBLOCK() RESPONSE -----//
//Request Format:  FS_READBLOCK <session> <sequence> <pathname> <block><NULL>
//Response Format: <size><NULL><session> <sequence><NULL><data>
void filesystem::readblock_response(int client, const char *username, char *request,
							unsigned int request_size){
	assert(false);
}


// ----- FS_WRITEBLOCK() RESPONSE -----//
//Request Format:  FS_WRITEBLOCK <session> <sequence> <pathname> <block><NULL><data>
//Response Format: <size><NULL><session> <sequence><NULL>
void filesystem::writeblock_response(int client, const char *username, char *request,
							unsigned int request_size){
	assert(false);
}


// ----- FS_CREATE() RESPONSE -----//
//Request Format:  FS_CREATE <session> <sequence> <pathname> <type><NULL>
//Response Format: <size><NULL><session> <sequence><NULL>
void filesystem::create_response(int client, const char *username, char *request,
							unsigned int request_size){
	vector<char *> request_items = split_request(request, " ");
	if(strcmp(request_items[REQUEST_NAME], "FS_CREATE"))
		return;
	cout << "request_name: " << request_items[REQUEST_NAME] << endl;

	//Check validity of session/sequence
	if (!users[username]->session_request(stoi(request_items[SESSION]), stoi(request_items[SEQUENCE])))
		return;
	cout << "sequence valid" << endl;

	//Create internal/external entry and inode for new directory/file
	if (create_entry(username, request_items[PATH], request_items[TYPE]) == -1)
		return;

	//Send response back to client on success
	cout << "entry created, sending response..." << endl;
	ostringstream response;
	response << request_items[SESSION] << " " << request_items[SEQUENCE];
	send_response(client, username, response.str());
}


// ----- FS_DELETE() RESPONSE -----//
//Request Format:  FS_DELETE <session> <sequence> <pathname><NULL>
//Response Format: <size><NULL><session> <sequence><NULL>
void filesystem::delete_response(int client, const char *username, char *request,
							unsigned int request_size){
	assert(false);
}


//Helper function, sends responses back to client after successfully handling request
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


//Helper function, splits char * into segments delimited by 'token'
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


//Helper function, for create_response(), check path validity and 
//create new internal and external filesystem entry/inode
int filesystem::create_entry(const char* username, char *path, char* type) {

	//Split given path into individual directories
	vector<char *> split_path = split_request(path, "/");
	cout << "path split" << endl;

	//Find smallest free disk block for new entry's inode, mark it as used
	unsigned int new_entry_inode_block = next_free_disk_block();
	if (new_entry_inode_block == FS_DISKSIZE)
		return -1;
	cout << "have enough space for new entry" << endl;
	disk_blocks[new_entry_inode_block] = 1;

	//Read in inode of root directory from disk
	fs_inode* inode = new fs_inode;
	disk_readblock(0, inode);
	cout << split_path[0] << endl;

	//If parent directory of new directory/file is not the '/' directory,
	//traverse filesystem to find the correct parent directory
	entry* parent_entry = &root;
	if (root.entries.find(split_path[0]) != root.entries.end()) {
		cout << "recurse_filesystem to find parent_entry" << endl;
		parent_entry = recurse_filesystem(username, split_path, 0, root.entries[split_path[1]], inode, 'c');
	} else {
		cout << "path is in root" << endl;
	}
	
	//Path provided in fs_create() is invalid
	if(parent_entry == nullptr){
		cout << "path invalid" << endl;
		delete inode;
		return -1;
	}

	cout << "parent_entry found, path valid" << endl;
	cout << "parent_entry->inode_block: " << parent_entry->inode_block << endl;

	//parent_directory[] represents the direntries in the block at the highest index in parent directory's blocks[]
	fs_direntry parent_directory[FS_DIRENTRIES];
	
	//If new direntry for this new directory/file will require another data block in parent directory's inode...
	if (inode->size * FS_DIRENTRIES == parent_entry->entries.size()) {
		cout << "allocate disk block in parent_entry" << endl;

		//...find the smallest free disk block for parent directory's inode to use...
		unsigned int next_disk_block = next_free_disk_block();

		//...and make sure 1) a free block exists on disk and 2) parent directory's inode can hold another block
		if (next_disk_block == FS_DISKSIZE && inode->size < FS_MAXFILEBLOCKS) {
			disk_blocks[new_entry_inode_block] = 0;
			delete inode;
			return -1;
		}
		cout << "have enough space for new parent_entry block" << endl;

		//If check above passes, mark smallest free disk block as used, now available for parent directory's inode
		disk_blocks[next_disk_block] = 1;

		//Add new data block to inode, increment size of inode (# blocks in inode)
		inode->blocks[inode->size] = next_disk_block;
		inode->size += 1;

		//Initialize array of direntries at this new block in parent directory's blocks[] to be empty
		memset(parent_directory, 0, FS_BLOCKSIZE);
	}
	//Else new direntry can be placed in a pre-existing data block in inode
	else{
		//Read in direntries of block at highest index in parent directory's blocks[]
		//(parent_directory[] represents the direntries in the block at the highest index in parent directory's blocks[]))
		disk_readblock(inode->blocks[inode->size - 1], parent_directory);
	}

	//Create new internal entry for directory/file being created
	parent_entry->entries[split_path.back()] = new entry(new_entry_inode_block, parent_entry->entries.size());

	//Create new external direntry for directory/file being created
	fs_direntry new_direntry;
	new_direntry.inode_block = new_entry_inode_block;
	strcpy(new_direntry.name, split_path.back());
	cout << "sizeof(new_direntry): " << sizeof(new_direntry) << endl;
	cout << "parent_blocks_index: " << parent_entry->entries[split_path.back()]->parent_blocks_index << endl;
	cout << "sizeof(fs_direntry): " << sizeof(fs_direntry) << endl;
	cout << "inode->blocks: " << inode->blocks << endl;

	//Place new_direntry in next unoccupied index 
	//(parent_directory[] represents the direntries in the block at the highest index in parent directory's blocks[])
	parent_directory[parent_entry->entries[split_path.back()]->parent_blocks_index] = new_direntry;
	cout << "new_direntry copied into parent_entry" << endl;

	//Create new inode for directory/file being created
	fs_inode new_inode;
	new_inode.type = type[0];
	strcpy(new_inode.owner, username);
	new_inode.size = 0;
	cout << "new entry created" << endl;

	//Update the blocks[] array of the parent directory of the new directory/file on disk 
	//to now include the direntry/data block # of new directory/file
	disk_writeblock(inode->blocks[inode->size - 1], parent_directory);

	//Write inode of parent directory to disk
	disk_writeblock(parent_entry->inode_block, inode);

	//Write inode of new directory/file to reserved disk block
	disk_writeblock(new_entry_inode_block, &new_inode);

	cout << "entry written to disk" << endl;

	//Delete new inode object created above
	delete inode;
	return 0;
}


//FIX THIS!!!!!!!!!!!
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


//Helper function, find lowest free disk block. DOES NOT RESERVE BLOCK, just returns it's #
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

