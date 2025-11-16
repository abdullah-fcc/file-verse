// client.cpp - Comprehensive OFS Test Client
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

using namespace std;

class OFSClient {
private:
    int sock;
    bool connected;

public:
    OFSClient() : sock(-1), connected(false) {}
    
    ~OFSClient() { disconnect(); }
    
    bool connect_to_server(const string& host, int port) {
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            cerr << "Error: Cannot create socket" << endl;
            return false;
        }
        
        sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        inet_pton(AF_INET, host.c_str(), &server_addr.sin_addr);
        
        if (connect(sock, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            cerr << "Error: Cannot connect to server" << endl;
            return false;
        }
        
        connected = true;
        cout << "✓ Connected to " << host << ":" << port << endl;
        return true;
    }
    
    void disconnect() {
        if (connected) {
            close(sock);
            connected = false;
        }
    }
    
    string send_request(const string& json_request) {
        if (!connected) return "{\"status\":\"error\"}";
        
        send(sock, json_request.c_str(), json_request.length(), 0);
        
        char buffer[4096];
        int bytes = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) return "{\"status\":\"error\"}";
        
        buffer[bytes] = '\0';
        return string(buffer);
    }
    
    // User operations
    string user_login(const string& username, const string& password) {
        string json = "{\"operation\":\"user_login\",\"session_id\":\"\",";
        json += "\"request_id\":\"1\",\"parameters\":{";
        json += "\"username\":\"" + username + "\",";
        json += "\"password\":\"" + password + "\"}}";
        return send_request(json);
    }
    
    string user_create(const string& username, const string& password) {
        string json = "{\"operation\":\"user_create\",\"session_id\":\"\",";
        json += "\"request_id\":\"2\",\"parameters\":{";
        json += "\"username\":\"" + username + "\",";
        json += "\"password\":\"" + password + "\"}}";
        return send_request(json);
    }
    
    // File operations
    string file_create(const string& path, const string& data) {
        string json = "{\"operation\":\"file_create\",\"session_id\":\"s1\",";
        json += "\"request_id\":\"3\",\"parameters\":{";
        json += "\"path\":\"" + path + "\",";
        json += "\"data\":\"" + data + "\"}}";
        return send_request(json);
    }
    
    string file_read(const string& path) {
        string json = "{\"operation\":\"file_read\",\"session_id\":\"s1\",";
        json += "\"request_id\":\"4\",\"parameters\":{";
        json += "\"path\":\"" + path + "\"}}";
        return send_request(json);
    }
    
    string file_delete(const string& path) {
        string json = "{\"operation\":\"file_delete\",\"session_id\":\"s1\",";
        json += "\"request_id\":\"5\",\"parameters\":{";
        json += "\"path\":\"" + path + "\"}}";
        return send_request(json);
    }
    
    string file_exists(const string& path) {
        string json = "{\"operation\":\"file_exists\",\"session_id\":\"s1\",";
        json += "\"request_id\":\"6\",\"parameters\":{";
        json += "\"path\":\"" + path + "\"}}";
        return send_request(json);
    }
    
    string file_rename(const string& old_path, const string& new_path) {
        string json = "{\"operation\":\"file_rename\",\"session_id\":\"s1\",";
        json += "\"request_id\":\"7\",\"parameters\":{";
        json += "\"old_path\":\"" + old_path + "\",";
        json += "\"new_path\":\"" + new_path + "\"}}";
        return send_request(json);
    }
    
    // Directory operations
    string dir_create(const string& path) {
        string json = "{\"operation\":\"dir_create\",\"session_id\":\"s1\",";
        json += "\"request_id\":\"8\",\"parameters\":{";
        json += "\"path\":\"" + path + "\"}}";
        return send_request(json);
    }
    
    string dir_list(const string& path) {
        string json = "{\"operation\":\"dir_list\",\"session_id\":\"s1\",";
        json += "\"request_id\":\"9\",\"parameters\":{";
        json += "\"path\":\"" + path + "\"}}";
        return send_request(json);
    }
    
    string dir_delete(const string& path) {
        string json = "{\"operation\":\"dir_delete\",\"session_id\":\"s1\",";
        json += "\"request_id\":\"10\",\"parameters\":{";
        json += "\"path\":\"" + path + "\"}}";
        return send_request(json);
    }
    
    string dir_exists(const string& path) {
        string json = "{\"operation\":\"dir_exists\",\"session_id\":\"s1\",";
        json += "\"request_id\":\"11\",\"parameters\":{";
        json += "\"path\":\"" + path + "\"}}";
        return send_request(json);
    }
    
    // Info operations
    string get_stats() {
        string json = "{\"operation\":\"get_stats\",\"session_id\":\"s1\",";
        json += "\"request_id\":\"12\",\"parameters\":{}}";
        return send_request(json);
    }
    
    string get_metadata(const string& path) {
        string json = "{\"operation\":\"get_metadata\",\"session_id\":\"s1\",";
        json += "\"request_id\":\"13\",\"parameters\":{";
        json += "\"path\":\"" + path + "\"}}";
        return send_request(json);
    }
};

int main() {
    cout << "\n================================================" << endl;
    cout << "     OFS COMPREHENSIVE SYSTEM TEST" << endl;
    cout << "================================================\n" << endl;
    
    OFSClient client;
    
    if (!client.connect_to_server("127.0.0.1", 8080)) {
        return 1;
    }
    
    cout << "\n--- User Operations ---\n" << endl;
    cout << "1. Login as admin..." << endl;
    cout << client.user_login("admin", "admin123") << "\n" << endl;
    
    cout << "\n--- File Operations ---\n" << endl;
    cout << "2. Create file..." << endl;
    cout << client.file_create("/test.txt", "Hello OFS!") << "\n" << endl;
    
    cout << "3. Check if file exists..." << endl;
    cout << client.file_exists("/test.txt") << "\n" << endl;
    
    cout << "4. Read file..." << endl;
    cout << client.file_read("/test.txt") << "\n" << endl;
    
    cout << "5. Rename file..." << endl;
    cout << client.file_rename("/test.txt", "/renamed.txt") << "\n" << endl;
    
    cout << "6. Get file metadata..." << endl;
    cout << client.get_metadata("/renamed.txt") << "\n" << endl;
    
    cout << "\n--- Directory Operations ---\n" << endl;
    cout << "7. Create directory..." << endl;
    cout << client.dir_create("/documents") << "\n" << endl;
    
    cout << "8. Check if directory exists..." << endl;
    cout << client.dir_exists("/documents") << "\n" << endl;
    
    cout << "9. Create file in directory..." << endl;
    cout << client.file_create("/documents/note.txt", "Notes here") << "\n" << endl;
    
    cout << "10. List directory..." << endl;
    cout << client.dir_list("/documents") << "\n" << endl;
    
    cout << "\n--- System Statistics ---\n" << endl;
    cout << "11. Get file system stats..." << endl;
    cout << client.get_stats() << "\n" << endl;
    
    cout << "\n--- Cleanup ---\n" << endl;
    cout << "12. Delete file..." << endl;
    cout << client.file_delete("/renamed.txt") << "\n" << endl;
    
    cout << "================================================" << endl;
    cout << "     ALL TESTS COMPLETED!" << endl;
    cout << "================================================\n" << endl;
    
    return 0;
}