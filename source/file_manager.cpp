#include <iostream>
#include <fstream>
#include <vector>
#include <ctime>
#include "include/odf_types.hpp"

using namespace std;

bool compare(const char* name_array, const string& name) {
    size_t i = 0;
    while (i < name.length() && name_array[i] != '\0') {
        if (name_array[i] != name[i]) {
            return false;
        }
        i++;
    }
    return (i == name.length() && name_array[i] == '\0');
}

void copy(char* dest, size_t dest_size, const string& src) {
    size_t len = src.length();
    if (len >= dest_size) {
        len = dest_size - 1;
    }
    
    for (size_t i = 0; i < len; i++) {
        dest[i] = src[i];
    }
    dest[len] = '\0';
}

bool find_file(ifstream& file, const string& path, FileEntry& entry, uint64_t& position) {
    uint64_t file_table_start = sizeof(OMNIHeader) + (100 * sizeof(UserInfo));
    
    file.clear();
    file.seekg(file_table_start, ios::beg);
    
    for (int i = 0; i < 1000; i++) {
        uint64_t current_pos = file_table_start + (i * sizeof(FileEntry));
        file.seekg(current_pos, ios::beg);
        
        if (!file.read((char*)&entry, sizeof(entry))) {
            break;
        }
        
        if (entry.name[0] != '\0' && compare(entry.name, path) && entry.type == 0) {
            position = current_pos;
            return true;
        }
    }
    
    return false;
}

bool find_empty_slot(ifstream& file, uint64_t& position) {
    uint64_t file_table_start = sizeof(OMNIHeader) + (100 * sizeof(UserInfo));
    FileEntry entry("", Entry_FILE, 0, 0, "", 0);
    
    file.clear();
    
    for (int i = 0; i < 1000; i++) {
        uint64_t current_pos = file_table_start + (i * sizeof(FileEntry));
        file.seekg(current_pos, ios::beg);
        
        if (!file.read((char*)&entry, sizeof(entry))) {
            position = current_pos;
            return true;
        }
        
        if (entry.name[0] == '\0') {
            position = current_pos;
            return true;
        }
    }
    
    return false; 
}

int file_create(void* session, const string& path, const string& data) {
    if (!session) {
        cerr << "Error: Invalid session" << endl;
        return ERROR_INVALID_OPERATION;
    }

    SessionInfo* s = (SessionInfo*)session;

    ifstream check_file("../compiled/test.omni", ios::binary);
    if (!check_file) {
        cerr << "Error: Cannot open file system" << endl;
        return ERROR_IO_ERROR;
    }

    FileEntry existing("", Entry_FILE, 0, 0, "", 0);
    uint64_t pos;
    if (find_file(check_file, path, existing, pos)) {
        cerr << "Error: File already exists: " << path << endl;
        check_file.close();
        return ERROR_FILE_EXISTS;
    }
    
    uint64_t slot_pos;
    if (!find_empty_slot(check_file, slot_pos)) {
        cerr << "Error: No space for new files" << endl;
        check_file.close();
        return ERROR_NO_SPACE;
    }
    check_file.close();

    FileEntry entry(path, Entry_FILE, data.size(), 0644, string(s->user.username), rand() % 10000);
    entry.created_time = time(nullptr);
    entry.modified_time = time(nullptr);

    fstream file("../compiled/test.omni", ios::in | ios::out | ios::binary);
    if (!file) {
        cerr << "Error: Cannot open file system for writing" << endl;
        return ERROR_IO_ERROR;
    }

    file.seekp(slot_pos, ios::beg);
    file.write((const char*)&entry, sizeof(entry));
    
    file.seekp(0, ios::end);
    file.write(data.c_str(), data.size());
    
    file.flush();
    file.close();

    cout << "SUCCESS: Created file '" << path << "' (" << data.size() << " bytes)" << endl;
    return SUCCESS;
}

int file_read(void* session, const string& path, string& content) {
    if (!session) {
        cerr << "Error: Invalid session" << endl;
        return ERROR_INVALID_OPERATION;
    }

    ifstream file("../compiled/test.omni", ios::binary);
    if (!file) {
        cerr << "Error: Cannot open file system" << endl;
        return ERROR_IO_ERROR;
    }

    FileEntry entry("", Entry_FILE, 0, 0, "", 0);
    uint64_t pos;
    
    if (!find_file(file, path, entry, pos)) {
        cerr << "Error: File not found: " << path << endl;
        file.close();
        return ERROR_NOT_FOUND;
    }

    vector<char> buffer(entry.size);
    uint64_t data_pos = pos + sizeof(FileEntry);
    file.seekg(data_pos, ios::beg);
    file.read(buffer.data(), entry.size);
    
    content = string(buffer.begin(), buffer.end());
    
    file.close();

    cout << "SUCCESS: Read file '" << path << "' (" << entry.size << " bytes)" << endl;
    return SUCCESS;
}


