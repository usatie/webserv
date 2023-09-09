import requests
import sys

def test_get_request():
    server_url = 'http://localhost:8181'

    response = requests.get(server_url + '/tests/static/a.text')

    assert response.status_code == 200
    assert response.headers['Content-Type'] == 'text/plain'


if __name__ == '__main__':
    try:
        test_get_request()
        sys.exit(0)
    except AssertionError:
        sys.exit(1)
