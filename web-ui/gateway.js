class OFSGatewayHTTP {
    constructor(host = 'localhost', port = 3000) {
        this.baseUrl = `http://${host}:${port}`;
        this.requestId = 0;
        this.sessionId = null;
    }

    async sendRequest(operation, parameters = {}) {
        const requestId = `req_${++this.requestId}_${Date.now()}`;

        const request = {
            operation,
            session_id: this.sessionId || "",
            request_id: requestId,
            parameters
        };

        console.log("[Gateway → Proxy] Sending:", request);

        const response = await fetch(this.baseUrl, {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify(request)
        });

        const data = await response.json();

        console.log("[Proxy → Gateway] Response:", data);

        if (data.status === "success") return data;
        throw new Error(data.error_message || "Unknown error");
    }

    async login(username, password) {
        const res = await this.sendRequest("user_login", { username, password });

        if (res.data && res.data.session_id)
            this.sessionId = res.data.session_id;

        return res;
    }

    async logout() {
        const r = await this.sendRequest("user_logout", {});
        this.sessionId = null;
        return r;
    }

    async createUser(username, password, role) {
        return this.sendRequest("user_create", { username, password, role });
    }

    async listUsers() {
        return this.sendRequest("user_list", {});
    }

    async createFile(path, content) {
        return this.sendRequest("file_create", { path, data: content });
    }

    async readFile(path) {
        return this.sendRequest("file_read", { path });
    }

    async deleteFile(path) {
        return this.sendRequest("file_delete", { path });
    }

    async createDirectory(path) {
        return this.sendRequest("dir_create", { path });
    }

    async listDirectory(path) {
        return this.sendRequest("dir_list", { path });
    }

    async deleteDirectory(path) {
        return this.sendRequest("dir_delete", { path });
    }

    async getStats() {
        return this.sendRequest("get_stats", {});
    }

    async getMetadata(path) {
        return this.sendRequest("get_metadata", { path });
    }
}

window.OFSGatewayHTTP = OFSGatewayHTTP;
