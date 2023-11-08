#!/usr/bin/perl

print "Content-type: text/html\n\n";
print "<html><head></head><body>";
print "서버이름: $ENV{'SERVER_NAME'}<br />";
print "서버 포트: $ENV{'SERVER_PORT'}<br />";
print "프로토콜: $ENV{'SERVER_PROTOCOL'}<br />";
print "사용자 웹브라우져: $ENV{'HTTP_USER_AGENT'}<br />";
print "사용자 IP 주소: $ENV{'REMOTE_ADDR'}";
print "</body></html>";