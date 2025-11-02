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
  1번째 줄 결과 수신: 44 chars, 9 words
  2번째 줄 전송...
  2번째 줄 결과 수신: 12 chars, 2 words
  3번째 줄 전송...
  3번째 줄 결과 수신: 9 chars, 2 words
  4번째 줄 전송...
  4번째 줄 결과 수신: 9 chars, 5 words
  5번째 줄 전송...
  5번째 줄 결과 수신: 184 chars, 1 words
  6번째 줄 전송...
  6번째 줄 결과 수신: 14 chars, 1 words
  7번째 줄 전송...
  7번째 줄 결과 수신: 10 chars, 1 words
  8번째 줄 전송...
  8번째 줄 결과 수신: 21 chars, 4 words
  9번째 줄 전송...
  9번째 줄 결과 수신: 4 chars, 1 words

  === 처리 통계 ===
  처리 모드: count
  처리한 줄 수: 9줄
  소요 시간: 0.00초

```

### 클라이언트: upper

```

  ./file_processor_clnt test.txt upper

  1번째 줄 전송...
  1번째 줄 결과 수신: THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG.
  2번째 줄 전송...
  2번째 줄 결과 수신: HELLO WORLD.
  3번째 줄 전송...
  3번째 줄 결과 수신: COUNT ME.
  4번째 줄 전송...
  4번째 줄 결과 수신: A B C D E
  5번째 줄 전송...
  5번째 줄 결과 수신: THISLINEISMUCHLONGERTHANTHEOTHERSANDITTESTSBUFFEROVERFLOWSCENARIOSIFTHEBUFFERISSMALLBUTSINCEOURBUFFERISAROUND4KBITSMORELIKELYTOTESTGENERALREADLOGICCONSISTENCYACROSSMULTIPLESYSTEMCALLS.
  6번째 줄 전송...
  6번째 줄 결과 수신: !@#$%^&*()_+-=
  7번째 줄 전송...
  7번째 줄 결과 수신: 1234567890
  8번째 줄 전송...
  8번째 줄 결과 수신: MIXED CASE AND SPACES
  9번째 줄 전송...
  9번째 줄 결과 수신: DONE

  === 처리 통계 ===
  처리 모드: upper
  처리한 줄 수: 9줄
  소요 시간: 0.00초

```



### 클라이언트: lower

```

  ./file_processor_clnt test.txt lower

  1번째 줄 전송...
  1번째 줄 결과 수신: the quick brown fox jumps over the lazy dog.
  2번째 줄 전송...
  2번째 줄 결과 수신: hello world.
  3번째 줄 전송...
  3번째 줄 결과 수신: count me.
  4번째 줄 전송...
  4번째 줄 결과 수신: a b c d e
  5번째 줄 전송...
  5번째 줄 결과 수신: thislineismuchlongerthantheothersandittestsbufferoverflowscenariosifthebufferissmallbutsinceourbufferisaround4kbitsmorelikelytotestgeneralreadlogicconsistencyacrossmultiplesystemcalls.
  6번째 줄 전송...
  6번째 줄 결과 수신: !@#$%^&*()_+-=
  7번째 줄 전송...
  7번째 줄 결과 수신: 1234567890
  8번째 줄 전송...
  8번째 줄 결과 수신: mixed case and spaces
  9번째 줄 전송...
  9번째 줄 결과 수신: done

  === 처리 통계 ===
  처리 모드: lower
  처리한 줄 수: 9줄
  소요 시간: 0.00초

```



### 클라이언트: reverse

```

  ./file_processor_clnt test.txt reverse

  1번째 줄 전송...
  1번째 줄 결과 수신: .god yzal eht revo spmuj xof nworb kciuq ehT
  2번째 줄 전송...
  2번째 줄 결과 수신: .dlrow olleh
  3번째 줄 전송...
  3번째 줄 결과 수신: .em tnuoc
  4번째 줄 전송...
  4번째 줄 결과 수신: E D C B A
  5번째 줄 전송...
  5번째 줄 결과 수신: .sllaCmetsySelpitluMssorcAycnetsisnoCcigoLdaeRlareneGtseToTylekiLeroMstIBK4dnuorAsIreffuBruOecniStuBllamSsIreffuBehTfIsoiranecSwolfrevOreffuBstseTtIdnAsrehtOehTnahTregnoLhcuMsIeniLsihT
  6번째 줄 전송...
  6번째 줄 결과 수신: =-+_)(*&^%$#@!
  7번째 줄 전송...
  7번째 줄 결과 수신: 0987654321
  8번째 줄 전송...
  8번째 줄 결과 수신: secApS dnA eSaC dexim
  9번째 줄 전송...
  9번째 줄 결과 수신: ENOD

  === 처리 통계 ===
  처리 모드: reverse
  처리한 줄 수: 9줄
  소요 시간: 0.00초

```



### 서버

```
  ./file_processor_svr

  1번째 줄 처리 중...
  2번째 줄 처리 중...
  3번째 줄 처리 중...
  4번째 줄 처리 중...
  5번째 줄 처리 중...
  6번째 줄 처리 중...
  7번째 줄 처리 중...
  8번째 줄 처리 중...
  9번째 줄 처리 중...

```