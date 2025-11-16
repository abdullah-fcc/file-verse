
#include "../include/odf_types.hpp"
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>
using namespace std;

bool compare(const char* a, const char* b, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

bool compare_name(const char* name_array, const string& name) {
    size_t i = 0;
    while (i < name.length() && name_array[i] != '\0') {
        if (name_array[i] != name[i]) 
            return false;
        i++;
    }
    return (i == name.length() && name_array[i] == '\0');
}

void copy_name(char* dest, size_t dest_size, const string& src) {
    size_t len = min(src.length(), dest_size - 1);
    for (size_t i = 0; i < len; i++) {
        dest[i] = src[i];
    }
    dest[len] = '\0';
}
vector<FileEntry> load_all_entries() {
    vector<FileEntry> entries;
    
    ifstream file("../compiled/test.omni", ios::binary);
    if (!file) {
        return entries;
    }
    uint64_t start_pos = sizeof(OMNIHeader) + (100 * sizeof(UserInfo));
    file.seekg(start_pos, ios::beg);

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