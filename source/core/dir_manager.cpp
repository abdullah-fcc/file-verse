#include <iostream>
#include <fstream>
#include <vector>
#include <ctime>
#include "helper.hpp"
#include "../include/odf_types.hpp"
#include "../include/ofs_functions.hpp"
using namespace std;


bool find_empty_slot(uint64_t& position) {
    ifstream file("../compiled/test.omni", ios::binary);
    if (!file) {
        return false;
    }
    
    uint64_t start_pos = sizeof(OMNIHeader) + (100 * sizeof(UserInfo));
    FileEntry entry("", Entry_FILE, 0, 0, "", 0);
    
    for (int i = 0; i < 1000; i++) {
        uint64_t current_pos = start_pos + (i * sizeof(FileEntry));
        file.seekg(current_pos, ios::beg);
        
        if (!file.read((char*)&entry, sizeof(entry))) {

            position = current_pos;
            file.close();
            return true;
        }
        
        if (entry.name[0] == '\0') {
            position = current_pos;
            file.close();
            return true;
        }
    }
    
    file.close();
    return false; 
}

bool find_directory(const string& path, FileEntry& entry) {
    vector<FileEntry> entries = load_all_entries();
    
    for (size_t i = 0; i < entries.size(); i++) {
        if (entries[i].type == 1 && compare_name(entries[i].name, path)) {
            entry = entries[i];
            return true;
        }
    }
    
    return false;
}


int dir_exists(void* session, const string& path) {
    if (!session) {
        cerr << "Error: Invalid session" << endl;
        return ERROR_INVALID_OPERATION;
    }

    FileEntry entry("", DIRECTORY, 0, 0, "", 0);
    if (find_directory(path, entry)) {
        cout << "Directory '" << path << "' exists" << endl;
        return SUCCESS;
    }

    cout << "Directory '" << path << "' does not exist" << endl;
    return ERROR_NOT_FOUND;
}


int dir_create(void* session, const string& path) {
    if (!session) {
        cerr << "Error: Invalid session" << endl;
        return ERROR_INVALID_OPERATION;
    }

    FileEntry existing("", DIRECTORY, 0, 0, "", 0);
    if (find_directory(path, existing)) {
        cerr << "Error: Directory already exists: " << path << endl;
        return ERROR_FILE_EXISTS;
    }

    uint64_t slot_pos;
    if (!find_empty_slot(slot_pos)) {
        cerr << "Error: No space for new directories" << endl;
        return ERROR_NO_SPACE;
    }

    SessionInfo* s = (SessionInfo*)session;

    FileEntry dir(path, DIRECTORY, 0, 0755, string(s->user.username), rand() % 10000);
    dir.created_time = time(nullptr);
    dir.modified_time = time(nullptr);

    fstream file("../compiled/test.omni", ios::in | ios::out | ios::binary);
    if (!file) {
        cerr << "Error: Cannot open file system" << endl;
        return ERROR_IO_ERROR;
    }

    file.seekp(slot_pos, ios::beg);
    file.write((char*)&dir, sizeof(dir));
    file.flush();
    file.close();

    cout << "SUCCESS: Created directory '" << path << "'" << endl;
    return SUCCESS;
}


int dir_list(void* session, const string& path, vector<FileEntry>& children) {
    if (!session) {
        cerr << "Error: Invalid session" << endl;
        return ERROR_INVALID_OPERATION;
    }

    vector<FileEntry> all_entries = load_all_entries();

    string prefix = path;
    if (prefix.back() != '/') {
        prefix += '/';
    }

    children.clear();
    
    for (size_t i = 0; i < all_entries.size(); i++) {
        string entry_name(all_entries[i].name);
        
        if (entry_name.find(prefix) == 0) {
            string sub = entry_name.substr(prefix.length());
            
            bool is_direct_child = true;
            for (size_t j = 0; j < sub.length(); j++) {
                if (sub[j] == '/') {
                    is_direct_child = false;
                    break;
                }
            }
            
            if (is_direct_child) {
                children.push_back(all_entries[i]);
            }
        }
    }

    if (children.size() == 0) {
        cout << "Directory '" << path << "' is empty" << endl;
        return ERROR_NOT_FOUND;
    }

    cout << "Directory '" << path << "' contains " << children.size() << " items:" << endl;
    for (size_t i = 0; i < children.size(); i++) {
        cout << "  " << (i + 1) << ". " << children[i].name;
        if (children[i].type == 1) {
            cout << " (directory)";
        } else {
            cout << " (" << children[i].size << " bytes)";
        }
        cout << endl;
    }

    return SUCCESS;
}


