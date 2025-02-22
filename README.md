<a id="readme-top"></a>

<!-- PROJECT SHIELDS -->
[![Contributors][contributors-shield]][contributors-url]
[![Forks][forks-shield]][forks-url]
[![Stargazers][stars-shield]][stars-url]
[![Issues][issues-shield]][issues-url]
[![Unlicense License][license-shield]][license-url]
<br /><br />


<!-- PROJECT LOGO & NAME -->
<div align="center">
  <img alt="Logo" src="https://i.imgur.com/8OjBVG3.png" width="350" height="40">
</div>


<!-- HEADLINE -->
<h1 align="center">
  C++98 I/O Multiplexing 웹 서버
</h1>


<!-- SHORT DESCRIPTION -->
<h4 align="center">
  GET, POST, DELETE 요청, 다중 클라이언트 병렬 처리, 정적 파일 제공과 같은 기본적인 HTTP/1.1 및 NGINX Configure 문법을 지원합니다.
</h4>
<br />


<!-- SHOWCASE -->
<!--
<div align="center">
  <img alt="Showcase" src="https://cdn.pixabay.com/animation/2022/10/11/23/03/23-03-06-809_512.gif" width="500" height="500">
</div>
<br />
-->


<!-- TABLE OF CONTENTS -->
<details>
  <summary><b>Table of Contents</b></summary>
  <ul>
    <li><a href="#주요-기능">주요 기능</a></li>
    <li><a href="#빠른-시작">빠른 시작</a></li>
    <li><a href="#사용-예시">사용 예시</a></li>
    <li><a href="#webserver-cpp를-만든-이유">Webserver Cpp를 만든 이유</a></li>
    <li><a href="#기술-스택">기술 스택</a></li>
    <li><a href="#라이선스">라이선스</a></li>
    <li><a href="#acknowledgments">Acknowledgments</a></li>
  </ul>
</details>


# 주요 기능

- **RFC 규격 준수 HTTP Parser**:
    - 자체 구현한 유한 상태 오토마타 Parser
    - HTTP/1.1 규격 지원
    - **Zero-Copy Parsing**
- **이벤트 기반 I/O 멀티플렉싱**:
    - 효율적인 이벤트 관리를 위한 kqueue 사용
    - 논블로킹 I/O 작업
    - 비동기 요청/응답 처리
    - 동시 연결 처리 지원
- **HTTP/1.1 프로토콜 지원**  
- **동적 설정 파일**: Nginx의 Config 문법을 사용한 설정 파일 시스템
- **CGI 지원**: Perl, Python CGI 스크립트 처리 및 효율적인 프로세스 관리
- **다중 서버 블록**: 여러 포트에서 다중 가상 호스트 구성 지원
- **고급 요청 처리**:
    - GET, POST, DELETE 메소드 지원
    - 청크 전송 인코딩 (Chunked Transfer Encoding)
    - 클라이언트 요청 본문 크기 제한
    - 사용자 정의 오류 페이지
- **디렉토리 목록 자동 생성**: Autoindex 기능 (활성화/비활성화 설정 가능)
- **URL 라우팅**: 경로 기반 Location 블록 시스템

<p align="right"><a href="#readme-top">▲ back to top</a></p>


# 빠른 시작

### 요구 사항
- C++ compiler with C++98 support
- Make
- Python3 (for CGI support)
- Perl (for CGI support)

### 설치 및 실행

1. Clone the repository
```sh
git clone https://github.com/yourusername/webserv.git
```

2. Build the project
```sh
make
```

3. Run the server
```sh
./webserv [config_file]
```

설정 파일을 명시하지 않으면, 기본 설정 파일인 `webserv.config`이 적용됩니다.  

<p align="right"><a href="#readme-top">▲ back to top</a></p>


# 사용 예시

### 서버 시작
```bash
# 기본 설정 파일로 서버 시작
./webserv

# 사용자 정의 설정 파일로 서버 시작
./webserv your_config.conf
```

### 서버 기능 테스트
1. **기본 HTTP 요청 테스트**

```bash
# GET request
curl http://localhost:8080/

# POST request
curl -X POST -d "data=test" http://localhost:8080/

# DELETE request
curl -X DELETE http://localhost:8080/file.txt
```

2. **파일 업로드 테스트**

```bash
# POST 요청으로 파일 업로드
curl -X POST -F "file=@./test.txt" http://localhost:8080/upload.pl

# 업로드한 파일 확인
curl http://localhost:8080/uploaded_list.pl
```

