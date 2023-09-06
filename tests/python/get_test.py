import socket

def test_get_request():
    server_host = 'localhost'
    server_port = 8181
    
    request = 'GET /tests/static/a.text HTTP/1.1\r\n\r\n'
    
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((server_host, server_port))
        s.sendall(request.encode())
        
        response = s.recv(4096).decode()
        
        assert 'HTTP/1.1 200 OK' in response
        assert 'Content-Type: text/plain' in response
        assert 'aaaaa' in response


if __name__ == '__main__':
    test_get_request()
