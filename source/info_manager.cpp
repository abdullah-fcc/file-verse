#include <iostream>
#include <fstream>
#include <vector>
#include <ctime>
#include "include/odf_types.hpp"

using namespace std;

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

// Compare two strings (char array vs string)
bool compare_name(const char* name_array, const string& name) {
    size_t i = 0;
    while (i < name.length() && name_array[i] != '\0') {
        if (name_array[i] != name[i]) {
            return false;
        }
        i++;
    }
    return (i == name.length() && name_array[i] == '\0');
}

// Load all entries from fixed slots
vector<FileEntry> load_all_entries() {
    vector<FileEntry> entries;
    
    ifstream file("../compiled/test.omni", ios::binary);
    if (!file) {
        return entries;
    }

    uint64_t start_pos = sizeof(OMNIHeader) + (100 * sizeof(UserInfo));
    file.seekg(start_pos, ios::beg);

    // Read through fixed slots (1000 max)
    for (int i = 0; i < 1000; i++) {
        FileEntry entry("", Entry_FILE, 0, 0, "", 0);
        
        if (!file.read((char*)&entry, sizeof(entry))) {
            break;
        }
        
        if (entry.name[0] != '\0') {
            entries.push_back(entry);
        }
    }

    file.close();
    return entries;
}

// Find entry by path
bool find_entry(const string& path, FileEntry& entry) {
    vector<FileEntry> entries = load_all_entries();
    
    for (size_t i = 0; i < entries.size(); i++) {
        if (compare_name(entries[i].name, path)) {
            entry = entries[i];
            return true;
        }
    }
    
    return false;
}

// ============================================================================
// GET_METADATA
// ============================================================================

int get_metadata(void* session, const string& path, FileMetadata& meta) {
    // Step 1: Validate inputs
    if (!session) {
        cerr << "Error: Invalid session" << endl;
        return ERROR_INVALID_OPERATION;
    }

    // Step 2: Find the file/directory
    FileEntry entry("", Entry_FILE, 0, 0, "", 0);
    if (!find_entry(path, entry)) {
        cerr << "Error: File not found: " << path << endl;
        return ERROR_NOT_FOUND;
    }

    // Step 3: Create metadata object
    meta = FileMetadata(path, entry);
    
    // Step 4: Calculate blocks used (assuming 4KB blocks)
    meta.blocks_used = (entry.size + 4095) / 4096;
    meta.actual_size = entry.size;

    cout << "SUCCESS: Metadata fetched for '" << path << "'" << endl;
    cout << "  Size: " << entry.size << " bytes" << endl;
    cout << "  Blocks: " << meta.blocks_used << endl;
    cout << "  Owner: " << entry.owner << endl;
    cout << "  Permissions: " << entry.permissions << endl;

    return SUCCESS;
}

// ============================================================================
// SET_PERMISSIONS
// ============================================================================

int set_permissions(void* session, const string& path, uint32_t permissions) {
    // Step 1: Validate inputs
    if (!session) {
        cerr << "Error: Invalid session" << endl;
        return ERROR_INVALID_OPERATION;
    }

    // Step 2: Open file
    fstream file("../compiled/test.omni", ios::in | ios::out | ios::binary);
    if (!file) {
        cerr << "Error: Cannot open file system" << endl;
        return ERROR_IO_ERROR;
    }

    // Step 3: Search through slots
    uint64_t start_pos = sizeof(OMNIHeader) + (100 * sizeof(UserInfo));

    for (int i = 0; i < 1000; i++) {
        uint64_t current_pos = start_pos + (i * sizeof(FileEntry));
        file.seekg(current_pos, ios::beg);
        
        FileEntry entry("", Entry_FILE, 0, 0, "", 0);
        if (!file.read((char*)&entry, sizeof(entry))) {
            break;
        }

        // Step 4: Check if this is the entry we want
        if (compare_name(entry.name, path)) {
            // Update permissions
            entry.permissions = permissions;
            
            // Write back
            file.seekp(current_pos, ios::beg);
            file.write((char*)&entry, sizeof(entry));
            file.flush();
            file.close();
            
            cout << "SUCCESS: Permissions updated for '" << path << "'" << endl;
            cout << "  New permissions: " << permissions << endl;
            return SUCCESS;
        }
    }

    file.close();
    cerr << "Error: File not found: " << path << endl;
    return ERROR_NOT_FOUND;
}

// ============================================================================
// GET_STATS
// ============================================================================