int file_delete(void* session, const string& path) {
    if (!session) {
        cerr << "Error: Invalid session" << endl;
        return ERROR_INVALID_OPERATION;
    }

    ifstream check_file("../compiled/test.omni", ios::binary);
    if (!check_file) {
        cerr << "Error: Cannot open file system" << endl;
        return ERROR_IO_ERROR;
    }

    FileEntry entry("", Entry_FILE, 0, 0, "", 0);
    uint64_t pos;
    
    if (!find_file(check_file, path, entry, pos)) {
        cerr << "Error: File not found: " << path << endl;
        check_file.close();
        return ERROR_NOT_FOUND;
    }
    check_file.close();

    entry.name[0] = '\0';

    fstream file("../compiled/test.omni", ios::in | ios::out | ios::binary);
    if (!file) {
        cerr << "Error: Cannot open file system for writing" << endl;
        return ERROR_IO_ERROR;
    }
    
    file.seekp(pos, ios::beg);
    file.write((char*)&entry, sizeof(entry));
    file.close();

    cout << "SUCCESS: Deleted file '" << path << "'" << endl;
    return SUCCESS;
}

int file_exists(void* session, const string& path) {
    if (!session) {
        cerr << "Error: Invalid session" << endl;
        return ERROR_INVALID_OPERATION;
    }

    ifstream file("../compiled/test.omni", ios::binary);
    if (!file) {
        cerr << "Error: Cannot open file system" << endl;
        return ERROR_IO_ERROR;
    }

    FileEntry entry("", Entry_FILE, 0, 0, "", 0);
    uint64_t pos;
    
    if (find_file(file, path, entry, pos)) {
        file.close();
        cout << "File '" << path << "' exists" << endl;
        return SUCCESS;
    }

    file.close();
    cout << "File '" << path << "' does not exist" << endl;
    return ERROR_NOT_FOUND;
}


int file_rename(void* session, const string& old_path, const string& new_path) {

    if (!session) {
        cerr << "Error: Invalid session" << endl;
        return ERROR_INVALID_OPERATION;
    }

    ifstream check_file("../compiled/test.omni", ios::binary);
    if (!check_file) {
        cerr << "Error: Cannot open file system" << endl;
        return ERROR_IO_ERROR;
    }

    FileEntry entry("", Entry_FILE, 0, 0, "", 0);
    uint64_t pos;
    
    if (!find_file(check_file, old_path, entry, pos)) {
        cerr << "Error: File not found: " << old_path << endl;
        check_file.close();
        return ERROR_NOT_FOUND;
    }
    check_file.close();


    copy(entry.name, sizeof(entry.name), new_path);

    fstream file("../compiled/test.omni", ios::in | ios::out | ios::binary);
    if (!file) {
        cerr << "Error: Cannot open file system for writing" << endl;
        return ERROR_IO_ERROR;
    }
    
    file.seekp(pos, ios::beg);
    file.write((char*)&entry, sizeof(entry));
    file.close();

    cout << "SUCCESS: Renamed '" << old_path << "' to '" << new_path << "'" << endl;
    return SUCCESS;
}

int main() {
    cout << "\n========================================" << endl;
    cout << "  FILE OPERATIONS TEST" << endl;
    cout << "========================================\n" << endl;

    // Create test session
    UserInfo user("abdullah", "HASH_1234", ADMIN, time(nullptr));
    SessionInfo session("SID_001", user, time(nullptr));

    // Test 1: Create file
    cout << "Test 1: Creating file..." << endl;
    string data = "This is a test file in .omni!";
    int result = file_create(&session, "hello.txt", data);
    if (result != SUCCESS) {
        cerr << "Failed to create file!" << endl;
        return result;
    }

    // Test 2: Check if file exists
    cout << "\nTest 2: Checking if file exists..." << endl;
    file_exists(&session, "hello.txt");

    // Test 3: Read file
    cout << "\nTest 3: Reading file..." << endl;
    string content;
    result = file_read(&session, "hello.txt", content);
    if (result == SUCCESS) {
        cout << "  Content: " << content << endl;
    }

    // Test 4: Rename file
    cout << "\nTest 4: Renaming file..." << endl;
    file_rename(&session, "hello.txt", "renamed.txt");

    // Test 5: Check new name exists
    cout << "\nTest 5: Checking renamed file..." << endl;
    file_exists(&session, "renamed.txt");

    // Test 6: Delete file
    cout << "\nTest 6: Deleting file..." << endl;
    file_delete(&session, "renamed.txt");

    // Test 7: Check if deleted
    cout << "\nTest 7: Verifying deletion..." << endl;
    file_exists(&session, "renamed.txt");

    cout << "\n========================================" << endl;
    cout << "  ✓ ALL TESTS COMPLETED!" << endl;
    cout << "========================================\n" << endl;

    return SUCCESS;
}