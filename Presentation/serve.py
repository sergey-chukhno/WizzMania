#!/usr/bin/env python3
"""
Tiny presentation server that serves static files AND
can launch the WizzMania clients via a /launch endpoint.

Usage:
    cd WizzMania
    python3 Presentation/serve.py

Then open http://localhost:5050 in your browser.
"""

import http.server
import os
import subprocess
import sys

PORT = 5050
PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PRESENTATION_DIR = os.path.join(PROJECT_ROOT, "Presentation")
LAUNCH_SCRIPT = os.path.join(PROJECT_ROOT, "launch_multiclient.sh")


class PresentationHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=PRESENTATION_DIR, **kwargs)

    def do_POST(self):
        if self.path == "/launch":
            try:
                subprocess.Popen(
                    ["bash", LAUNCH_SCRIPT],
                    cwd=PROJECT_ROOT,
                    stdout=subprocess.DEVNULL,
                    stderr=subprocess.DEVNULL,
                )
                self.send_response(200)
                self.send_header("Content-Type", "application/json")
                self.send_header("Access-Control-Allow-Origin", "*")
                self.end_headers()
                self.wfile.write(b'{"status":"ok"}')
                print(f"[serve] Launched: {LAUNCH_SCRIPT}")
            except Exception as e:
                self.send_response(500)
                self.send_header("Content-Type", "application/json")
                self.end_headers()
                self.wfile.write(f'{{"error":"{e}"}}'.encode())
        else:
            self.send_response(404)
            self.end_headers()

    def do_OPTIONS(self):
        self.send_response(200)
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "POST")
        self.end_headers()


if __name__ == "__main__":
    print(f"[serve] Project root:  {PROJECT_ROOT}")
    print(f"[serve] Serving from:  {PRESENTATION_DIR}")
    print(f"[serve] Launch script: {LAUNCH_SCRIPT}")
    print(f"[serve] Open http://localhost:{PORT}")
    print()
    with http.server.HTTPServer(("", PORT), PresentationHandler) as httpd:
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\n[serve] Stopped.")
            sys.exit(0)
