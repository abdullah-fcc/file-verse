#include <fstream>
#include <sstream>
#include <iostream>
#include <unordered_map>

using namespace std;

struct Config {
    struct Filesystem {
        size_t total_size;
        size_t header_size;
        size_t block_size;
        int max_files;
        int max_filename_length;
    } filesystem;

    struct Security {
        int max_users;
        string admin_username;
        string admin_password;
        bool require_auth;
    } security;

    struct Server {
        int port;
        int max_connections;
        int queue_timeout;
    } server;

    bool load(const string& filename) {
        ifstream config_file(filename);
        if (!config_file) {
            cerr << "Error: Could not open config file: " << filename << endl;
            return false;
        }

        string line;
        while (getline(config_file, line)) {
            if (line.empty() || line[0] == '#') continue; 
            
            istringstream iss(line);
            string key, value;
            if (getline(iss, key, '=') && getline(iss, value)) {
                key = key.substr(key.find_first_not_of(" \t"));
                value = value.substr(value.find_first_not_of(" \t"));
                
                // Parse each section of the config
                if (key == "total_size") 
                    filesystem.total_size = stoi(value);
                else if (key == "header_size") 
                    filesystem.header_size = stoi(value);
                else if (key == "block_size")
                    filesystem.block_size = stoi(value);
                else if (key == "max_files") 
                    filesystem.max_files = stoi(value);
                else if (key == "max_filename_length") 
                    filesystem.max_filename_length = stoi(value);
                else if (key == "max_users") 
                    security.max_users = stoi(value);
                else if (key == "admin_username") 
                    security.admin_username = value;
                else if (key == "admin_password") 
                    security.admin_password = value;
                else if (key == "require_auth") 
                    security.require_auth = (value == "true");
                else if (key == "port") 
                    server.port = stoi(value);
                else if (key == "max_connections") 
                    server.max_connections = stoi(value);
                else if (key == "queue_timeout") 
                    server.queue_timeout = stoi(value);
            }
        }
        
        return true;
    }
};
