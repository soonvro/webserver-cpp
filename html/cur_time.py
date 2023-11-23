#!/usr/bin/env python3

import time
import os

def print_headers():
    print("Content-Type: text/html")
    print()

def main():
    print("<html><body>")
    if os.environ.get('REQUEST_METHOD') == 'GET':
        print_headers()
        tm = time.localtime(time.time())
        print(f"<h1 style=\"text-align:center\">Current Time: {tm.tm_year}.{tm.tm_mon}.{tm.tm_mday}.{tm.tm_hour}.{tm.tm_min}.{tm.tm_sec}</h1>")
    else:
        print_headers()
        print("<h1>Only GET method is allowed.</h1>")
    print("<a href=\"/index.pl\">back to index</a>")
    print("</body></html>")

if __name__ == "__main__":
    main()
