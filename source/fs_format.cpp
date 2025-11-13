#include <iostream>
#include <fstream>
#include <ctime>
#include <algorithm>
#include "include/odf_types.hpp"

using namespace std;


void copy(char* dest, size_t dest_size, const string& src) {
    size_t copy_len = min(src.length(), dest_size - 1);
    for (size_t i = 0; i < copy_len; i++) {
        dest[i] = src[i];
    }
    dest[copy_len] = '\0';
    
    for (size_t i = copy_len + 1; i < dest_size; i++) {
        dest[i] = '\0';
    }
}

/**
 * Creates a new .omni file system
 * Returns: 0 on success, negative error code on failure
 */
int fs_format(const string& omni_path,
              const string& student_id,
              const string& submission_date,
              uint64_t total_size,
              uint64_t block_size)
{
    // Step 1: Create new empty file
    ofstream file(omni_path, ios::binary | ios::trunc);
    if (!file) {
        cerr << "Error: Cannot create file " << omni_path << endl;
        return ERROR_IO_ERROR;
    }

    OMNIHeader header(0x00010000, total_size, sizeof(OMNIHeader), block_size);  
    
    // Step 3: Fill basic info
    std::memcpy(header.magic, "OMNIFS01", 8);
    header.format_version = 0x00010000;  // Version 1.0
    header.total_size = total_size;
    header.header_size = sizeof(OMNIHeader);
    header.block_size = block_size;

    // Step 4: Add student info
    copy(header.student_id, sizeof(header.student_id), student_id);
    copy(header.submission_date, sizeof(header.submission_date), submission_date);

    // Step 5: Set default values
    copy(header.config_hash, sizeof(header.config_hash), "INIT_HASH");
    header.config_timestamp = uint64_t(time(nullptr));
    header.user_table_offset = uint32_t(sizeof(OMNIHeader));
    header.max_users = 100;
    header.file_state_storage_offset = 0;
    header.change_log_offset = 0;

    // Step 6: Write header to file
    file.write((const char*)(&header), sizeof(header));

    // Step 7: Allocate remaining space (fill file to total_size)
    if (total_size > sizeof(header)) {
        // Move the file pointer to the last position
        uint64_t last_position = total_size - 1;
        file.seekp(last_position);
        file.put('\0');
    }

    file.close();

    // Success message
    double size_mb = total_size / (1024.0 * 1024.0);
    cout << "SUCCESS: Created " << omni_path 
         << " (" << size_mb << " MB)" << endl;

    return SUCCESS;
}

int main() {
    // Configuration
    string omni_file = "../compiled/test.omni";
    string student_id = "BSAI-24003";           // YOUR ID HERE
    string date = "2025-11-11";               // TODAY'S DATE
    uint64_t total_size = 5 * 1024 * 1024;    // 5 MB
    uint64_t block_size = 4096;               // 4 KB per block

    // Create the file system
    int result = fs_format(omni_file, student_id, date, total_size, block_size);
    
    if (result == 0) {
        cout << "\nFile system created successfully!" << endl;
    } else {
        cout << "\nFile system creation failed!" << endl;
    }

    return result;
}