// server.cpp - Complete Fixed OFS Socket Server with JSON Escaping
#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <cstdio>
#include "../include/odf_types.hpp"
#include "../include/ofs_functions.hpp"
#include "../core/helper.hpp"

using namespace std;

// ============================================================================
// JSON HELPER FUNCTIONS
// ============================================================================

// Escape special characters for JSON string
string escape_json_string(const string& input) {
    string output;
    output.reserve(input.length() * 2); // Reserve space for efficiency
    
    for (char c : input) {
        switch (c) {
            case '"':  output += "\\\""; break;
            case '\\': output += "\\\\"; break;
            case '\b': output += "\\b"; break;
            case '\f': output += "\\f"; break;
            case '\n': output += "\\n"; break;
            case '\r': output += "\\r"; break;
            case '\t': output += "\\t"; break;
            default:
                if (c < 0x20) {
                    // Escape control characters
                    char buf[7];
                    snprintf(buf, sizeof(buf), "\\u%04x", (unsigned char)c);
                    output += buf;
                } else {
                    output += c;
                }
        }
    }
    return output;
}

// ============================================================================
// JSON PARSING
// ============================================================================

struct JSONRequest {
    string operation;
    string session_id;
    string request_id;
    
    // Parameters
    string username;
    string password;
    string path;
    string old_path;
    string new_path;
    string data;
    uint32_t role;
    uint32_t permissions;
};

struct JSONResponse {
    string status;
    string operation;
    string request_id;
    int error_code;
    string error_message;
    string data;
};

// Extract value from JSON
string extract_json_value(const string& json, const string& key) {
    string search = "\"" + key + "\":\"";
    size_t pos = json.find(search);
    if (pos == string::npos) return "";
    
    size_t start = pos + search.length();
    size_t end = json.find("\"", start);
    if (end == string::npos) return "";
    
    return json.substr(start, end - start);
}

// Better extraction for data field that may contain escaped characters
string extract_json_data(const string& json) {
    string search = "\"data\":\"";
    size_t pos = json.find(search);
    if (pos == string::npos) return "";
    
    size_t start = pos + search.length();
    string result;
    bool escaped = false;
    
    for (size_t i = start; i < json.length(); i++) {
        char c = json[i];
        
        if (escaped) {
            // Handle escape sequences
            switch (c) {
                case 'n': result += '\n'; break;
                case 't': result += '\t'; break;
                case 'r': result += '\r'; break;
                case '"': result += '"'; break;
                case '\\': result += '\\'; break;
                case 'b': result += '\b'; break;
                case 'f': result += '\f'; break;
                default: result += c;
            }
            escaped = false;
        } else if (c == '\\') {
            escaped = true;
        } else if (c == '"') {
            // End of string value
            break;
        } else {
            result += c;
        }
    }
    
    cout << "[DEBUG] Extracted data length: " << result.length() << " bytes" << endl;
    if (result.length() > 0 && result.length() <= 100) {
        cout << "[DEBUG] Data content: " << result << endl;
    }
    
    return result;
}

// Extract numeric value
int extract_json_number(const string& json, const string& key) {
    string search = "\"" + key + "\":";
    size_t pos = json.find(search);
    if (pos == string::npos) return 0;
    
    size_t start = pos + search.length();
    size_t end = json.find_first_of(",}", start);
    if (end == string::npos) return 0;
    
    string num_str = json.substr(start, end - start);
    return atoi(num_str.c_str());
}

// Parse JSON request
JSONRequest parse_json_request(const string& json) {
    JSONRequest req;
    
    req.operation = extract_json_value(json, "operation");
    req.session_id = extract_json_value(json, "session_id");
    req.request_id = extract_json_value(json, "request_id");
    
    // Extract parameters
    size_t params_start = json.find("\"parameters\":{");
    if (params_start != string::npos) {
        string params = json.substr(params_start);
        req.username = extract_json_value(params, "username");
        req.password = extract_json_value(params, "password");
        req.path = extract_json_value(params, "path");
        req.old_path = extract_json_value(params, "old_path");
        req.new_path = extract_json_value(params, "new_path");
        
        // Use special extraction for data field to handle escape sequences
        req.data = extract_json_data(json);
        
        req.role = extract_json_number(params, "role");
    }
    
    return req;
}