int get_stats(void* session, FSStats& stats) {
    // Step 1: Validate session
    if (!session) {
        cerr << "Error: Invalid session" << endl;
        return ERROR_INVALID_OPERATION;
    }

    // Step 2: Load all entries
    vector<FileEntry> entries = load_all_entries();

    // Step 3: Count files and directories
    uint64_t total_files = 0;
    uint64_t total_dirs = 0;
    uint64_t used_space = 0;

    for (size_t i = 0; i < entries.size(); i++) {
        if (entries[i].type == 0) {  // File
            total_files++;
            used_space += entries[i].size;
        } else if (entries[i].type == 1) {  // Directory
            total_dirs++;
        }
    }

    // Step 4: Load header to get total size
    ifstream file("../compiled/test.omni", ios::binary);
    if (!file) {
        cerr << "Error: Cannot open file system" << endl;
        return ERROR_IO_ERROR;
    }

    OMNIHeader header(0, 0, 0, 0);
    file.read((char*)&header, sizeof(header));
    file.close();

    // Step 5: Calculate free space
    uint64_t free_space = 0;
    if (header.total_size > used_space) {
        free_space = header.total_size - used_space;
    }

    // Step 6: Fill stats structure
    stats = FSStats(header.total_size, used_space, free_space);
    stats.total_files = total_files;
    stats.total_directories = total_dirs;
    stats.fragmentation = 0.0;  // Can implement later

    cout << "SUCCESS: File system statistics computed" << endl;
    cout << "  Total files: " << total_files << endl;
    cout << "  Total directories: " << total_dirs << endl;
    cout << "  Used space: " << used_space << " bytes" << endl;
    cout << "  Free space: " << free_space << " bytes" << endl;

    return SUCCESS;
}

// ============================================================================
// GET_ERROR_MESSAGE
// ============================================================================

string get_error_message(int error_code) {
    switch (error_code) {
        case SUCCESS:
            return "Operation successful";
        case ERROR_NOT_FOUND:
            return "Item not found";
        case ERROR_PERMISSION_DENIED:
            return "Permission denied";
        case ERROR_IO_ERROR:
            return "I/O operation failed";
        case ERROR_INVALID_PATH:
            return "Invalid path";
        case ERROR_FILE_EXISTS:
            return "File or directory already exists";
        case ERROR_NO_SPACE:
            return "No space left in file system";
        case ERROR_INVALID_CONFIG:
            return "Invalid configuration";
        case ERROR_NOT_IMPLEMENTED:
            return "Feature not implemented";
        case ERROR_INVALID_SESSION:
            return "Invalid or expired session";
        case ERROR_DIRECTORY_NOT_EMPTY:
            return "Directory not empty";
        case ERROR_INVALID_OPERATION:
            return "Invalid operation";
        default:
            return "Unknown error code";
    }
}

// ============================================================================
// MAIN TEST
// ============================================================================

int main() {
    cout << "\n========================================" << endl;
    cout << "  INFORMATION FUNCTIONS TEST" << endl;
    cout << "========================================\n" << endl;

    // Create test session
    UserInfo user("abdullah", "HASH_1234", ADMIN, time(nullptr));
    SessionInfo session("SID_001", user, time(nullptr));

    // Test 1: Get metadata for a file
    cout << "Test 1: Getting file metadata..." << endl;
    FileMetadata meta("", FileEntry("", Entry_FILE, 0, 0, "", 0));
    int result = get_metadata(&session, "hello.txt", meta);
    
    if (result != SUCCESS) {
        cout << "  Note: File 'hello.txt' not found (create it first)" << endl;
    }

    // Test 2: Set permissions
    cout << "\nTest 2: Setting permissions..." << endl;
    result = set_permissions(&session, "hello.txt", 0600);
    
    if (result != SUCCESS) {
        cout << "  Note: File 'hello.txt' not found" << endl;
    }

    // Test 3: Get file system statistics
    cout << "\nTest 3: Getting file system stats..." << endl;
    FSStats stats(0, 0, 0);
    result = get_stats(&session, stats);

    if (result == SUCCESS) {
        cout << "\n=== File System Statistics ===" << endl;
        cout << "Total Size:  " << stats.total_size << " bytes" << endl;
        cout << "Used Space:  " << stats.used_space << " bytes" << endl;
        cout << "Free Space:  " << stats.free_space << " bytes" << endl;
        cout << "Files:       " << stats.total_files << endl;
        cout << "Directories: " << stats.total_directories << endl;
        cout << "===============================" << endl;
    }

    // Test 4: Get error messages
    cout << "\nTest 4: Testing error messages..." << endl;
    cout << "  SUCCESS: " << get_error_message(SUCCESS) << endl;
    cout << "  ERROR_NOT_FOUND: " << get_error_message(ERROR_NOT_FOUND) << endl;
    cout << "  ERROR_PERMISSION_DENIED: " << get_error_message(ERROR_PERMISSION_DENIED) << endl;

    cout << "\n========================================" << endl;
    cout << "  ✓ ALL TESTS COMPLETED!" << endl;
    cout << "========================================\n" << endl;

    return SUCCESS;
}