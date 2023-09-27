import requests
import sys

url = 'http://localhost:8181'
cnt = 0
err_cnt = 0
err = False

def assertEqual(actual, expected, itemName=None):
    global err
    if actual != expected:
        if not err:
            sys.stdout.write('\033[31mNG\033[0m\n')
        if itemName is not None:
            sys.stderr.write(itemName + ': ')
        sys.stderr.write('Expected {}, but got {}.\n'.format(expected, actual))
        err = True

def assertHeader(response, headerName, expected):
    actual = response.headers.get(headerName)
    assertEqual(actual, expected, headerName)

def assertContent(response, expected):
    actual = response.content
    if expected is None:
        assertEqual(actual, b'', 'content')
    else:
        assertEqual(actual, expected, 'content')

def test_get_request(path, status_code, content_type=None, content_length=None, file_path=None, content=None, location=None):
    global err
    err = False
    assert(file_path is None or content is None) # Specifying both is not allowed
    global cnt
    cnt += 1
    sys.stdout.write('Test ' + str(cnt) + ': ' + path + ' ')
    sys.stdout.flush()
    response = requests.get(url + path, allow_redirects=False)
    
    # Status Code
    assertEqual(response.status_code, status_code, 'status_code')
    # Content-Type
    if content_type is not None:
        assertHeader(response, 'Content-Type', content_type)
    # Content-Length
    if content_length is not None:
        assertHeader(response, 'Content-Length', str(content_length))
    # Location
    if location is not None:
        assertHeader(response, 'Location', location)

    # Content
    if file_path is not None:
        content = open(file_path, 'rb').read()

    if content is not None:
        # Content-Length again
        assertHeader(response, 'Content-Length', str(len(content)))
        # Content
        assertEqual(response.content, content, 'content')

    # OK with green color
    if err:
        global err_cnt
        err_cnt += 1
    else:
        print('\033[32mOK\033[0m')

if __name__ == '__main__':
    test_get_request(path='/tests/static/a.text', status_code=200, file_path='./tests/static/a.text', content_type='text/plain')
    test_get_request(path='/tests/static/b.text', status_code=200, file_path='./tests/static/b.text', content_type='text/plain')
    test_get_request(path='/tests/static/abe.html', status_code=200, file_path='./tests/static/abe.html', content_type='text/html')
    test_get_request(path='/tests/static/abe_files/abehiroshi.jpg', status_code=200, file_path='./tests/static/abe_files/abehiroshi.jpg', content_type='image/jpeg')
    test_get_request(path='/tests/static/abe_files/menu.html', status_code=200, file_path='./tests/static/abe_files/menu.html', content_type='text/html')
    test_get_request(path='/', status_code=200, file_path='./tests/html/index.html', content_type='text/html')
    test_get_request(path='/cgi/echo.py', status_code=200, content_type=None, content_length=0)
    test_get_request(path='/cgi/hello.py', status_code=200, content_type='text/plain', content=b'Hello, world!\n')
    test_get_request(path='/root/', status_code=200, file_path='./tests/html/root/root.html', content_type='text/html')
    test_get_request(path='/root/root.html', status_code=200, file_path='./tests/html/root/root.html', content_type='text/html')
    test_get_request(path='/alias/', status_code=200, file_path='./tests/html/foo/index.html', content_type='text/html')
    test_get_request(path='/alias/foo.html', status_code=200, file_path='./tests/html/foo/foo.html', content_type='text/html')
    test_get_request(path='/index/', status_code=200, file_path='./tests/html/foo/foo.html', content_type='text/html')
    test_get_request(path='/kapouet/pouic/toto/pouet', status_code=200, file_path='/tmp/www/pouic/toto/pouet', content_type='text/plain')
    test_get_request(path='/multiple-index/', status_code=200, file_path='./tests/html/foo/foo.html', content_type='text/html')
    test_get_request(path='/error_page/nosuchfile', status_code=404, file_path='/tmp/www/404.html', content_type='text/html')
    test_get_request(path='/limit_except/post/', status_code=405, content_type='text/html')
    test_get_request(path='/limit_except/get/', status_code=200, file_path='./tests/html/foo/index.html', content_type='text/html')
    test_get_request(path='/redirect/url/', status_code=301, location='https://www.google.com')
    test_get_request(path='/redirect/path/', status_code=301, location='/kapouet/pouic/toto/pouet')
    test_get_request(path='/cgi-bin/a.out.cgi', status_code=200, content_type='text/html', content=b'<html><head><title>CGI</title></head><body>\n<h1>CGI</h1>\n<p>CGI is a standard for interfacing external applications with information servers, such as HTTP or web servers.</p>\n</body></html>\n')
    test_get_request(path='/autoindex/', status_code=200, content_type='text/html')
    if err_cnt > 0:
        sys.stdout.write('\033[31m' + str(err_cnt) + '/' + str(cnt) + ' tests failed.\033[0m\n')
        sys.exit(1)
    else:
        sys.stdout.write('\033[32m' + str(cnt) + '/' + str(cnt) + ' tests passed.\033[0m\n')
        sys.exit(0)