string create_json_response(const JSONResponse& resp) {
    string json = "{";
    json += "\"status\":\"" + resp.status + "\",";
    json += "\"operation\":\"" + resp.operation + "\",";
    json += "\"request_id\":\"" + resp.request_id + "\"";
    
    if (resp.status == "error") {
        json += ",\"error_code\":" + to_string(resp.error_code);
        json += ",\"error_message\":\"" + escape_json_string(resp.error_message) + "\"";
    } else if (!resp.data.empty()) {
        json += ",\"data\":{" + resp.data + "}";
    }
    
    json += "}";
    return json;
}

// ============================================================================
// FIFO QUEUE SYSTEM
// ============================================================================

struct QueuedOperation {
    JSONRequest request;
    int client_socket;
    
    QueuedOperation(const JSONRequest& req, int sock) 
        : request(req), client_socket(sock) {}
};

class FIFOQueue {
private:
    queue<QueuedOperation> operations;
    mutex queue_mutex;
    condition_variable queue_cv;
    bool shutdown_flag;

public:
    FIFOQueue() : shutdown_flag(false) {}
    
    void enqueue(const QueuedOperation& op) {
        lock_guard<mutex> lock(queue_mutex);
        operations.push(op);
        queue_cv.notify_one();
        
        cout << "[QUEUE] Enqueued: " << op.request.operation 
             << " (Size: " << operations.size() << ")" << endl;
    }
    
    bool dequeue(QueuedOperation& op) {
        unique_lock<mutex> lock(queue_mutex);
        
        queue_cv.wait(lock, [this] { 
            return !operations.empty() || shutdown_flag; 
        });
        
        if (shutdown_flag && operations.empty()) {
            return false;
        }
        
        op = operations.front();
        operations.pop();
        return true;
    }
    
    void shutdown() {
        lock_guard<mutex> lock(queue_mutex);
        shutdown_flag = true;
        queue_cv.notify_all();
    }
};

// ============================================================================
// OPERATION PROCESSOR
// ============================================================================

class OperationProcessor {
private:
    string omni_path;
    
public:
    OperationProcessor(const string& path) : omni_path(path) {}
    
    JSONResponse process(const JSONRequest& req) {
        JSONResponse resp;
        resp.operation = req.operation;
        resp.request_id = req.request_id;
        
        cout << "[PROCESSOR] Executing: " << req.operation << " | Path: '" << req.path << "'" << endl;
        
        // Route to actual functions
        if (req.operation == "user_login") return process_user_login(req);
        else if (req.operation == "user_logout") return process_user_logout(req);
        else if (req.operation == "user_create") return process_user_create(req);
        else if (req.operation == "user_list") return process_user_list(req);
        else if (req.operation == "file_create") return process_file_create(req);
        else if (req.operation == "file_read") return process_file_read(req);
        else if (req.operation == "file_delete") return process_file_delete(req);
        else if (req.operation == "file_exists") return process_file_exists(req);
        else if (req.operation == "file_rename") return process_file_rename(req);
        else if (req.operation == "dir_create") return process_dir_create(req);
        else if (req.operation == "dir_list") return process_dir_list(req);
        else if (req.operation == "dir_delete") return process_dir_delete(req);
        else if (req.operation == "dir_exists") return process_dir_exists(req);
        else if (req.operation == "get_stats") return process_get_stats(req);
        else if (req.operation == "get_metadata") return process_get_metadata(req);
        else {
            resp.status = "error";
            resp.error_code = ERROR_NOT_IMPLEMENTED;
            resp.error_message = get_error_message(ERROR_NOT_IMPLEMENTED);
        }
        
        return resp;
    }
    
private:
    // User Operations
    JSONResponse process_user_login(const JSONRequest& req) {
        JSONResponse resp;
        void* session = nullptr;
        
        int result = user_login(&session, req.username, req.password, omni_path);
        
        if (result == SUCCESS) {
            resp.status = "success";
            resp.data = "\"session_id\":\"session_" + escape_json_string(req.username) + "\"";
        } else {
            resp.status = "error";
            resp.error_code = result;
            resp.error_message = get_error_message(result);
        }
        
        return resp;
    }
    
    JSONResponse process_user_logout(const JSONRequest& req) {
        JSONResponse resp;
        resp.status = "success";
        resp.data = "\"message\":\"Logged out successfully\"";
        return resp;
    }
    
    JSONResponse process_user_create(const JSONRequest& req) {
        JSONResponse resp;
        
        cout << "[USER_CREATE] Username: " << req.username 
             << ", Role: " << req.role << endl;
        
        int result = user_create(omni_path, req.username, req.password, req.role);
        
        if (result == SUCCESS) {
            resp.status = "success";
            resp.data = "\"message\":\"User created successfully\"";
        } else {
            resp.status = "error";
            resp.error_code = result;
            resp.error_message = get_error_message(result);
        }
        
        return resp;
    }
    
