import requests
import sys

url = 'http://localhost:8181'
cnt = 0

def assertEqual(actual, expected, itemName=None):
    if actual != expected:
        sys.stderr.write('\033[31mNG\033[0m\n')
        if itemName is not None:
            sys.stderr.write(itemName + ': ')
        sys.stderr.write('Expected {}, but got {}.\n'.format(expected, actual))
        sys.exit(1)

def test_get_request(path, expected_status_code, expected_file_path, expected_content_type):
    global cnt
    cnt += 1
    sys.stdout.write('Test ' + str(cnt) + ': ' + path + ' ')
    sys.stdout.flush()
    response = requests.get(url + path, allow_redirects=False)
    
    assertEqual(response.status_code, expected_status_code, 'status_code')
    if expected_content_type is not None:
        assertEqual(response.headers['Content-Type'], expected_content_type, 'Content-Type')
    else:
        assertEqual(response.headers.get('Content-Type'), None, 'Content-Type')

    # using 'rb' mode to handle binary data including cases with \r\n and \n characters.
    if expected_file_path is not None:
        expected_content = open(expected_file_path, 'rb').read()
        assertEqual(response.headers['Content-Length'], str(len(expected_content)), 'Content-Length')
        assertEqual(response.content, expected_content, 'content')
    else:
        if response.headers.get('Content-Length') is not None:
            assertEqual(response.headers['Content-Length'], '0', 'Content-Length')
        assertEqual(response.content, b'', 'content')

    # OK with green color
    print('\033[32mOK\033[0m')

if __name__ == '__main__':
    test_get_request('/tests/static/a.text', 200, './tests/static/a.text', 'text/plain')
    test_get_request('/tests/static/b.text', 200, './tests/static/b.text', 'text/plain')
    test_get_request('/tests/static/abe.html', 200, './tests/static/abe.html', 'text/html')
    test_get_request('/tests/static/abe_files/abehiroshi.jpg', 200, './tests/static/abe_files/abehiroshi.jpg', 'image/jpeg')
    test_get_request('/tests/static/abe_files/menu.html', 200, './tests/static/abe_files/menu.html', 'text/html')
    test_get_request('/', 200, './tests/html/index.html', 'text/html')
    test_get_request('/cgi/echo.py', 200, None, 'text/plain')
    # test_get_request('/cgi/hello.py', 200, 'hello.out', 'text/plain') # TODO: hello.out is not created
    test_get_request('/root/', 200, './tests/html/root/root.html', 'text/html')
    test_get_request('/root/root.html', 200, './tests/html/root/root.html', 'text/html')
    test_get_request('/alias/', 200, './tests/html/foo/index.html', 'text/html')
    test_get_request('/alias/foo.html', 200, './tests/html/foo/foo.html', 'text/html')
    test_get_request('/index/', 200, './tests/html/foo/foo.html', 'text/html')
    test_get_request('/kapouet/pouic/toto/pouet', 200, '/tmp/www/pouic/toto/pouet', 'text/plain')
    test_get_request('/multiple-index/', 200, './tests/html/foo/foo.html', 'text/html')
    test_get_request('/error_page/nosuchfile', 404, '/tmp/www/404.html', 'text/html')
    # test_get_request('/limit_except/post/', 405, './tests/html/405.html', 'text/html') # TODO: 405.html is not created
    test_get_request('/limit_except/get/', 200, './tests/html/foo/index.html', 'text/html')
    test_get_request('/redirect/url/', 301, None, None) #TODO: Test Location
    test_get_request('/redirect/path/', 301, None, None) #TODO: Test Location
    # test_get_request('/cgi-bin/a.out.cgi', 200, 'a.out.cgi.out', 'text/html') # TODO: a.out.cgi.out is not created
    # test_get_request('/autoindex/', 200, './tests/html/autoindex/', 'text/html') # TODO : autoindex is not created
    
    sys.exit(0)
