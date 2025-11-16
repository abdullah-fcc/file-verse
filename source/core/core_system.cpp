#include <iostream>
#include <fstream>
#include <vector>
#include "helper.hpp"
#include "../include/odf_types.hpp"
#include "../include/ofs_functions.hpp"
using namespace std;

struct FileSystem {
    OMNIHeader header;
    vector<UserInfo> users;
    vector<SessionInfo> sessions;
    string omni_path;

    FileSystem() : header(0, 0, 0, 0) {

    }
};




int fs_init(void** instance, const char* omni_path, const char* config_path) {
  
    if (!instance || !omni_path) {
        cerr << "Error: Invalid parameters" << endl;
        return ERROR_INVALID_CONFIG;
    }

    ifstream file(omni_path, ios::binary);
    if (!file) {
        cerr << "Error: Cannot open " << omni_path << endl;
        return ERROR_NOT_FOUND;
    }

    FileSystem* fs = new FileSystem();

    file.read((char*)(&fs->header), sizeof(fs->header));
    if (!compare(fs->header.magic, "OMNIFS01", 8)) {
        cerr << "Error: Invalid OMNI file format" << endl;
        delete fs;
        file.close();
        return ERROR_INVALID_CONFIG;
    }
    file.seekg(fs->header.user_table_offset, ios::beg);

    for (uint32_t i = 0; i < fs->header.max_users; i++) {
        UserInfo user("", "", NORMAL, 0);
        file.read((char*)(&user), sizeof(user));

        if (!file) 
            break;
        if (user.is_active == 1 && user.username[0] != '\0') {
            fs->users.push_back(user);
        }
    }

    fs->omni_path = omni_path;

    file.close();

    cout << "SUCCESS: Loaded OMNI file system" << endl;
    cout << "  File: " << omni_path << endl;
    cout << "  Users loaded: " << fs->users.size() << endl;
    cout << "  Max users: " << fs->header.max_users << endl;

    *instance = fs;

    return SUCCESS;
}

void fs_shutdown(void* instance) {
    if (!instance) {
        cerr << "Error: Invalid instance pointer" << endl;
        return;
    }

    FileSystem* fs = (FileSystem*)instance;
    fstream file(fs->omni_path, ios::in | ios::out | ios::binary);
    if (!file) {
        cerr << "Error: Cannot open file for saving" << endl;
        delete fs;
        return;
    }

    file.seekp(fs->header.user_table_offset, ios::beg);

    for (size_t i = 0; i < fs->users.size(); i++) {
        if (i >= fs->header.max_users) {
            break;  
        }
        file.write((const char*)(&fs->users[i]), sizeof(UserInfo));
    }

    file.close();
    cout << "SUCCESS: File system saved and closed" << endl;
    cout << "  Users saved: " << fs->users.size() << endl;

    delete fs;
}

int main_test_coresystem() {
    string omni_file = "../compiled/test.omni";
    void* fs_instance = nullptr;

    cout << "\n========================================" << endl;
    cout << "  FILE SYSTEM INIT/SHUTDOWN TEST" << endl;
    cout << "========================================\n" << endl;

    // Test 1: Initialize file system
    cout << "Test 1: Initializing file system..." << endl;
    int result = fs_init(&fs_instance, omni_file.c_str(), nullptr);
    
    if (result != SUCCESS) {
        cerr << "FAILED: Could not initialize file system" << endl;
        return result;
    }

    // Test 2: Access file system data
    cout << "\nTest 2: Accessing file system data..." << endl;
    FileSystem* fs = (FileSystem*)fs_instance;
    
    cout << "  Magic: " << fs->header.magic << endl;
    cout << "  Total size: " << fs->header.total_size << " bytes" << endl;
    cout << "  Block size: " << fs->header.block_size << " bytes" << endl;

    // Test 3: List loaded users
    cout << "\nTest 3: Listing loaded users..." << endl;
    for (size_t i = 0; i < fs->users.size(); i++) {
        cout << "  " << (i + 1) << ". " << fs->users[i].username;
        cout << " (Role: " << (fs->users[i].role == ADMIN ? "Admin" : "Normal") << ")";
        cout << endl;
    }

    // Test 4: Shutdown file system
    cout << "\nTest 4: Shutting down file system..." << endl;
    fs_shutdown(fs_instance);

    cout << "\n========================================" << endl;
    cout << "  ✓ ALL TESTS PASSED!" << endl;
    cout << "========================================\n" << endl;

    return SUCCESS;
}