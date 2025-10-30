# 프로세스 협력 파일 처리기



### 개요

- 클라이언트-서버 모델을 사용하여 파일의 각 줄 처리

- 클라이언트는 파일의 각 줄을 서버에 전송

- 서버는 지정된 처리 모드에 따라 각 줄을 처리한 후 결과를 클라이언트에 반환

- 처리 모드:

    - count: 문자 수 및 단어 수 계산

    - upper: 대문자로 변환

    - lower: 소문자로 변환

    - reverse: 문자열 뒤집기

- 사용법

```

./file_processor_clnt <input_file> <mode>

```

- <input_file>: 처리할 텍스트 파일의 경로

- <mode>: count, upper, lower, reverse 중 하나



## 실행결과

### 클라이언트: count

```

./file_processor_clnt test.txt count

  1번째 줄 전송...

  1번째 줄 결과 수신: Line 1: 11 chars, 2 words

  2번째 줄 전송...

  2번째 줄 결과 수신: Line 2: 26 chars, 4 words

  === 처리 통계 ===

  처리 모드: count

  처리한 줄 수: 10줄

  소요 시간: 0.05초

```

### 클라이언트: upper

```

./file_processor_clnt test.txt upper

  .

  . <이 부분은 실제 출력 결과로 대체>

  .

```



### 클라이언트: lower

```

./file_processor_clnt test.txt lower

  .

  . <이 부분은 실제 출력 결과로 대체>

  .

```



### 클라이언트: reverse

```

./file_processor_clnt test.txt reverse

  .

  . <이 부분은 실제 출력 결과로 대체>

  .

```



### 서버

```

  1번째 줄 처리 중...

  2번째 줄 처리 중...

```