    JSONResponse process_user_list(const JSONRequest& req) {
        JSONResponse resp;
        vector<UserInfo> users;
        
        int result = user_list(omni_path, users);
        
        if (result == SUCCESS) {
            resp.status = "success";
            string users_json = "\"users\":[";
            for (size_t i = 0; i < users.size(); i++) {
                users_json += "{";
                users_json += "\"username\":\"" + escape_json_string(string(users[i].username)) + "\",";
                users_json += "\"role\":" + to_string(users[i].role) + ",";
                users_json += "\"password_hash\":\"" + escape_json_string(string(users[i].password_hash)) + "\"";
                users_json += "}";
                if (i < users.size() - 1) users_json += ",";
            }
            users_json += "]";
            resp.data = users_json;
        } else {
            resp.status = "error";
            resp.error_code = result;
            resp.error_message = get_error_message(result);
        }
        
        return resp;
    }
    
    // File Operations
    JSONResponse process_file_create(const JSONRequest& req) {
        JSONResponse resp;
        
        SessionInfo dummy_session("session_1", UserInfo("admin", "", ADMIN, 0), time(nullptr));
        void* session = &dummy_session;
        
        cout << "[FILE_CREATE] Path: '" << req.path << "'" << endl;
        cout << "[FILE_CREATE] Content length: " << req.data.length() << " bytes" << endl;
        cout << "[FILE_CREATE] Content (first 100 chars): ";
        if (req.data.length() > 0) {
            string preview = req.data.substr(0, min((size_t)100, req.data.length()));
            for (char c : preview) {
                if (c >= 32 && c <= 126) cout << c;
                else cout << "\\x" << hex << (int)(unsigned char)c << dec;
            }
            cout << endl;
        } else {
            cout << "(EMPTY)" << endl;
        }
        
        int result = file_create(session, req.path, req.data);
        
        if (result == SUCCESS) {
            resp.status = "success";
            resp.data = "\"message\":\"File created successfully\"";
            cout << "[FILE_CREATE] SUCCESS" << endl;
        } else {
            resp.status = "error";
            resp.error_code = result;
            resp.error_message = get_error_message(result);
            cout << "[FILE_CREATE] FAILED with error: " << result << endl;
        }
        
        return resp;
    }
    
    JSONResponse process_file_read(const JSONRequest& req) {
        JSONResponse resp;
        
        SessionInfo dummy_session("session_1", UserInfo("admin", "", ADMIN, 0), time(nullptr));
        void* session = &dummy_session;
        string content;
        
        cout << "[FILE_READ] Path: '" << req.path << "'" << endl;
        
        int result = file_read(session, req.path, content);
        
        if (result == SUCCESS) {
            resp.status = "success";
            cout << "[FILE_READ] Success, content length: " << content.length() << " bytes" << endl;
            cout << "[FILE_READ] Content (first 100 chars): ";
            if (content.length() > 0) {
                string preview = content.substr(0, min((size_t)100, content.length()));
                for (char c : preview) {
                    if (c >= 32 && c <= 126) cout << c;
                    else cout << "\\x" << hex << (int)(unsigned char)c << dec;
                }
                cout << endl;
            } else {
                cout << "(EMPTY)" << endl;
            }
            
            // PROPERLY ESCAPE THE CONTENT FOR JSON
            resp.data = "\"content\":\"" + escape_json_string(content) + "\"";
        } else {
            resp.status = "error";
            resp.error_code = result;
            resp.error_message = get_error_message(result);
            cout << "[FILE_READ] Failed with error: " << result << endl;
        }
        
        return resp;
    }
    
    JSONResponse process_file_delete(const JSONRequest& req) {
        JSONResponse resp;
        
        SessionInfo dummy_session("session_1", UserInfo("admin", "", ADMIN, 0), time(nullptr));
        void* session = &dummy_session;
        
        int result = file_delete(session, req.path);
        
        if (result == SUCCESS) {
            resp.status = "success";
            resp.data = "\"message\":\"File deleted successfully\"";
        } else {
            resp.status = "error";
            resp.error_code = result;
            resp.error_message = get_error_message(result);
        }
        
        return resp;
    }
    
    JSONResponse process_file_exists(const JSONRequest& req) {
        JSONResponse resp;
        
        SessionInfo dummy_session("session_1", UserInfo("admin", "", ADMIN, 0), time(nullptr));
        void* session = &dummy_session;
        
        int result = file_exists(session, req.path);
        
        resp.status = "success";
        resp.data = "\"exists\":" + string(result == SUCCESS ? "true" : "false");
        
        return resp;
    }
    
