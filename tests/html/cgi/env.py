#!/usr/bin/env python3

import sys
import os


def main():
    # Print Sample CGI response

    print("Content-Type: text/plain")
    print("Status: 200 OK")
    print()
    print(*[f"{k}={v}" for k, v in os.environ.items()], sep='\n')
    print()
    # read from stdin, the web server will pass the data to the CGI script
    # read and write only 10 bytes per loop
    while True:
        chunk = sys.stdin.read(10)
        if not chunk:
            break
        sys.stdout.write(chunk)


if __name__ == "__main__":
    main()
