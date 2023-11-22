#!/usr/bin/env python3

import time
import os

def print_headers():
    print("Content-Type: text/html")
    print()

def main():
    if os.environ.get('REQUEST_METHOD') == 'GET':
        print_headers()
        print(f"<html><body><h1>Current Time: {time.time()}</h1></body></html>")
    else:
        print_headers()
        print("<html><body><h1>Only GET method is allowed.</h1></body></html>")

if __name__ == "__main__":
    main()
