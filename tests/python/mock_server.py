import requests

def test_get_request():
    server_url = 'http://localhost:8181'

    response = requests.get(server_url + '/tests/static/a.text')

    assert response.status_code == 200
    print(response.headers)
    assert response.headers['Content-Type'] == 'text/plain'

if __name__ == '__main__':
    test_get_request()
