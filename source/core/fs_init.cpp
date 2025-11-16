#include <iostream>
#include <fstream>
#include "../include/odf_types.hpp"
#include "../include/ofs_functions.hpp"
#include "helper.hpp"
using namespace std;



/**
 * Opens and validates an existing .omni file
 * Returns: 0 on success, negative error code on failure
 */
int fs_init(const string& omni_path)
{
    ifstream file(omni_path, ios::binary);
    if (!file) {
        cerr << "Error: Cannot open file " << omni_path << endl;
        return ERROR_NOT_FOUND;
    }

    OMNIHeader header(0, 0, 0, 0); 
    file.read((char*)(&header), sizeof(header));
    
    if (!file) {
        cerr << "Error: Failed to read header" << endl;
        return ERROR_IO_ERROR;
    }

    if (!compare(header.magic, "OMNIFS01", 8)) {
        cerr << "Error: Invalid file format (not an OMNI file)" << endl;
        return ERROR_INVALID_CONFIG;
    }

    cout << "\n=== OMNI File System Loaded ===" << endl;
    cout << "\nBasic Info:" << endl;
    cout << "  Magic:           " << header.magic << endl;
    cout << "  Format Version:  " << header.format_version << endl;
    cout << "  Total Size:      " << header.total_size << " bytes" << endl;
    cout << "  Header Size:     " << header.header_size << " bytes" << endl;
    cout << "  Block Size:      " << header.block_size << " bytes" << endl;
    
    cout << "\nStudent Info:" << endl;
    cout << "  Student ID:      " << header.student_id << endl;
    cout << "  Submission Date: " << header.submission_date << endl;
    
    cout << "\nConfiguration:" << endl;
    cout << "  Max Users:       " << header.max_users << endl;
    cout << "  Config Time:     " << header.config_timestamp << endl;
    
    cout << "\n================================\n" << endl;

    file.close();
    return SUCCESS;
}

int main() {
    string omni_file = "../compiled/test.omni";

    int result = fs_init(omni_file);
    
    if (result == SUCCESS) {
        cout << "File system initialized successfully!" << endl;
    } else {
        cout << "File system initialization failed!" << endl;
    }
    
    return result;
}