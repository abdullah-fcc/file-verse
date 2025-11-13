#pragma once
#include <iostream>
#include <string>
#include <vector>
#include "odf_types.hpp"
using namespace std;


struct UserNode {
    string username;
    UserInfo info;
    bool active;  
};


class UserHashTable {
private:
    vector<UserNode> users;  
    int max_users;
    
    int hash(const string& username) {
        int sum = 0;
        for (char c : username) {
            sum += c;  
        }
        return sum % max_users;
    }
    
public:

    UserHashTable(int size = 100) {
        max_users = size;
        users.resize(max_users);
        

        for (int i = 0; i < max_users; i++) {
            users[i].active = false;
        }
    }
    
    void insert(const string& username, const UserInfo& info) {
        int index = hash(username);
        int start = index;
        
        while (true) {
            if (!users[index].active || users[index].username == username) {
                users[index].username = username;
                users[index].info = info;
                users[index].active = true;
                return;
            }
            
            index = (index + 1) % max_users;
            
            if (index == start) {
                cout << "ERROR: Hash table is full!" << endl;
                return;
            }
        }
    }
    
    UserInfo* get(const string& username) {
        int index = hash(username);
        int start = index;
        
        while (users[index].active) {
            if (users[index].username == username) {
                return &users[index].info;
            }
            
            index = (index + 1) % max_users;
            
            // Searched full circle
            if (index == start) break;
        }
        
        return nullptr;
    }
    
    bool remove(const string& username) {
        int index = hash(username);
        int start = index;
        
        while (users[index].active) {
            if (users[index].username == username) {
                users[index].active = false;
                users[index].username = "";
                return true;
            }
            
            index = (index + 1) % max_users;
            
            if (index == start) break;
        }
        
        return false;
    }
    
    vector<UserInfo> getAllUsers() {
        vector<UserInfo> result;
        
        for (int i = 0; i < max_users; i++) {
            if (users[i].active) {
                result.push_back(users[i].info);
            }
        }
        
        return result;
    }
    
    int count() {
        int total = 0;
        for (int i = 0; i < max_users; i++) {
            if (users[i].active) {
                total++;
            }
        }
        return total;
    }
    

    void print() {
        cout << "\n=== USERS IN HASH TABLE ===" << endl;
        cout << "Total users: " << count() << endl;
        
        for (int i = 0; i < max_users; i++) {
            if (users[i].active) {
                cout << "[" << i << "] " << users[i].username << endl;
            }
        }
        cout << "===========================\n" << endl;
    }
};

