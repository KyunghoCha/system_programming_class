## 💻 1. 클라이언트 ($\text{file_processor_clnt.c}$) 필수 메서드

| 분류 | 역할 (Purpose) | 함수명 (Function Signature)                                                | 설명 (Description) |
| :--- | :--- |:------------------------------------------------------------------------| :--- |
| **I/O 무결성** | 읽기 보장 | `ssize_t read_all(int fd, void *buf, size_t count)`                     | 요청한 **모든 바이트**를 읽을 때까지 `read()` 반복. 서버 응답 수신에 사용. |
| | 쓰기 보장 | `ssize_t write_all(int fd, const void *buf, size_t count)`              | 요청한 **모든 바이트**를 쓸 때까지 `write()` 반복. 서버로 요청 전송에 사용. |
| **프로토콜** | 메시지 전송 (패킹) | `int send_message(int fd, MessageHeader *header, const char *data, size_t len)` | 메시지 **길이 헤더**와 **본문**을 `write_all`로 순차 전송. |
| | 메시지 수신 (언패킹) | `char* receive_message(int fd, MessageHeader *header, size_t *out_len)`         | `read_all`로 헤더와 본문을 정확히 읽어 서버 응답 수신. |
| **파일 $\text{I/O}$ & $\text{SPLIT}$** | **줄 읽기** | `char *read_line(int fd, size_t *out_len)`                              | **잔여 데이터 관리** 로직을 포함하여, 파일 $\text{fd}$에서 데이터를 읽어 **한 줄씩 분리**하여 반환. **(Split 로직 포함)** |
| **메인 로직** | 통계 출력 | `void print_stats(const Stats *stats)`                               | **`StatData` 구조체 포인터**를 받아 처리 통계 형식을 출력. |
| **공통** | 오류 보고 | `void handle_error(const char *msg)`                                    | 오류 발생 시 메시지를 출력하고 프로그램을 종료. |
| | 메인 함수 | `int main(int argc, char *argv[])`                                      | 명령줄 처리, $\text{FIFO}$ $\text{I/O}$ 관리, `read_line`을 사용하여 입력 파일의 각 줄을 서버에 전송. |

---

## 💻 2. 서버 ($\text{file_processor_svr.c}$) 필수 메서드

| 분류 | 역할 (Purpose) | 함수명 (Function Signature) | 설명 (Description) |
| :--- | :--- | :--- | :--- |
| **I/O 공통** | 읽기 보장 | `ssize_t read_all(int fd, void *buf, size_t count)` | 요청한 **모든 바이트**를 읽을 때까지 `read()` 반복. 클라이언트 요청 수신에 사용. |
| | 쓰기 보장 | `ssize_t write_all(int fd, const void *buf, size_t count)` | 요청한 **모든 바이트**를 쓸 때까지 `write()` 반복. 클라이언트에게 결과 전송에 사용. |
| **프로토콜** | 메시지 수신 (언패킹) | `char* receive_message(int fd, MessageHeader *header, size_t *out_len)` | `read_all`로 헤더와 본문을 정확히 읽어 클라이언트 요청 수신. |
| | 메시지 전송 (패킹) | `int send_message(int fd, const MessageHeader *header, const char *data, size_t len)` | 메시지 **길이 헤더**와 **본문**을 `write_all`로 순차 전송. |
| **처리 로직** | 명령어 처리 | `char* process_line(const char *line, const char *mode)` | 수신된 `mode`에 따라 하위 처리 함수를 호출하는 **핵심 분기** 함수. |
| | 단어 수 계산 | `char* count_words(const char *str)` | 문자 수와 단어 수를 계산하여 결과 문자열을 생성. |
| | 대문자 변환 | `char* to_upper(const char *str)` | 문자열의 모든 소문자를 대문자로 변환하여 반환. |
| | 소문자 변환 | `char* to_lower(const char *str)` | 문자열의 모든 대문자를 소문자로 변환하여 반환. |
| | 문자열 역순 | `char* reverse_string(const char *str)` | 문자열의 문자를 역순으로 배열하여 반환. |
| **공통** | 오류 보고 | `void handle_error(const char *msg)` | 오류 발생 시 메시지를 출력하고 프로그램을 종료. |
| | 메인 함수 | `int main()` | $\text{FIFO}$ 생성/열기, `receive_message`로 요청을 받아 `process_line`을 실행하는 **무한 루프**. |