#!/usr/bin/env python3

import http.server

class HttpRecorder(http.server.SimpleHTTPRequestHandler):
    def do_GET(self):
        print('GET request for {}\n'.format(self.path))

        # And log this action.
        with open('/tmp/http_recorder.log', 'a') as f:
            f.write('GET request for {}\n'.format(self.path))

        http.server.SimpleHTTPRequestHandler.do_GET(self)


def run(server_class=http.server.HTTPServer, handler_class=HttpRecorder):
    server_address = ('', 8080)
    httpd = server_class(server_address, handler_class)
    httpd.serve_forever()

run()