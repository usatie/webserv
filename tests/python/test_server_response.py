import requests
import sys

def test_get_request():
    server_url = 'http://localhost:8181'
    path = '/tests/static/a.text'

    response = requests.get(server_url + path)

    text_content = open('.' + path, 'rb').read()

    assert response.status_code == 200
    assert response.headers['Content-Type'] == 'text/plain'
    assert response.headers['Content-Length'] == str(len(text_content))

    assert response.text == text_content.decode()

if __name__ == '__main__':
    try:
        test_get_request()
        sys.exit(0)
    except AssertionError as e:
        # print(e)
        raise e
        sys.exit(1)
