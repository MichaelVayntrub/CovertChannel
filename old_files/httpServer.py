from http.server import SimpleHTTPRequestHandler, HTTPServer, socketserver

PORT = 12345

class HttpHandler(SimpleHTTPRequestHandler):
    def do_GET(self):
        # Log the requested page
        print(f"Page requested: {self.path}")

        # Route handling
        if self.path == '/' or self.path == '/index':
            self.path = 'index.html'
        elif self.path == '/a':
            self.path = 'a.html'
            print("A")
        elif self.path == '/b':
            self.path = 'b.html'
            print("B")
        else:
            self.send_error(404, "File Not Found")
            return

        return SimpleHTTPRequestHandler.do_GET(self)

def run_http_server():
    with socketserver.TCPServer(("", PORT), HttpHandler) as httpd:
        print(f"Serving at port {PORT}")
        httpd.serve_forever()
