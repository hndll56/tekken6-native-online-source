import http.server
import os
from pathlib import Path
import ssl
import subprocess
import sys
import tempfile
import threading

class Handler(http.server.BaseHTTPRequestHandler):
    def log_message(self, *args): pass
    def do_GET(self):
        if self.path == '/redirect':
            self.send_response(302)
            self.send_header('Location', '/ok')
            self.end_headers()
            return
        body = b'x' * (2 << 20) if self.path == '/large' else b'hello'
        self.send_response(200)
        self.send_header('Content-Length', str(len(body)))
        self.end_headers()
        try: self.wfile.write(body)
        except (BrokenPipeError, ConnectionResetError): pass
    def do_POST(self):
        self.rfile.read(int(self.headers.get('Content-Length', '0')))
        self.send_response(500)
        self.end_headers()
        self.wfile.write(b'soap-error')

with tempfile.TemporaryDirectory() as temp:
    cert, key = Path(temp)/'cert.pem', Path(temp)/'key.pem'
    subprocess.run(['openssl', 'req', '-x509', '-newkey', 'rsa:2048', '-nodes', '-keyout', str(key), '-out', str(cert), '-days', '1', '-subj', '/CN=localhost', '-addext', 'subjectAltName=DNS:localhost'], check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    plain = http.server.ThreadingHTTPServer(('127.0.0.1', 0), Handler)
    tls = http.server.ThreadingHTTPServer(('127.0.0.1', 0), Handler)
    context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    context.load_cert_chain(cert, key)
    tls.socket = context.wrap_socket(tls.socket, server_side=True)
    for server in [plain, tls]:
        threading.Thread(target=server.serve_forever, daemon=True).start()
    try:
        result = subprocess.run([sys.argv[1], f'http://127.0.0.1:{plain.server_port}', f'https://localhost:{tls.server_port}/ok'], env={**os.environ, 'T6_TEST_CA': str(cert)}, timeout=15)
        assert result.returncode == 0, result.returncode
    finally:
        for server in [plain, tls]: server.shutdown(); server.server_close()
