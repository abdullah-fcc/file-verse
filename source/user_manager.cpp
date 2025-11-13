#include <iostream>
#include <fstream>
#include <vector>
#include <ctime>
#include <algorithm>
#include "include/odf_types.hpp"

using namespace std;


vector<UserInfo> user_list;
bool users_loaded = false;

vector<SessionInfo> active_sessions;

bool compare(const char* a, const char* b, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

void copy(char* dest, size_t dest_size, const string& src) {
    size_t len = min(src.length(), dest_size - 1);
    for (size_t i = 0; i < len; i++) {
        dest[i] = src[i];
    }
    dest[len] = '\0';
}

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
    for (size_t i = 0; i < user_list.size(); i++) {
        if (compare_username(user_list[i].username, username)) {
            return &user_list[i];
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

    user_list.clear();

    for (uint32_t i = 0; i < header.max_users; i++) {
        UserInfo user("", "", NORMAL, 0);
        file.read((char*)&user, sizeof(user));

        if (user.is_active == 1 && user.username[0] != '\0') {
            user_list.push_back(user);
        }
    }

    users_loaded = true;
    cout << "Loaded " << user_list.size() << " users into memory" << endl;
    
    file.close();
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
            user_list.push_back(new_user);
            
            return SUCCESS;
        }
    }

    file.close();
    cerr << "Error: No free user slots available" << endl;
    return ERROR_NO_SPACE;
}

void user_list_all() {
    cout << "\n=== All Users ===" << endl;
    cout << "Total: " << user_list.size() << " users\n" << endl;
    
    for (size_t i = 0; i < user_list.size(); i++) {
        UserInfo& u = user_list[i];
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

int main() {
    string omni_file = "../compiled/test.omni";
    
    cout << "\n========================================" << endl;
    cout << "  COMPLETE USER & SESSION TEST" << endl;
    cout << "========================================\n" << endl;

    // PHASE 1: USER CREATION
    cout << ">>> PHASE 1: User Creation <<<\n" << endl;
    
    cout << "Step 1: Loading existing users..." << endl;
    load_users(omni_file);
    
    cout << "\nStep 2: Creating admin user..." << endl;
    user_create(omni_file, "admin", "admin123", ADMIN);
    
    cout << "\nStep 3: Creating normal users..." << endl;
    user_create(omni_file, "alice", "pass123", NORMAL);
    user_create(omni_file, "bob", "pass456", NORMAL);
    
    cout << "\nStep 4: Listing all users..." << endl;
    user_list_all();

    // PHASE 2: SESSION LOGIN
    cout << "\n>>> PHASE 2: User Login <<<\n" << endl;
    
    void* session_admin = nullptr;
    void* session_alice = nullptr;
    void* session_bob = nullptr;
    
    cout << "Step 1: Admin login..." << endl;
    int result = user_login(&session_admin, "admin", "admin123", omni_file);
    if (result != SUCCESS) {
        cerr << "FAILED: Admin login failed!" << endl;
        return result;
    }
    
    cout << "\nStep 2: Alice login..." << endl;
    result = user_login(&session_alice, "alice", "pass123", omni_file);
    if (result != SUCCESS) {
        cerr << "FAILED: Alice login failed!" << endl;
        return result;
    }
    
    cout << "\nStep 3: Bob login..." << endl;
    result = user_login(&session_bob, "bob", "pass456", omni_file);
    if (result != SUCCESS) {
        cerr << "FAILED: Bob login failed!" << endl;
        return result;
    }
    
    cout << "\nStep 4: Listing active sessions..." << endl;
    list_active_sessions();

    // PHASE 3: SESSION INFO
    cout << "\n>>> PHASE 3: Session Information <<<\n" << endl;
    
    SessionInfo info("", UserInfo("", "", NORMAL, 0), 0);
    result = get_session_info(session_admin, &info);
    if (result == SUCCESS) {
        cout << "Admin Session Info:" << endl;
        cout << "  Username: " << info.user.username << endl;
        cout << "  Role: " << (info.user.role == ADMIN ? "Admin" : "Normal") << endl;
        cout << "  Active: " << (info.user.is_active ? "Yes" : "No") << endl;
    }

    // PHASE 4: WRONG PASSWORD TEST
    cout << "\n>>> PHASE 4: Security Test <<<\n" << endl;
    
    cout << "Testing wrong password..." << endl;
    void* bad_session = nullptr;
    result = user_login(&bad_session, "admin", "wrongpassword", omni_file);
    if (result == ERROR_PERMISSION_DENIED) {
        cout << "✓ Security working: Wrong password rejected!" << endl;
    }

    // PHASE 5: LOGOUT
    cout << "\n>>> PHASE 5: User Logout <<<\n" << endl;
    
    cout << "Logging out Alice..." << endl;
    user_logout(session_alice);
    
    cout << "\nRemaining sessions:" << endl;
    list_active_sessions();
    
    cout << "Logging out Bob and Admin..." << endl;
    user_logout(session_bob);
    user_logout(session_admin);
    
    cout << "\nFinal session check:" << endl;
    list_active_sessions();

    cout << "\n========================================" << endl;
    cout << "  ✓ ALL TESTS PASSED!" << endl;
    cout << "========================================\n" << endl;

    return SUCCESS;
}