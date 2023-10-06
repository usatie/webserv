import requests
import sys
import os

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

def test_get_request(path, status_code, content_type=None, content_length=None, file_path=None, content=None, location=None, host=None, headers=None):
    test_request(
            method='GET',
            path=path, 
            status_code=status_code,
            content_type=content_type,
            content_length=content_length,
            file_path=file_path,
            content=content,
            location=location,
            host=host,
            data=None,
            headers=headers
            )

def test_post_request(path, status_code, content_type=None, content_length=None, file_path=None, content=None, location=None, host=None, data=None, headers=None):
    test_request(
            method='POST',
            path=path, 
            status_code=status_code,
            content_type=content_type,
            content_length=content_length,
            file_path=file_path,
            content=content,
            location=location,
            host=host,
            data=data,
            headers=headers
            )

def test_request(method, path, status_code, content_type=None, content_length=None, file_path=None, content=None, location=None, host=None, data=None, headers=None):
    global err
    err = False
    assert(file_path is None or content is None) # Specifying both is not allowed
    global cnt
    cnt += 1
    # Print test name, cnt, path and host
    sys.stdout.write('Test {:02d}: {} {} '.format(cnt, method, path))
    if host is not None:
        sys.stdout.write('(Host: {}) '.format(host))
    # Print data (Trim if too long)
    if data is not None:
        if isinstance(data, bytes) and len(data) > 15:
            sys.stdout.write('(Body: {}...({} bytes)) '.format(data[:15], len(data)))
        else:
            sys.stdout.write('(Body: {}) '.format(data))
    sys.stdout.write(': ')
    sys.stdout.flush()
    if headers is None:
        headers = {}
    if host is not None:
        headers['Host'] = host
    if data is not None and content_length is None:
        if isinstance(data, str):
            headers['Content-Length'] = str(len(data))
    response = requests.request(method, url + path, headers=headers, data=data, allow_redirects=False)
    
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

