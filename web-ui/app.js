/**
 * OFS Application - Path Normalization Fixed
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
        // Filter out empty strings and join with /
        const cleanParts = parts.filter(p => p && p.length > 0);
        const path = cleanParts.join('/');
        
        // Remove any double slashes
        const normalized = path.replace(/\/+/g, '/');
        
        // Remove leading slash if present
        return normalized.startsWith('/') ? normalized.substring(1) : normalized;
    }
    setupEventListeners() {
        // Login form
        document.getElementById('login-form').addEventListener('submit', (e) => {
            e.preventDefault();
            this.handleLogin();
        });
        // Logout button
        document.getElementById('logout-btn').addEventListener('click', () => {
            this.handleLogout();
        });
        // Navigation
        document.querySelectorAll('.nav-item').forEach(item => {
            item.addEventListener('click', (e) => {
                e.preventDefault();
                this.switchView(item.dataset.view);
            });
        });
        // Create File
        document.getElementById('new-file-btn').addEventListener('click', () => {
            this.showModal('file-modal');
        });
        document.getElementById('file-form').addEventListener('submit', (e) => {
            e.preventDefault();
            this.handleCreateFile();
        });
        // Create Directory
        document.getElementById('new-dir-btn').addEventListener('click', () => {
            this.showModal('dir-modal');
        });
        document.getElementById('dir-form').addEventListener('submit', (e) => {
            e.preventDefault();
            this.handleCreateDirectory();
        });
        // Create User (Admin Only)
        document.getElementById('new-user-btn').addEventListener('click', () => {
            this.showModal('user-modal');
        });
        document.getElementById('user-form').addEventListener('submit', (e) => {
            e.preventDefault();
            this.handleCreateUser();
        });
        // Stats refresh
        document.getElementById('refresh-stats-btn').addEventListener('click', () => {
            this.loadStats();
        });
        // File view modal
        document.getElementById('delete-file-btn').addEventListener('click', () => {
            this.handleDeleteCurrentFile();
        });
        // Modal close buttons
        document.querySelectorAll('.modal-close, .cancel-btn').forEach(btn => {
            btn.addEventListener('click', () => {
                this.hideAllModals();
            });
        });
        // Click outside modal to close
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
            this.currentRole = username === 'admin' ? 'admin' : 'user';
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
            
            // Extract just the filename (last part after /)
            const fileName = rawName.includes('/') ? rawName.split('/').pop() : rawName;
            
            const item = document.createElement('div');
            item.className = 'file-item';
            
            // Build proper full path from current path + filename
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
            // Main item click - open folder or view file
            item.addEventListener('click', (e) => {
                if (!e.target.closest('.file-actions')) {
                    if (isDirectory) {
                        this.loadFiles(fullPath);
                    } else {
                        this.viewFile(fullPath);
                    }
                }
            });
            // View button click
            const viewBtn = item.querySelector('.view-btn');
            if (viewBtn) {
                viewBtn.addEventListener('click', (e) => {
                    e.stopPropagation();
                    this.viewFile(fullPath);
                });
            }
            // Delete button click
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
        // Root item
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
            
            // Unescape the content (convert \\n to actual newlines, etc.)
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
            const passwordHash = u.password_hash || 'N/A';
            const item = document.createElement('div');
            item.className = 'user-card';
            item.innerHTML = `
                <div class="user-header">
                    <div class="user-avatar">${name[0].toUpperCase()}</div>
                    <div class="user-details">
                        <div class="user-name">${name}</div>
                        <div class="user-role-label">${role}</div>
                    </div>
                </div>
                <div class="user-info-hidden" style="display: none; margin-top: 10px; padding: 10px; background: #f5f5f5; border-radius: 5px;">
                    <div style="margin-bottom: 5px;"><strong>Username:</strong> ${name}</div>
                    <div style="margin-bottom: 5px;"><strong>Role:</strong> ${role}</div>
                    <div style="margin-bottom: 5px; word-break: break-all;"><strong>Password Hash:</strong> ${passwordHash}</div>
                </div>
            `;
            // Toggle details on click
            item.addEventListener('click', () => {
                const detailsDiv = item.querySelector('.user-info-hidden');
                if (detailsDiv.style.display === 'none') {
                    detailsDiv.style.display = 'block';
                } else {
                    detailsDiv.style.display = 'none';
                }
            });
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