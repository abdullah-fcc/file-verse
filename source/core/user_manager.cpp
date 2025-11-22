#include <iostream>
#include <fstream>
#include <vector>
#include <ctime>
#include <algorithm>
#include "../include/odf_types.hpp"
#include "../include/ofs_functions.hpp"
#include "helper.hpp"
using namespace std;

vector<UserInfo> user_list_internal;
bool users_loaded = false;

vector<SessionInfo> active_sessions;

bool compare_username(const char* username_array, const string& username) {
    size_t i = 0;
    while (i < username.length() && username_array[i] != '\0') {
        if (username_array[i] != username[i]) {
            return false;
        }
        i++;
    }
    return (i == username.length() && username_array[i] == '\0');
}

string hash_password(const string& password) {
    int hash = 0;
    for (size_t i = 0; i < password.length(); i++) {
        hash = hash + (password[i] * (i + 1));
    }
    return to_string(hash);
}

string generate_session_id(const string& username) {
    return username + "_" + to_string(time(nullptr));
}

UserInfo* find_user(const string& username) {
    for (size_t i = 0; i < user_list_internal.size(); i++) {
        if (compare_username(user_list_internal[i].username, username)) {
            return &user_list_internal[i];
        }
    }
    return nullptr;
}

bool user_exists(const string& username) {
    return find_user(username) != nullptr;
}

SessionInfo* find_session(const string& session_id) {
    for (size_t i = 0; i < active_sessions.size(); i++) {
        string current_id(active_sessions[i].session_id);
        if (current_id == session_id) {
            return &active_sessions[i];
        }
    }
    return nullptr;
}

int load_users(const string& omni_path) {
    ifstream file(omni_path, ios::binary);
    if (!file) {
        cerr << "Error: Cannot open file" << endl;
        return ERROR_IO_ERROR;
    }

    OMNIHeader header(0, 0, 0, 0);
    file.read((char*)&header, sizeof(header));

    if (!compare(header.magic, "OMNIFS01", 8)) {
        cerr << "Error: Invalid file format" << endl;
        return ERROR_INVALID_CONFIG;
    }

    file.seekg(header.user_table_offset, ios::beg);

    user_list_internal.clear();

    for (uint32_t i = 0; i < header.max_users; i++) {
        UserInfo user("", "", NORMAL, 0);
        file.read((char*)&user, sizeof(user));

        if (user.is_active == 1 && user.username[0] != '\0') {
            user_list_internal.push_back(user);
        }
    }

    users_loaded = true;
    cout << "Loaded " << user_list_internal.size() << " users into memory" << endl;
    
    file.close();
    return SUCCESS;
}

// NEW FUNCTION - Returns user list to caller
int user_list(const string& omni_path, vector<UserInfo>& users) {
    if (!users_loaded) {
        int result = load_users(omni_path);
        if (result != SUCCESS) {
            return result;
        }
    }
    
    users = user_list_internal;
    return SUCCESS;
}

int user_create(const string& omni_path, 
                const string& username, 
                const string& password, 
                uint32_t role) 
{
    if (!users_loaded) {
        load_users(omni_path);
    }

    if (user_exists(username)) {
        cerr << "Error: User '" << username << "' already exists" << endl;
        return ERROR_FILE_EXISTS;
    }

    string hashed_pw = hash_password(password);
    UserInfo new_user(username, hashed_pw, (UserRole)role, (uint64_t)time(nullptr));

    fstream file(omni_path, ios::in | ios::out | ios::binary);
    if (!file) {
        cerr << "Error: Cannot open file" << endl;
        return ERROR_IO_ERROR;
    }

    OMNIHeader header(0, 0, 0, 0);
    file.read((char*)&header, sizeof(header));

    file.seekp(header.user_table_offset, ios::beg);

    for (uint32_t i = 0; i < header.max_users; i++) {
        UserInfo temp("", "", NORMAL, 0);
        
        uint64_t current_pos = header.user_table_offset + (i * sizeof(UserInfo));
        file.seekg(current_pos, ios::beg);
        file.read((char*)&temp, sizeof(temp));

        if (temp.username[0] == '\0' || temp.is_active == 0) {
            file.seekp(current_pos, ios::beg);
            file.write((char*)&new_user, sizeof(new_user));
            
            cout << "SUCCESS: User '" << username << "' created at slot " << i << endl;
            file.close();
            user_list_internal.push_back(new_user);
            
            return SUCCESS;
        }
    }

    file.close();
    cerr << "Error: No free user slots available" << endl;
    return ERROR_NO_SPACE;
}

