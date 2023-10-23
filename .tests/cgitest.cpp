#include <fcntl.h>
#include <unistd.h>

#include <iostream>
#include <string>

int main(int argc, char **argv, char **envp) {
  argv = new char *[100];
  argv[0] = "/usr/bin/php";
  argv[1] = "/Users/inskim/inskim/5circle/Webserv/cgi.php";
  argv[3] = NULL;
  envp = new char *[100];
  envp[0] = "GATEWAY_INTERFACE=CGI/1.1";
  envp[1] =
      "PATH_INFO=/Users/inskim/inskim/5circle/Webserv/"
      "cgi.php";
  envp[2] = "QUERY_STRING=name=inskim";
  envp[3] = "REMOTE_ADDR=127.0.0.1";
  envp[4] = "REQUEST_METHOD=GET";
  envp[5] = "SCRIPT_NAME=/Users/inskim/inskim/5circle/Webserv/cgi.php";
  envp[6] = "SERVER_NAME=inskim";
  envp[7] = "SERVER_PORT=8080";
  envp[8] = "SERVER_PROTOCOL=HTTP/1.1";
  envp[9] = "SERVER_SOFTWARE=Webserv/1.0";
  envp[10] = "AUTH_TYPE=";
  envp[11] = "CONTENT_LENGTH=";
  envp[12] = "CONTENT_TYPE=text/html";
  envp[13] = "argv=name=subsub";
  envp[14] = "argc=2";

  execve("/usr/bin/php", argv, envp);
}
