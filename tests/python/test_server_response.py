import requests
import sys

def test_get_request():
    server_url = 'http://localhost:8181'
    file_path = '/tests/static/a.text'

    response = requests.get(server_url + file_path)

    # using 'rb' mode to handle binary data including cases with \r\n and \n characters.
    text_content = open('.' + file_path, 'rb').read()

    assert response.status_code == 200
    assert response.headers['Content-Type'] == 'text/plain'
    assert response.headers['Content-Length'] == str(len(text_content))

    # ensure that the response encoding matches the apparent encoding, especially for non-ASCII characters.
    response.encoding = response.apparent_encoding
    assert response.text == text_content.decode()

if __name__ == '__main__':
    try:
        test_get_request()
        sys.exit(0)
    except AssertionError as e:
        raise e
