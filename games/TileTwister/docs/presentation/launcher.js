const http = require('http');
const fs = require('fs');
const path = require('path');
const { spawn } = require('child_process');

const PORT = 8080;
// Absolute path relative to where this script is located (docs/presentation)
// Adjust if running from root, but let's assume running from docs/presentation or we fix paths.
// Let's make it robust.
const PROJECT_ROOT = path.resolve(__dirname, '../../');
const GAME_EXECUTABLE = path.join(PROJECT_ROOT, 'build', 'TileTwister');

const MIME_TYPES = {
    '.html': 'text/html',
    '.css': 'text/css',
    '.js': 'text/javascript',
    '.png': 'image/png',
    '.jpg': 'image/jpeg'
};

const server = http.createServer((req, res) => {
    console.log(`Request: ${req.url}`);

    // API Endpoint: Launch Game
    if (req.url === '/launch') {
        console.log('Launching game...');

        // Spawn the game process detached so it doesn't block
        const subprocess = spawn(GAME_EXECUTABLE, [], {
            detached: true,
            stdio: 'ignore',
            cwd: PROJECT_ROOT // Run in project root so it finds "assets/" folder
        });

        subprocess.unref();

        res.writeHead(200, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify({ status: 'launched' }));
        return;
    }

    // Static File Serving
    let filePath = path.join(__dirname, req.url === '/' ? 'index.html' : req.url);
    const ext = path.extname(filePath);
    const contentType = MIME_TYPES[ext] || 'application/octet-stream';

    fs.readFile(filePath, (err, content) => {
        if (err) {
            if (err.code === 'ENOENT') {
                res.writeHead(404);
                res.end('File not found');
            } else {
                res.writeHead(500);
                res.end(`Server Error: ${err.code}`);
            }
        } else {
            res.writeHead(200, { 'Content-Type': contentType });
            res.end(content);
        }
    });
});

server.listen(PORT, () => {
    console.log(`Presentation Server running at http://localhost:${PORT}/`);
    console.log(`Game executable target: ${GAME_EXECUTABLE}`);
});
