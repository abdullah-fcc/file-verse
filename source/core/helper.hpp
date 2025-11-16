#include "../include/odf_types.hpp"
#include <string>
#include <algorithm>
using namespace std;

bool compare(const char* a, const char* b, size_t len);
bool compare_name(const char* stored, const std::string& given);
void copy_name(char* dest, size_t dest_size, const std::string& src);

vector<FileEntry> load_all_entries(); 


