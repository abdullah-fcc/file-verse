const net = require("net");
const express = require("express");
const cors = require("cors");

const TCP_HOST = "127.0.0.1";
const TCP_PORT = 8080;

const app = express();
app.use(cors());
app.use(express.json());

// LOGGING
console.log("Starting HTTP → TCP proxy...");

app.post("/", (req, res) => {
    console.log("\n=== Incoming HTTP Request ===");
    console.log(req.body);

    const client = new net.Socket();

    client.connect(TCP_PORT, TCP_HOST, () => {
        console.log("Connected to C++ server");
        const json = JSON.stringify(req.body);
        console.log("Sending:", json);
        client.write(json);
    });

    client.on("data", (data) => {
        console.log("=== C++ Server Response ===");
        console.log(data.toString());

        try {
            res.json(JSON.parse(data.toString()));
        } catch (err) {
            res.status(500).json({
                status: "error",
                error_message: "Invalid JSON from C++ server"
            });
        }

        client.destroy();
    });

    client.on("error", (err) => {
        console.log("TCP ERROR:", err);
        res.status(500).json({
            status: "error",
            error_message: "C++ Server unreachable"
        });
    });
});

app.listen(3000, () => {
    console.log("HTTP Proxy running on http://localhost:3000");
});