void user_list_all() {
    cout << "\n=== All Users ===" << endl;
    cout << "Total: " << user_list_internal.size() << " users\n" << endl;
    
    for (size_t i = 0; i < user_list_internal.size(); i++) {
        UserInfo& u = user_list_internal[i];
        cout << (i + 1) << ". " << u.username;
        cout << " (Role: " << (u.role == ADMIN ? "Admin" : "Normal") << ")";
        cout << endl;
    }
    cout << "================\n" << endl;
}

int user_login(void** session, const string& username, const string& password, const string& omni_path) {
    ifstream file(omni_path, ios::binary);
    if (!file) {
        cerr << "Error: Cannot open file " << omni_path << endl;
        return ERROR_IO_ERROR;
    }

    OMNIHeader header(0, 0, 0, 0);
    file.read((char*)&header, sizeof(header));

    file.seekg(header.user_table_offset, ios::beg);

    for (uint32_t i = 0; i < header.max_users; i++) {
        UserInfo user("", "", NORMAL, 0);
        file.read((char*)&user, sizeof(user));

        if (!file) 
            break;

        if (user.is_active == 1 && compare_username(user.username, username)) {
            
            string input_hash = hash_password(password);
            string stored_hash(user.password_hash);

            if (input_hash != stored_hash) {
                cerr << "Error: Invalid password for user '" << username << "'" << endl;
                file.close();
                return ERROR_PERMISSION_DENIED;
            }

            string session_id = generate_session_id(username);
            SessionInfo new_session(session_id, user, time(nullptr));

            active_sessions.push_back(new_session);
            *session = &active_sessions[active_sessions.size() - 1];

            cout << "SUCCESS: User '" << username << "' logged in" << endl;

            file.close();
            return SUCCESS;
        }
    }

    cerr << "Error: User '" << username << "' not found" << endl;
    file.close();
    return ERROR_NOT_FOUND;
}

int user_logout(void* session) {
    if (!session) {
        cerr << "Error: Invalid session pointer" << endl;
        return ERROR_INVALID_SESSION;
    }

    SessionInfo* session_ptr = (SessionInfo*)session;
    string session_id(session_ptr->session_id);

    for (size_t i = 0; i < active_sessions.size(); i++) {
        string current_id(active_sessions[i].session_id);
        if (current_id == session_id) {
            cout << "SUCCESS: Logged out session " << session_id << endl;
            active_sessions.erase(active_sessions.begin() + i);
            return SUCCESS;
        }
    }

    cerr << "Error: Session not found" << endl;
    return ERROR_INVALID_SESSION;
}

int get_session_info(void* session, SessionInfo* out_info) {
    if (!session || !out_info) {
        cerr << "Error: Invalid session or output pointer" << endl;
        return ERROR_INVALID_SESSION;
    }

    SessionInfo* session_ptr = (SessionInfo*)session;
    *out_info = *session_ptr;

    return SUCCESS;
}

void list_active_sessions() {
    cout << "\n=== Active Sessions ===" << endl;
    cout << "Total: " << active_sessions.size() << " sessions\n" << endl;

    for (size_t i = 0; i < active_sessions.size(); i++) {
        SessionInfo& s = active_sessions[i];
        cout << (i + 1) << ". " << s.user.username;
        cout << " (Session: " << s.session_id << ")";
        cout << endl;
    }
    cout << "======================\n" << endl;
}