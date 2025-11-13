// reset_omni.cpp - Simple script to reset the OMNI file
#include <iostream>
#include <cstdio>  // For remove()
using namespace std;

int main() {
    string omni_file = "../compiled/test.omni";
    
    cout << "\n========================================" << endl;
    cout << "  RESET OMNI FILE" << endl;
    cout << "========================================\n" << endl;
    
    cout << "Deleting old file: " << omni_file << endl;
    
    // Delete the old file
    int result = remove(omni_file.c_str());
    
    if (result == 0) {
        cout << "✓ Old file deleted successfully" << endl;
    } else {
        cout << "! File doesn't exist or already deleted" << endl;
    }
    
    cout << "\n========================================" << endl;
    cout << "Now run these commands:" << endl;
    cout << "  1. ./fs_format    (to create fresh file)" << endl;
    cout << "  2. ./user_manager (to create users & test)" << endl;
    cout << "========================================\n" << endl;
    
    return 0;
}