    JSONResponse process_file_rename(const JSONRequest& req) {
        JSONResponse resp;
        
        SessionInfo dummy_session("session_1", UserInfo("admin", "", ADMIN, 0), time(nullptr));
        void* session = &dummy_session;
        
        int result = file_rename(session, req.old_path, req.new_path);
        
        if (result == SUCCESS) {
            resp.status = "success";
            resp.data = "\"message\":\"File renamed successfully\"";
        } else {
            resp.status = "error";
            resp.error_code = result;
            resp.error_message = get_error_message(result);
        }
        
        return resp;
    }
    
    // Directory Operations
    JSONResponse process_dir_create(const JSONRequest& req) {
        JSONResponse resp;
        
        SessionInfo dummy_session("session_1", UserInfo("admin", "", ADMIN, 0), time(nullptr));
        void* session = &dummy_session;
        
        cout << "[DIR_CREATE] Path: '" << req.path << "'" << endl;
        
        int result = dir_create(session, req.path);
        
        if (result == SUCCESS) {
            resp.status = "success";
            resp.data = "\"message\":\"Directory created successfully\"";
        } else {
            resp.status = "error";
            resp.error_code = result;
            resp.error_message = get_error_message(result);
        }
        
        return resp;
    }
    
    JSONResponse process_dir_list(const JSONRequest& req) {
        JSONResponse resp;
        
        SessionInfo dummy_session("session_1", UserInfo("admin", "", ADMIN, 0), time(nullptr));
        void* session = &dummy_session;
        vector<FileEntry> children;
        
        cout << "[DIR_LIST] Path: '" << req.path << "'" << endl;
        
        int result = dir_list(session, req.path, children);
        
        if (result == SUCCESS) {
            resp.status = "success";
            string files = "\"files\":[";
            for (size_t i = 0; i < children.size(); i++) {
                files += "{";
                files += "\"name\":\"" + escape_json_string(string(children[i].name)) + "\",";
                files += "\"type\":\"" + string(children[i].type == DIRECTORY ? "directory" : "file") + "\",";
                files += "\"size\":" + to_string(children[i].size);
                files += "}";
                if (i < children.size() - 1) files += ",";
            }
            files += "]";
            resp.data = files;
            
            cout << "[DIR_LIST] Found " << children.size() << " items" << endl;
        } else {
            resp.status = "error";
            resp.error_code = result;
            resp.error_message = get_error_message(result);
            cout << "[DIR_LIST] Failed with error: " << result << endl;
        }
        
        return resp;
    }
    
    JSONResponse process_dir_delete(const JSONRequest& req) {
        JSONResponse resp;
        
        SessionInfo dummy_session("session_1", UserInfo("admin", "", ADMIN, 0), time(nullptr));
        void* session = &dummy_session;
        
        int result = dir_delete(session, req.path);
        
        if (result == SUCCESS) {
            resp.status = "success";
            resp.data = "\"message\":\"Directory deleted successfully\"";
        } else {
            resp.status = "error";
            resp.error_code = result;
            resp.error_message = get_error_message(result);
        }
        
        return resp;
    }
    
    JSONResponse process_dir_exists(const JSONRequest& req) {
        JSONResponse resp;
        
        SessionInfo dummy_session("session_1", UserInfo("admin", "", ADMIN, 0), time(nullptr));
        void* session = &dummy_session;
        
        int result = dir_exists(session, req.path);
        
        resp.status = "success";
        resp.data = "\"exists\":" + string(result == SUCCESS ? "true" : "false");
        
        return resp;
    }
    
    // Information Operations
    JSONResponse process_get_stats(const JSONRequest& req) {
        JSONResponse resp;
        
        SessionInfo dummy_session("session_1", UserInfo("admin", "", ADMIN, 0), time(nullptr));
        void* session = &dummy_session;
        FSStats stats(0, 0, 0);
        
        int result = get_stats(session, stats);
        
        if (result == SUCCESS) {
            resp.status = "success";
            resp.data = "\"total_size\":" + to_string(stats.total_size) + ",";
            resp.data += "\"used_space\":" + to_string(stats.used_space) + ",";
            resp.data += "\"free_space\":" + to_string(stats.free_space) + ",";
            resp.data += "\"total_files\":" + to_string(stats.total_files) + ",";
            resp.data += "\"total_directories\":" + to_string(stats.total_directories);
        } else {
            resp.status = "error";
            resp.error_code = result;
            resp.error_message = get_error_message(result);
        }
        
        return resp;
    }
    
