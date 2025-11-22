/**
 * OFS Application - Complete with RBAC and all features
 */
class OFSApp {
    constructor() {
        this.gateway = new OFSGatewayHTTP('localhost', 3000);
        this.currentUser = null;
        this.currentRole = null;
        this.currentPath = '';
        this.currentFiles = [];
        this.init();
    }

    init() {
        this.setupEventListeners();
        this.showLoginScreen();
    }

    /**
     * Helper function to build proper paths
     */
    buildPath(...parts) {
        const cleanParts = parts.filter(p => p && p.length > 0);
        const path = cleanParts.join('/');
        const normalized = path.replace(/\/+/g, '/');
        return normalized.startsWith('/') ? normalized.substring(1) : normalized;
    }

    setupEventListeners() {
        document.getElementById('login-form').addEventListener('submit', (e) => {
            e.preventDefault();
            this.handleLogin();
        });

        document.getElementById('logout-btn').addEventListener('click', () => {
            this.handleLogout();
        });

        document.querySelectorAll('.nav-item').forEach(item => {
            item.addEventListener('click', (e) => {
                e.preventDefault();
                this.switchView(item.dataset.view);
            });
        });

        document.getElementById('new-file-btn').addEventListener('click', () => {
            this.showModal('file-modal');
        });

        document.getElementById('file-form').addEventListener('submit', (e) => {
            e.preventDefault();
            this.handleCreateFile();
        });

        document.getElementById('new-dir-btn').addEventListener('click', () => {
            this.showModal('dir-modal');
        });

        document.getElementById('dir-form').addEventListener('submit', (e) => {
            e.preventDefault();
            this.handleCreateDirectory();
        });

        document.getElementById('new-user-btn').addEventListener('click', () => {
            this.showModal('user-modal');
        });

        document.getElementById('user-form').addEventListener('submit', (e) => {
            e.preventDefault();
            this.handleCreateUser();
        });

        document.getElementById('refresh-stats-btn').addEventListener('click', () => {
            this.loadStats();
        });

        document.getElementById('delete-file-btn').addEventListener('click', () => {
            this.handleDeleteCurrentFile();
        });

        document.querySelectorAll('.modal-close, .cancel-btn').forEach(btn => {
            btn.addEventListener('click', () => {
                this.hideAllModals();
            });
        });

        document.querySelectorAll('.modal').forEach(modal => {
            modal.addEventListener('click', (e) => {
                if (e.target === modal) {
                    this.hideAllModals();
                }
            });
        });
    }

    // ===========================
    // AUTHENTICATION
    // ===========================

    async handleLogin() {
        const username = document.getElementById('username').value;
        const password = document.getElementById('password').value;
        const errorDiv = document.getElementById('login-error');

        try {
            errorDiv.textContent = 'Connecting...';
            const response = await this.gateway.login(username, password);

            this.currentUser = username;
            
            // Fetch user list to determine actual role
            try {
                const usersResponse = await this.gateway.listUsers();
                const currentUserData = usersResponse.data.users.find(u => u.username === username);
                if (currentUserData) {
                    this.currentRole = currentUserData.role === 1 ? 'admin' : 'user';
                } else {
                    this.currentRole = username === 'admin' ? 'admin' : 'user';
                }
            } catch {
                this.currentRole = username === 'admin' ? 'admin' : 'user';
            }

            this.showAppScreen();
            this.showToast('Login successful!', 'success');
            this.currentPath = '';
            this.loadFiles(this.currentPath);

        } catch (error) {
            errorDiv.textContent = error.message || "Login failed";
        }
    }

    async handleLogout() {
        try {
            await this.gateway.logout();
        } catch (_) {}

        this.currentUser = null;
        this.currentRole = null;
        this.currentPath = '';

        this.showLoginScreen();
        this.showToast('Logged out successfully', 'success');
    }

    // ===========================
    // SCREEN MANAGEMENT
    // ===========================

    showLoginScreen() {
        document.getElementById('login-screen').classList.add('active');
        document.getElementById('app-screen').classList.remove('active');
        document.getElementById('login-form').reset();
        document.getElementById('login-error').textContent = '';
    }

    showAppScreen() {
        document.getElementById('login-screen').classList.remove('active');
        document.getElementById('app-screen').classList.add('active');

        document.getElementById('current-user').textContent = this.currentUser;

        const badge = document.getElementById('user-role');
        badge.textContent = this.currentRole;
        badge.className = `role-badge ${this.currentRole}`;

        document.getElementById('users-nav').style.display =
            this.currentRole === 'admin' ? 'flex' : 'none';
    }

    // ===========================
    // VIEW SWITCHING
    // ===========================

