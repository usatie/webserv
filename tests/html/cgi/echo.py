#!/usr/bin/env python3
import sys
def main():
    # Print Sample CGI response

    print("Content-Type: text/plain")
    print("Status: 200 OK")
    print()
    # read from stdin, the web server will pass the data to the CGI script
    print(sys.stdin.read())

if __name__ == "__main__":
    main()
