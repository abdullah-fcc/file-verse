#include "odf_types.hpp"
#include <string>
#include <vector>

// Core System
int fs_init(void** instance, const char* omni_path, const char* config_path);
void fs_shutdown(void* instance);
int fs_format(const std::string& omni_path, const std::string& student_id, 
              const std::string& submission_date, uint64_t total_size, uint64_t block_size);

// User Management
int user_login(void** session, const std::string& username, const std::string& password, const std::string& omni_path);
int user_logout(void* session);
int user_create(const std::string& omni_path, const std::string& username, 
                const std::string& password, uint32_t role);
void user_list_all();

// File Operations
int file_create(void* session, const std::string& path, const std::string& data);
int file_read(void* session, const std::string& path, std::string& content);
int file_delete(void* session, const std::string& path);
int file_exists(void* session, const std::string& path);
int file_rename(void* session, const std::string& old_path, const std::string& new_path);

// Directory Operations
int dir_create(void* session, const std::string& path);
int dir_list(void* session, const std::string& path, std::vector<FileEntry>& children);
int dir_delete(void* session, const std::string& path);
int dir_exists(void* session, const std::string& path);

// Information Functions
int get_metadata(void* session, const std::string& path, FileMetadata& meta);
int set_permissions(void* session, const std::string& path, uint32_t permissions);
int get_stats(void* session, FSStats& stats);
std::string get_error_message(int error_code);