    JSONResponse process_get_metadata(const JSONRequest& req) {
        JSONResponse resp;
        
        SessionInfo dummy_session("session_1", UserInfo("admin", "", ADMIN, 0), time(nullptr));
        void* session = &dummy_session;
        FileMetadata meta("", FileEntry("", Entry_FILE, 0, 0, "", 0));
        
        int result = get_metadata(session, req.path, meta);
        
        if (result == SUCCESS) {
            resp.status = "success";
            resp.data = "\"size\":" + to_string(meta.entry.size) + ",";
            resp.data += "\"blocks_used\":" + to_string(meta.blocks_used) + ",";
            resp.data += "\"owner\":\"" + escape_json_string(string(meta.entry.owner)) + "\"";
        } else {
            resp.status = "error";
            resp.error_code = result;
            resp.error_message = get_error_message(result);
        }
        
        return resp;
    }
};

// ============================================================================
// PROCESSOR THREAD
// ============================================================================

void fifo_processor_thread(FIFOQueue* queue, OperationProcessor* processor) {
    cout << "[PROCESSOR] Started" << endl;
    
    while (true) {
        QueuedOperation op(JSONRequest(), 0);
        
        if (!queue->dequeue(op)) {
            break;
        }
        
        JSONResponse response = processor->process(op.request);
        string json_response = create_json_response(response);
        
        send(op.client_socket, json_response.c_str(), json_response.length(), 0);
        
        cout << "[PROCESSOR] Completed: " << op.request.operation << endl;
    }
}

// ============================================================================
// CLIENT HANDLER
// ============================================================================

void client_handler_thread(int client_socket, FIFOQueue* queue) {
    cout << "[CLIENT] Connected: socket " << client_socket << endl;
    
    char buffer[4096];
    
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytes = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        
        if (bytes <= 0) {
            cout << "[CLIENT] Disconnected: socket " << client_socket << endl;
            break;
        }
        
        buffer[bytes] = '\0';
        
        JSONRequest request = parse_json_request(string(buffer));
        queue->enqueue(QueuedOperation(request, client_socket));
    }
    
    close(client_socket);
}

// ============================================================================
// SERVER
// ============================================================================

class OFSServer {
private:
    int server_socket;
    int port;
    bool running;
    FIFOQueue queue;
    OperationProcessor* processor;
    thread processor_thread_obj;
    vector<thread> client_threads;

public:
    OFSServer(int port_num, const string& omni_path) 
        : server_socket(-1), port(port_num), running(false) {
        processor = new OperationProcessor(omni_path);
    }
    
    ~OFSServer() {
        stop();
        delete processor;
    }
    
    bool start() {
        server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket < 0) {
            cerr << "[ERROR] Cannot create socket" << endl;
            return false;
        }
        
        int opt = 1;
        setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        
        sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(port);
        
        if (::bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            cerr << "[ERROR] Cannot bind to port " << port << endl;
            close(server_socket);
            return false;
        }
        
        if (listen(server_socket, 20) < 0) {
            cerr << "[ERROR] Cannot listen" << endl;
            close(server_socket);
            return false;
        }
        
        running = true;
        processor_thread_obj = thread(fifo_processor_thread, &queue, processor);
        
        cout << "\n========================================" << endl;
        cout << "  OFS SERVER RUNNING" << endl;
        cout << "  Port: " << port << endl;
        cout << "========================================\n" << endl;
        
        while (running) {
            sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            
            int client_socket = accept(server_socket, (sockaddr*)&client_addr, &client_len);
            
            if (client_socket < 0) continue;
            
            client_threads.push_back(thread(client_handler_thread, client_socket, &queue));
        }
        
        return true;
    }
    
    void stop() {
        if (!running) return;
        running = false;
        queue.shutdown();
        
        if (server_socket >= 0) close(server_socket);
        if (processor_thread_obj.joinable()) processor_thread_obj.join();
        
        for (auto& t : client_threads) {
            if (t.joinable()) t.join();
        }
    }
};

// ============================================================================
// MAIN
// ============================================================================

int main() {
    string omni_path = "../compiled/test.omni";
    
    void* fs_instance = nullptr;
    if (fs_init(&fs_instance, omni_path.c_str(), nullptr) != SUCCESS) {
        cerr << "Failed to initialize file system" << endl;
        return 1;
    }
    
    OFSServer server(8080, omni_path);
    server.start();
    
    fs_shutdown(fs_instance);
    return 0;
}