# This is for chunked encoding test
def gen(message=None):
    if message is None:
        yield b'Hi, '
        yield b'there '
        yield b'bob'
        yield b'!'
    else:
        # yield message by chunk of 1000 bytes
        for i in range(0, len(message), 1000):
            yield message[i:i+1000]

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

    # Host header
    test_get_request(path='/', status_code=200, file_path='./tests/html/index.html', content_type='text/html', host=None)
    test_get_request(path='/', status_code=200, file_path='./tests/html/index.html', content_type='text/html', host='nosuchhost')
    test_get_request(path='/', status_code=200, file_path='./tests/html/index1.html', content_type='text/html', host='webserv1')
    test_get_request(path='/', status_code=200, file_path='./tests/html/index2.html', content_type='text/html', host='webserv2')
    test_get_request(path='/', status_code=200, file_path='./tests/html/index3.html', content_type='text/html', host='webserv3')
    test_get_request(path='/', status_code=200, file_path='./tests/html/index4.html', content_type='text/html', host='webserv4')
    test_get_request(path='/', status_code=200, file_path='./tests/html/index5.html', content_type='text/html', host='webserv5')

    # POST
    # test_post_request(path='/limit_except/post/', status_code=200, content_type='text/html', data=b'Hello, world!\n', content=b'Hello, world!\n') # TODO: Fix conf file
    test_post_request(path='/limit_except/get/', status_code=405, content_type='text/html', data=b'Hello, world!\n')
    test_post_request(path='/cgi/echo.py', status_code=200, content_type='text/plain', content=b'Hello, world!\n', data=b'Hello, world!\n')
    test_post_request(path='/cgi/echo.py', status_code=200, content_type='text/plain', content=b'42Tokyo', data=b'42Tokyo')
    test_post_request(path='/cgi/echo.py', status_code=200, content_type='text/plain', content=b'42Tokyo', data=b'42Tokyo')
    random_bytes = os.urandom(1000000)
    test_post_request(path='/cgi/echo.py', status_code=200, content_type='text/plain', content=random_bytes, data=random_bytes)
    test_post_request(path='/upload/', status_code=201, content_type='application/json', content=b'{"success":"true"}', data=b'Hello, world!\n') # TODO: Test Location /upload/1695790257-1622650073 (random)
    test_post_request(path='/upload/limit/', status_code=201, content_type='application/json', data=os.urandom(1024), content=b'{"success":"true"}')
    test_post_request(path='/upload/limit/', status_code=413, content_type='text/html', data=os.urandom(1025))

    # POST(Chunked)
    test_post_request(path='/cgi/echo.py', status_code=200, content_type='text/plain', content=b'Hi, there bob!', data=gen())
    test_post_request(path='/cgi/echo.py', status_code=200, content_type='text/plain', content=random_bytes, data=gen(random_bytes))

    # Server Error
    test_post_request(path='/cgi-invalid-handler/echo.py', status_code=500, content_type='text/html', data=b'Hello, world!\n')

    # GET (with query string)
    test_get_request(path='/?foo=bar', status_code=200, file_path='./tests/html/index.html', content_type='text/html')

    # GET (with fragment)
    # However, requests.request() will remove fragment in the actual HTTP request. So, we need another way to test this.
    # test_get_request(path='/#somefragment', status_code=200, file_path='./tests/html/index.html', content_type='text/html')
    # test_get_request(path='/?foo=bar#somefragment', status_code=200, file_path='./tests/html/index.html', content_type='text/html')

    # GET (with percent encoding)
    test_get_request(path='/alias/foo.html', status_code=200, file_path='./tests/html/foo/foo.html', content_type='text/html')
    test_get_request(path='/alias%2ffoo.html', status_code=200, file_path='./tests/html/foo/foo.html', content_type='text/html')

    # GET PATH_INFO
    test_get_request(path='/cgi/hello.py/foobar/path', status_code=200, content_type='text/plain', content=b'Hello, world!\n')

    # GET (with header name case insensitive)
    test_get_request(path='/', status_code=200, file_path='./tests/html/index1.html', content_type='text/html', headers={'Host': 'webserv1'})
    test_get_request(path='/', status_code=200, file_path='./tests/html/index1.html', content_type='text/html', headers={'host': 'webserv1'})
    test_get_request(path='/', status_code=200, file_path='./tests/html/index1.html', content_type='text/html', headers={'hOsT': 'webserv1'})

    # GET (with ../)
    # However, requests.request() will remove /../ in the actual HTTP request by resolving the relative path.
    # So, we need another way to test this.
    # test_get_request(path='/../', status_code=400, content_type='text/html')
    # test_get_request(path='/path/to/somewhere/../', status_code=400, content_type='text/html')
    # test_get_request(path='/path/to/somewhere/..', status_code=400, content_type='text/html')
    # GET (with .. but valid)
    test_get_request(path='/..path/to../..somewhere..', status_code=404, content_type='text/html')

    # Content-Length and Transfer-Encoding (Bad Request)
    test_post_request(path='/cgi/echo.py', status_code=400, content_type='text/html', data=gen(), headers={'Content-Length': '10', 'Transfer-Encoding': 'chunked'})

    ## Slow test
    ### GET (with query string)
    #test_get_request(path='/cgi/echo.py?foo=bar', status_code=200, content_type='text/plain', content=b'')
    ### Server Error
    #test_post_request(path='/cgi/infinite_loop.py', status_code=504, content_type='text/html', data=b'Hello, world!\n')
    #test_get_request(path='/cgi/infinite_loop.py', status_code=504, content_type='text/html')

    if err_cnt > 0:
        sys.stdout.write('\033[31m' + str(err_cnt) + '/' + str(cnt) + ' tests failed.\033[0m\n')
        sys.exit(1)
    else:
        sys.stdout.write('\033[32m' + str(cnt) + '/' + str(cnt) + ' tests passed.\033[0m\n')
        sys.exit(0)