    switchView(viewName) {
        document.querySelectorAll('.nav-item').forEach(i => i.classList.remove('active'));
        document.querySelector(`[data-view="${viewName}"]`).classList.add('active');

        document.querySelectorAll('.view').forEach(v => v.classList.remove('active'));
        document.getElementById(`${viewName}-view`).classList.add('active');

        if (viewName === 'files') this.loadFiles(this.currentPath);
        if (viewName === 'users') this.loadUsers();
        if (viewName === 'stats') this.loadStats();
    }

    // ===========================
    // FILE HANDLING
    // ===========================

    async loadFiles(path) {
        this.currentPath = path;

        const fileList = document.getElementById('file-list');
        fileList.innerHTML = '<div class="empty-state">Loading...</div>';

        try {
            console.log("[loadFiles] Requesting path:", path);
            const response = await this.gateway.listDirectory(path);
            this.updateBreadcrumb(path);

            console.log("[loadFiles] Response:", response);

            if (response.data?.files?.length > 0) {
                this.currentFiles = response.data.files;
                this.renderFiles(response.data.files);
            } else {
                fileList.innerHTML = '<div class="empty-state">This folder is empty</div>';
            }

        } catch (err) {
            console.error("[loadFiles] Error:", err);
            fileList.innerHTML = '<div class="empty-state">Failed to load files</div>';
            this.showToast("Failed to load files: " + err.message, "error");
        }
    }

    renderFiles(files) {
        const fileList = document.getElementById('file-list');
        fileList.innerHTML = '';

        files.forEach(file => {
            const rawName = file.name || file;
            const isDirectory = file.type === 'directory';
            
            const fileName = rawName.includes('/') ? rawName.split('/').pop() : rawName;
            
            const item = document.createElement('div');
            item.className = 'file-item';
            
            const fullPath = this.buildPath(this.currentPath, fileName);
            
            item.dataset.fullPath = fullPath;
            item.dataset.fileName = fileName;
            item.dataset.isDirectory = isDirectory;

            console.log(`[renderFiles] Raw: ${rawName}, FileName: ${fileName}, Full path: ${fullPath}`);

            item.innerHTML = `
                <div class="file-icon ${isDirectory ? 'folder' : 'file'}">
                    ${isDirectory ? '📁' : '📄'}
                </div>
                <div class="file-info">
                    <div class="file-name">${fileName}</div>
                    <div class="file-meta">${isDirectory ? 'Folder' : 'File'} ${!isDirectory && file.size ? `- ${file.size} bytes` : ''}</div>
                </div>
                <div class="file-actions">
                    ${!isDirectory ? `
                        <button class="file-action-btn view-btn" title="View">👁️</button>
                    ` : ""}
                    <button class="file-action-btn delete-btn" title="Delete">🗑️</button>
                </div>
            `;

            item.addEventListener('click', (e) => {
                if (!e.target.closest('.file-actions')) {
                    if (isDirectory) {
                        this.loadFiles(fullPath);
                    } else {
                        this.viewFile(fullPath);
                    }
                }
            });

            const viewBtn = item.querySelector('.view-btn');
            if (viewBtn) {
                viewBtn.addEventListener('click', (e) => {
                    e.stopPropagation();
                    this.viewFile(fullPath);
                });
            }

            const delBtn = item.querySelector('.delete-btn');
            delBtn.addEventListener('click', (e) => {
                e.stopPropagation();
                if (isDirectory) {
                    this.deleteDirectory(fullPath, fileName);
                } else {
                    this.deleteFile(fullPath, fileName);
                }
            });

            fileList.appendChild(item);
        });
    }

    updateBreadcrumb(path) {
        const breadcrumb = document.querySelector('.breadcrumb');
        breadcrumb.innerHTML = '';

        const rootItem = document.createElement('span');
        rootItem.className = 'breadcrumb-item';
        rootItem.textContent = 'Root';
        rootItem.dataset.path = '';
        rootItem.addEventListener('click', () => this.loadFiles(''));
        breadcrumb.appendChild(rootItem);

        if (!path) return;

        const parts = path.split('/').filter(x => x);
        let build = '';

        parts.forEach((part, index) => {
            build = this.buildPath(build, part);
            
            const separator = document.createElement('span');
            separator.textContent = ' / ';
            breadcrumb.appendChild(separator);
            
            const item = document.createElement('span');
            item.className = 'breadcrumb-item';
            item.textContent = part;
            item.dataset.path = build;
            item.addEventListener('click', () => this.loadFiles(build));
            breadcrumb.appendChild(item);
        });
    }