3. **CGI 스크립트 테스트**

```bash
# Perl CGI script 실행
curl http://localhost:8080/hello.pl

# Python CGI script 실행
curl http://localhost:8080/cur_time.py
```
  
4. **Autoindex 테스트 (설정에서 활성화된 경우)**
```bash
curl http://localhost:8080/YoupiBanane/
```


### 설정 파일 예시

1. **단일 호스트**

```nginx
http {
    server {
        listen 8080;
        server_name localhost;
        root html/;
        location / {
            index index.html;
            autoindex on;
        }
    }
}
```

2. **다중 가상 호스트**

```nginx
http {
    server {
        listen 8080;
        server_name site1.local;
        root html/site1/;
    }
    server {
        listen 8081;
        server_name site2.local;
        root html/site2/;
    }
}
```

3. **CGI 설정**

```nginx
http {
    server {
        listen 8080;
        location ~ \.pl$ {
            root html/;
            cgi_root /usr/bin/perl;
        }
        location ~ \.py$ {
            root html/;
            cgi_root /usr/bin/python3;
        }
    }
}
```

### 모니터링

```bash
# 포트 확인
lsof -i :8080

# 서버 로그 확인
tail -f /var/log/webserv.log

# 서버 프로세스 확인
ps aux | grep webserv
```

<p align="right"><a href="#readme-top">▲ back to top</a></p>


# Webserver Cpp를 만든 이유

단순히 웹 서버를 사용하는 것을 넘어, C++로 웹 서버를 직접 구현하여 동작 방식을 이해하기 위해 시작한 프로젝트입니다. 덕분에 평소에 블랙박스처럼 추상적이었던 웹 서버의 내부 구조를 명확히 이해하고, 필요하다면 분석 및 수정까지 가능한 대상이라고 인식을 바꿀 수 있었습니다.  

- HTTP/1.1 프로토콜부터 소켓 연결과 동적 콘텐츠 생성에 이르기까지, 웹 요청이 처리되는 모든 과정을 구현하여 웹 개발의 기본 원리를 탄탄하게 다질 수 있었습니다.  
- **I/O 멀티플렉싱 기법**을 적용하여 현대 서버 환경에서의 효율적인 동시 접속 처리 방안을 구현하고 성능을 검증했습니다. 멀티프로세싱 및 멀티쓰레딩 방식과 비교하여 I/O 멀티플렉싱의 장점과 활발하게 사용되는 이유를 확인하였습니다.  
- **CGI**(Common Gateway Interface) 핸들러를 구현하는 과정을 통해 웹 서버가 어떻게 프로세스 격리를 사용하여 동적 콘텐츠를 안전하게 실행하는 지 깨달았습니다.  

<p align="right"><a href="#readme-top">▲ back to top</a></p>


# 기술 스택

Language:  
- ![cpp][cpp-badge]  

<p align="right"><a href="#readme-top">▲ back to top</a></p>


# 라이선스

Unlicense를 준수합니다. 자세한 내용은 `LICENSE`에서 확인할 수 있습니다.

<p align="right"><a href="#readme-top">▲ back to top</a></p>


# Acknowledgments
[nginx](https://nginx.org/)  

<p align="right"><a href="#readme-top">▲ back to top</a></p>



<!-- MARKDOWN LINKS & IMAGES -->
[contributors-shield]: https://img.shields.io/github/contributors/soonvro/webserver-cpp.svg?style=for-the-badge
[contributors-url]: https://github.com/soonvro/webserver-cpp/graphs/contributors
[forks-shield]: https://img.shields.io/github/forks/soonvro/webserver-cpp.svg?style=for-the-badge
[forks-url]: https://github.com/soonvro/webserver-cpp/network/members
[stars-shield]: https://img.shields.io/github/stars/soonvro/webserver-cpp.svg?style=for-the-badge
[stars-url]: https://github.com/soonvro/webserver-cpp/stargazers
[issues-shield]: https://img.shields.io/github/issues/soonvro/webserver-cpp.svg?style=for-the-badge
[issues-url]: https://github.com/soonvro/webserver-cpp/issues
[license-shield]: https://img.shields.io/github/license/soonvro/webserver-cpp.svg?style=for-the-badge
[license-url]: https://github.com/soonvro/webserver-cpp/blob/master/LICENSE.txt

[cpp-badge]: https://img.shields.io/badge/c++-%2300599C.svg?style=for-the-badge&logo=c%2B%2B&logoColor=white