int dir_delete(void* session, const string& path) {
    if (!session) {
        cerr << "Error: Invalid session" << endl;
        return ERROR_INVALID_OPERATION;
    }

    FileEntry dir_entry("", DIRECTORY, 0, 0, "", 0);
    if (!find_directory(path, dir_entry)) {
        cerr << "Error: Directory not found: " << path << endl;
        return ERROR_NOT_FOUND;
    }

    vector<FileEntry> children;
    int result = dir_list(session, path, children);
    
    if (result == SUCCESS && children.size() > 0) {
        cerr << "Error: Directory not empty: " << path << endl;
        return ERROR_DIRECTORY_NOT_EMPTY;
    }

    fstream file("../compiled/test.omni", ios::in | ios::out | ios::binary);
    if (!file) {
        cerr << "Error: Cannot open file system" << endl;
        return ERROR_IO_ERROR;
    }

    uint64_t start_pos = sizeof(OMNIHeader) + (100 * sizeof(UserInfo));

    for (int i = 0; i < 1000; i++) {
        uint64_t current_pos = start_pos + (i * sizeof(FileEntry));
        file.seekg(current_pos, ios::beg);
        
        FileEntry entry("", DIRECTORY, 0, 0, "", 0);
        if (!file.read((char*)&entry, sizeof(entry))) {
            break;
        }
        
        if (entry.type == 1 && compare_name(entry.name, path)) {
            entry.name[0] = '\0';
            
            file.seekp(current_pos, ios::beg);
            file.write((char*)&entry, sizeof(entry));
            file.flush();
            file.close();
            
            cout << "SUCCESS: Deleted directory '" << path << "'" << endl;
            return SUCCESS;
        }
    }

    file.close();
    cerr << "Error: Directory not found during deletion" << endl;
    return ERROR_NOT_FOUND;
}


int test_dir_manager() {
    cout << "\n========================================" << endl;
    cout << "  DIRECTORY OPERATIONS TEST" << endl;
    cout << "========================================\n" << endl;

    // Create test session
    UserInfo user("abdullah", "HASH_1234", ADMIN, time(nullptr));
    SessionInfo session("SID_001", user, time(nullptr));

    // Test 1: Create directories
    cout << "Test 1: Creating directories..." << endl;
    dir_create(&session, "/docs");
    dir_create(&session, "/docs/projects");
    dir_create(&session, "/images");

    // Test 2: Check if directory exists
    cout << "\nTest 2: Checking if directory exists..." << endl;
    dir_exists(&session, "/docs");
    dir_exists(&session, "/nonexistent");

    // Test 3: List directory contents
    cout << "\nTest 3: Listing directory contents..." << endl;
    vector<FileEntry> children;
    dir_list(&session, "/docs", children);

    // Test 4: Try to delete non-empty directory
    cout << "\nTest 4: Trying to delete non-empty directory..." << endl;
    dir_delete(&session, "/docs");

    // Test 5: Delete empty directory
    cout << "\nTest 5: Deleting empty directory..." << endl;
    dir_delete(&session, "/images");

    // Test 6: Verify deletion
    cout << "\nTest 6: Verifying deletion..." << endl;
    dir_exists(&session, "/images");

    cout << "\n========================================" << endl;
    cout << "  ✓ ALL TESTS COMPLETED!" << endl;
    cout << "========================================\n" << endl;

    return SUCCESS;
}