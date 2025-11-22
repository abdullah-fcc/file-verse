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
    
    cout << "[find_directory] Searching for: '" << path << "'" << endl;
    
    for (size_t i = 0; i < entries.size(); i++) {
        string entry_name(entries[i].name);
        cout << "[find_directory] Checking: '" << entry_name << "' (type: " << (int)entries[i].type << ")" << endl;
        
        if (entries[i].type == DIRECTORY && compare_name(entries[i].name, path)) {
            entry = entries[i];
            cout << "[find_directory] FOUND!" << endl;
            return true;
        }
    }
    
    cout << "[find_directory] NOT FOUND" << endl;
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
    children.clear();

    cout << "[dir_list] Listing path: '" << path << "'" << endl;
    cout << "[dir_list] Total entries in system: " << all_entries.size() << endl;

    // Handle root directory (empty path or "/")
    if (path.empty() || path == "/") {
        cout << "[dir_list] Listing ROOT directory" << endl;
        
        for (size_t i = 0; i < all_entries.size(); i++) {
            string entry_name(all_entries[i].name);
            
            // Skip empty entries
            if (entry_name.empty()) continue;
            
            // Check if this is a root-level entry (no '/' in the name)
            bool is_root_entry = (entry_name.find('/') == string::npos);
            
            if (is_root_entry) {
                cout << "[dir_list] Found root entry: '" << entry_name << "'" << endl;
                children.push_back(all_entries[i]);
            }
        }
        
        if (children.size() == 0) {
            cout << "Root directory is empty" << endl;
            return SUCCESS;  // Return SUCCESS for empty root, not ERROR_NOT_FOUND
        }
        
        cout << "Root directory contains " << children.size() << " items" << endl;
        return SUCCESS;
    }

    // Handle subdirectories
    string prefix = path;
    if (prefix.back() != '/') {
        prefix += '/';
    }

    cout << "[dir_list] Looking for prefix: '" << prefix << "'" << endl;
    
    for (size_t i = 0; i < all_entries.size(); i++) {
        string entry_name(all_entries[i].name);
        
        if (entry_name.find(prefix) == 0) {
            string sub = entry_name.substr(prefix.length());
            
            // Check if this is a direct child (no more '/' in the remaining path)
            bool is_direct_child = true;
            for (size_t j = 0; j < sub.length(); j++) {
                if (sub[j] == '/') {
                    is_direct_child = false;
                    break;
                }
            }
            
            if (is_direct_child && !sub.empty()) {
                cout << "[dir_list] Found child: '" << entry_name << "'" << endl;
                children.push_back(all_entries[i]);
            }
        }
    }

    if (children.size() == 0) {
        cout << "Directory '" << path << "' is empty" << endl;
        return SUCCESS;  // Changed from ERROR_NOT_FOUND to SUCCESS
    }

    cout << "Directory '" << path << "' contains " << children.size() << " items:" << endl;
    for (size_t i = 0; i < children.size(); i++) {
        cout << "  " << (i + 1) << ". " << children[i].name;
        if (children[i].type == DIRECTORY) {
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

    cout << "[dir_delete] Attempting to delete: '" << path << "'" << endl;

    FileEntry dir_entry("", DIRECTORY, 0, 0, "", 0);
    if (!find_directory(path, dir_entry)) {
        cerr << "Error: Directory not found: " << path << endl;
        return ERROR_NOT_FOUND;
    }

    cout << "[dir_delete] Directory found, checking if empty..." << endl;

    // Check if directory has children
    vector<FileEntry> children;
    int result = dir_list(session, path, children);
    
    // If dir_list returns SUCCESS and has children, directory is not empty
    if (result == SUCCESS && children.size() > 0) {
        cerr << "Error: Directory not empty: " << path << " (contains " << children.size() << " items)" << endl;
        return ERROR_DIRECTORY_NOT_EMPTY;
    }

    cout << "[dir_delete] Directory is empty, proceeding with deletion..." << endl;

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
        
        string entry_name(entry.name);
        cout << "[dir_delete] Slot " << i << ": '" << entry_name << "' (type: " << (int)entry.type << ")" << endl;
        
        if (entry.type == DIRECTORY && compare_name(entry.name, path)) {
            cout << "[dir_delete] MATCH FOUND at slot " << i << endl;
            
            // Clear the entry by setting name to null
            entry.name[0] = '\0';
            entry.type = 0;
            entry.size = 0;
            
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
    dir_create(&session, "docs");
    dir_create(&session, "docs/projects");
    dir_create(&session, "images");

    // Test 2: Check if directory exists
    cout << "\nTest 2: Checking if directory exists..." << endl;
    dir_exists(&session, "docs");
    dir_exists(&session, "nonexistent");

    // Test 3: List directory contents
    cout << "\nTest 3: Listing directory contents..." << endl;
    vector<FileEntry> children;
    dir_list(&session, "docs", children);

    // Test 4: Try to delete non-empty directory
    cout << "\nTest 4: Trying to delete non-empty directory..." << endl;
    dir_delete(&session, "docs");

    // Test 5: Delete empty directory
    cout << "\nTest 5: Deleting empty directory..." << endl;
    dir_delete(&session, "images");

    // Test 6: Verify deletion
    cout << "\nTest 6: Verifying deletion..." << endl;
    dir_exists(&session, "images");

    cout << "\n========================================" << endl;
    cout << "  ✓ ALL TESTS COMPLETED!" << endl;
    cout << "========================================\n" << endl;

    return SUCCESS;
}