    async handleCreateFile() {
        const name = document.getElementById('file-name').value;
        const content = document.getElementById('file-content').value;
        const path = this.buildPath(this.currentPath, name);

        console.log("[handleCreateFile] Creating file at path:", path);

        try {
            await this.gateway.createFile(path, content);
            this.showToast("File created successfully", "success");
            this.hideAllModals();
            document.getElementById('file-form').reset();
            
            setTimeout(() => this.loadFiles(this.currentPath), 200);
        } catch (err) {
            console.error("[handleCreateFile] Error:", err);
            this.showToast("File creation failed: " + err.message, "error");
        }
    }

    async handleCreateDirectory() {
        const dir = document.getElementById('dir-name').value;
        const path = this.buildPath(this.currentPath, dir);

        console.log("[handleCreateDirectory] Creating folder at path:", path);

        try {
            await this.gateway.createDirectory(path);
            this.showToast("Folder created successfully", "success");
            this.hideAllModals();
            document.getElementById('dir-form').reset();
            
            setTimeout(() => this.loadFiles(this.currentPath), 200);
        } catch (err) {
            console.error("[handleCreateDirectory] Error:", err);
            this.showToast("Folder creation failed: " + err.message, "error");
        }
    }

    async viewFile(filePath) {
        console.log("[viewFile] Reading file at path:", filePath);

        try {
            const res = await this.gateway.readFile(filePath);
            console.log("[viewFile] File response:", res);

            const fileName = filePath.split('/').pop();
            
            let content = res.data?.content || "";
            content = content
                .replace(/\\n/g, '\n')
                .replace(/\\t/g, '\t')
                .replace(/\\r/g, '\r')
                .replace(/\\"/g, '"')
                .replace(/\\\\/g, '\\');
            
            document.getElementById('view-file-title').textContent = fileName;
            document.getElementById('view-file-content').value = content;
            document.getElementById('delete-file-btn').dataset.filePath = filePath;

            this.showModal('view-file-modal');

        } catch (err) {
            console.error("[viewFile] Error:", err);
            this.showToast("Failed to load file: " + err.message, "error");
        }
    }

    async deleteFile(filePath, fileName) {
        if (!confirm(`Delete file "${fileName}"?`)) return;

        console.log("[deleteFile] Deleting file at path:", filePath);

        try {
            await this.gateway.deleteFile(filePath);
            this.showToast("File deleted successfully", "success");
            
            setTimeout(() => this.loadFiles(this.currentPath), 200);
        } catch (err) {
            console.error("[deleteFile] Error:", err);
            this.showToast("Delete failed: " + err.message, "error");
        }
    }

    async deleteDirectory(dirPath, dirName) {
        if (!confirm(`Delete folder "${dirName}"?`)) return;

        console.log("[deleteDirectory] Deleting folder at path:", dirPath);

        try {
            await this.gateway.deleteDirectory(dirPath);
            this.showToast("Folder deleted successfully", "success");
            
            setTimeout(() => this.loadFiles(this.currentPath), 200);
        } catch (err) {
            console.error("[deleteDirectory] Error:", err);
            this.showToast("Delete failed: " + err.message, "error");
        }
    }

    async handleDeleteCurrentFile() {
        const filePath = document.getElementById('delete-file-btn').dataset.filePath;

        console.log("[handleDeleteCurrentFile] Deleting file at path:", filePath);

        try {
            await this.gateway.deleteFile(filePath);
            this.hideAllModals();
            this.showToast("File deleted successfully", "success");
            
            setTimeout(() => this.loadFiles(this.currentPath), 200);
        } catch (err) {
            console.error("[handleDeleteCurrentFile] Error:", err);
            this.showToast("Delete failed: " + err.message, "error");
        }
    }

    // ===========================
    // USERS (ADMIN)
    // ===========================

    async loadUsers() {
        if (this.currentRole !== "admin") {
            document.getElementById('user-list').innerHTML = 
                '<div class="empty-state">Admin access required</div>';
            return;
        }

        const userList = document.getElementById('user-list');
        userList.innerHTML = '<div class="empty-state">Loading users...</div>';

        try {
            const res = await this.gateway.listUsers();
            console.log("[loadUsers] Users response:", res);

            if (!res.data?.users?.length) {
                userList.innerHTML = '<div class="empty-state">No users found</div>';
                return;
            }

            this.renderUsers(res.data.users);

        } catch (err) {
            console.error("[loadUsers] Error:", err);
            userList.innerHTML = '<div class="empty-state">Error loading users: ' + err.message + '</div>';
            this.showToast("Failed to load users: " + err.message, "error");
        }
    }

    renderUsers(users) {
        const list = document.getElementById('user-list');
        list.innerHTML = '';

        users.forEach(u => {
            const name = u.username || u;
            const role = u.role === 1 ? 'Admin' : 'User';
            const actualPassword = u.password || 'N/A';
            const passwordHash = u.password_hash || 'N/A';
            const isAdmin = u.role === 1;

            const item = document.createElement('div');
            item.className = 'user-card';

            item.innerHTML = `
                <div class="user-header">
                    <div class="user-avatar">${name[0].toUpperCase()}</div>
                    <div class="user-details">
                        <div class="user-name">${name}</div>
                        <div class="user-role-label">${role}</div>
                    </div>
                    ${name !== 'admin' && this.currentRole === 'admin' ? `
                        <button class="user-delete-btn" title="Delete User" style="margin-left: auto; padding: 8px 12px; background: #ef4444; color: white; border: none; border-radius: 5px; cursor: pointer;">
                            🗑️ Delete
                        </button>
                    ` : ''}
                </div>
                <div class="user-info-hidden" style="display: none; margin-top: 10px; padding: 10px; background: #f5f5f5; border-radius: 5px;">
                    <div style="margin-bottom: 5px;"><strong>Username:</strong> ${name}</div>
                    <div style="margin-bottom: 5px;"><strong>Role:</strong> ${role}</div>
                    ${this.currentRole === 'admin' ? `
                        <div style="margin-bottom: 5px;"><strong>Password:</strong> <span style="color: #059669; font-weight: 600;">${actualPassword}</span></div>
                        <div style="margin-bottom: 5px; word-break: break-all; font-size: 0.85em; color: #666;"><strong>Hash:</strong> ${passwordHash}</div>
                    ` : ''}
                </div>
            `;

            const header = item.querySelector('.user-header');
            header.addEventListener('click', (e) => {
                if (!e.target.closest('.user-delete-btn')) {
                    const detailsDiv = item.querySelector('.user-info-hidden');
                    if (detailsDiv.style.display === 'none') {
                        detailsDiv.style.display = 'block';
                    } else {
                        detailsDiv.style.display = 'none';
                    }
                }
            });

            const deleteBtn = item.querySelector('.user-delete-btn');
            if (deleteBtn) {
                deleteBtn.addEventListener('click', (e) => {
                    e.stopPropagation();
                    this.handleDeleteUser(name);
                });
            }

            list.appendChild(item);
        });
    }

    async handleCreateUser() {
        const u = document.getElementById('new-username').value;
        const p = document.getElementById('new-password').value;
        const r = document.getElementById('user-role').value;

        console.log("[handleCreateUser] Creating user:", u, "with role:", r);

        try {
            await this.gateway.createUser(u, p, parseInt(r));
            this.showToast("User created successfully", "success");
            this.hideAllModals();
            document.getElementById('user-form').reset();
            this.loadUsers();
        } catch (err) {
            console.error("[handleCreateUser] Error:", err);
            this.showToast("Failed to create user: " + err.message, "error");
        }
    }

    async handleDeleteUser(username) {
        if (!confirm(`Are you sure you want to delete user "${username}"?\n\nThis action cannot be undone.`)) {
            return;
        }

        console.log("[handleDeleteUser] Deleting user:", username);

        try {
            await this.gateway.deleteUser(username);
            this.showToast("User deleted successfully", "success");
            this.loadUsers();
        } catch (err) {
            console.error("[handleDeleteUser] Error:", err);
            this.showToast("Failed to delete user: " + err.message, "error");
        }
    }

    // ===========================
    // STATS
    // ===========================

    async loadStats() {
        try {
            const res = await this.gateway.getStats();
            console.log("[loadStats] Stats response:", res);

            document.getElementById('stat-total-size').textContent =
                `${(res.data.total_size / 1024 / 1024).toFixed(2)} MB`;

            document.getElementById('stat-used-space').textContent =
                `${(res.data.used_space / 1024 / 1024).toFixed(2)} MB`;

            document.getElementById('stat-total-files').textContent = 
                res.data.total_files || 0;
                
            document.getElementById('stat-total-dirs').textContent = 
                res.data.total_directories || 0;

        } catch (err) {
            console.error("[loadStats] Error:", err);
            this.showToast("Failed to load stats: " + err.message, "error");
        }
    }

    // ===========================
    // MODALS + TOAST
    // ===========================

    showModal(id) {
        document.getElementById(id).classList.add('active');
    }

    hideAllModals() {
        document.querySelectorAll('.modal').forEach(m => m.classList.remove('active'));
    }

    showToast(msg, type = 'success') {
        const toast = document.getElementById('toast');
        toast.textContent = msg;
        toast.className = `toast ${type} show`;

        setTimeout(() => toast.classList.remove('show'), 3000);
    }
}

// Start app when DOM is ready
document.addEventListener('DOMContentLoaded', () => {
    window.app = new OFSApp();
});