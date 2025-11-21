/**
 * OFS Application - Main Application Logic
 * Handles UI interactions and coordinates with the HTTP gateway
 */

class OFSApp {
    constructor() {
        // Use HTTP gateway through Node proxy server at port 3000
        this.gateway = new OFSGatewayHTTP('localhost', 3000);

        this.currentUser = null;
        this.currentRole = null;
        this.currentPath = '/';
        this.currentFiles = [];

        this.init();
    }

    init() {
        this.setupEventListeners();
        this.showLoginScreen();
    }

    /**
     * Setup all event listeners
     */
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

            this.loadFiles(this.currentPath);

        } catch (error) {
            errorDiv.textContent = "Load failed";
        }
    }

    async handleLogout() {
        try {
            await this.gateway.logout();
        } catch (_) {}

        this.currentUser = null;
        this.currentRole = null;
        this.currentPath = '/';

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
            const response = await this.gateway.listDirectory(path);
            this.updateBreadcrumb(path);

            if (response.data?.files?.length > 0) {
                this.currentFiles = response.data.files;
                this.renderFiles(response.data.files);
            } else {
                fileList.innerHTML = '<div class="empty-state">Empty</div>';
            }

        } catch (err) {
            fileList.innerHTML = '<div class="empty-state">Failed to load</div>';
            this.showToast("Failed to load files", "error");
        }
    }

    renderFiles(files) {
        const fileList = document.getElementById('file-list');
        fileList.innerHTML = '';

        files.forEach(file => {
            const fileName = file.name || file;
            const isDirectory = file.type === 'directory' || fileName.endsWith('/');

            const item = document.createElement('div');
            item.className = 'file-item';

            item.innerHTML = `
                <div class="file-icon ${isDirectory ? 'folder' : 'file'}">
                    ${isDirectory ? '📁' : '📄'}
                </div>
                <div class="file-info">
                    <div class="file-name">${fileName}</div>
                    <div class="file-meta">${isDirectory ? 'Folder' : 'File'}</div>
                </div>
                <div class="file-actions">
                    ${!isDirectory ? `
                        <button class="file-action-btn" title="View">👁️</button>
                    ` : ""}
                    <button class="file-action-btn delete-btn" title="Delete">🗑️</button>
                </div>
            `;

            item.addEventListener('click', (e) => {
                if (!e.target.closest('.file-actions')) {
                    if (isDirectory)
                        this.loadFiles(`${this.currentPath}/${fileName}`);
                    else
                        this.viewFile(fileName);
                }
            });

            const viewBtn = item.querySelector('.file-action-btn:not(.delete-btn)');
            if (viewBtn) {
                viewBtn.addEventListener('click', (e) => {
                    e.stopPropagation();
                    this.viewFile(fileName);
                });
            }

            const delBtn = item.querySelector('.delete-btn');
            delBtn.addEventListener('click', (e) => {
                e.stopPropagation();
                if (isDirectory) this.deleteDirectory(fileName);
                else this.deleteFile(fileName);
            });

            fileList.appendChild(item);
        });
    }


    updateBreadcrumb(path) {
        const breadcrumb = document.querySelector('.breadcrumb');
        breadcrumb.innerHTML = `<span class="breadcrumb-item" data-path="/">/</span>`;

        const parts = path.split('/').filter(x => x);
        let build = '';

        parts.forEach(part => {
            build += '/' + part;
            const item = document.createElement('span');
            item.className = 'breadcrumb-item';
            item.textContent = ' / ' + part;
            item.dataset.path = build;
            item.addEventListener('click', () => this.loadFiles(build));
            breadcrumb.appendChild(item);
        });
    }

    async handleCreateFile() {
        const name = document.getElementById('file-name').value;
        const content = document.getElementById('file-content').value;

        const path = `${this.currentPath}/${name}`;

        try {
            await this.gateway.createFile(path, content);
            this.showToast("File created", "success");
            this.hideAllModals();
            this.loadFiles(this.currentPath);
        } catch {
            this.showToast("File creation failed", "error");
        }
    }

    async handleCreateDirectory() {
        const dir = document.getElementById('dir-name').value;
        const path = `${this.currentPath}/${dir}`;

        try {
            await this.gateway.createDirectory(path);
            this.showToast("Folder created", "success");
            this.hideAllModals();
            this.loadFiles(this.currentPath);
        } catch {
            this.showToast("Folder creation failed", "error");
        }
    }

    async viewFile(fileName) {
        const path = `${this.currentPath}/${fileName}`;

        try {
            const res = await this.gateway.readFile(path);

            document.getElementById('view-file-title').textContent = fileName;
            document.getElementById('view-file-content').value = res.data?.content || "";
            document.getElementById('delete-file-btn').dataset.file = path;

            this.showModal('view-file-modal');

        } catch {
            this.showToast("Failed to load file", "error");
        }
    }

    async deleteFile(fileName) {
        if (!confirm(`Delete file "${fileName}"?`)) return;

        const path = `${this.currentPath}/${fileName}`;

        try {
            await this.gateway.deleteFile(path);
            this.showToast("File deleted", "success");
            this.loadFiles(this.currentPath);
        } catch {
            this.showToast("Delete failed", "error");
        }
    }

    async deleteDirectory(dir) {
        if (!confirm(`Delete folder "${dir}"?`)) return;

        const path = `${this.currentPath}/${dir}`;

        try {
            await this.gateway.deleteDirectory(path);
            this.showToast("Folder deleted", "success");
            this.loadFiles(this.currentPath);
        } catch {
            this.showToast("Delete failed", "error");
        }
    }

    async handleDeleteCurrentFile() {
        const path = document.getElementById('delete-file-btn').dataset.file;

        try {
            await this.gateway.deleteFile(path);
            this.hideAllModals();
            this.loadFiles(this.currentPath);
        } catch {
            this.showToast("Delete failed", "error");
        }
    }


    // ===========================
    // USERS (ADMIN)
    // ===========================

    async loadUsers() {
        if (this.currentRole !== "admin") return;

        const userList = document.getElementById('user-list');
        userList.innerHTML = 'Loading...';

        try {
            const res = await this.gateway.listUsers();

            if (!res.data?.users?.length) {
                userList.innerHTML = 'No users';
                return;
            }

            this.renderUsers(res.data.users);

        } catch {
            userList.innerHTML = 'Error loading users';
        }
    }

    renderUsers(users) {
        const list = document.getElementById('user-list');
        list.innerHTML = '';

        users.forEach(u => {
            const name = u.username || u;

            const item = document.createElement('div');
            item.className = 'user-card';

            item.innerHTML = `
                <div class="user-header">
                    <div class="user-avatar">${name[0].toUpperCase()}</div>
                    <div class="user-name">${name}</div>
                </div>
            `;

            list.appendChild(item);
        });
    }

    async handleCreateUser() {
        const u = document.getElementById('new-username').value;
        const p = document.getElementById('new-password').value;
        const r = document.getElementById('user-role').value;

        try {
            await this.gateway.createUser(u, p, parseInt(r));
            this.showToast("User created", "success");
            this.hideAllModals();
            this.loadUsers();
        } catch {
            this.showToast("Failed to create user", "error");
        }
    }


    // ===========================
    // STATS
    // ===========================

    async loadStats() {
        try {
            const res = await this.gateway.getStats();

            document.getElementById('stat-total-size').textContent =
                `${(res.data.total_size / 1024 / 1024).toFixed(2)} MB`;

            document.getElementById('stat-used-space').textContent =
                `${(res.data.used_space / 1024 / 1024).toFixed(2)} MB`;

            document.getElementById('stat-total-files').textContent = res.data.total_files;
            document.getElementById('stat-total-dirs').textContent = res.data.total_directories;

        } catch {
            this.showToast("Failed to load stats", "error");
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

        setTimeout(() => toast.classList.remove('show'), 2500);
    }
}


// Start app when DOM is ready
document.addEventListener('DOMContentLoaded', () => {
    window.app = new OFSApp();
});
