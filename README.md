# 🎮 Rock–Paper–Scissors Server (IOCP 기반)

> Windows IOCP와 MySQL로 구현한 TCP 기반 가위바위보 게임 서버 입니다.  
> 회원 가입·로그인, 방 생성·참여, 실시간 대결, 전적 조회를 지원합니다.

---

## 🔖 목차

- [🎮 Rock–Paper–Scissors Server (IOCP 기반)](#-rockpaperscissors-server-iocp-기반)
  - [🔖 목차](#-목차)
  - [프로젝트 개요](#프로젝트-개요)
  - [주요 기능](#주요-기능)
  - [기술 스택](#기술-스택)
  - [설치 및 실행](#설치-및-실행)
    - [사전 준비](#사전-준비)
    - [데이터베이스 구성](#데이터베이스-구성)
  - [사용 예시](#사용-예시)
    - [게임 시작](#게임-시작)
    - [회원 가입](#회원-가입)
    - [전적 보기](#전적-보기)
  - [주요 로직](#주요-로직)
    - [IOCPManager](#iocpmanager)
    - [Session](#session)
    - [Room](#room)
    - [GameLogic](#gamelogic)
    - [DatabaseManager](#databasemanager)
  - [예외 처리 및 주의사항](#예외-처리-및-주의사항)
  - [알 수 없는 명령 → ERROR Unknown command](#알-수-없는-명령--error-unknown-command)
  

---

## 프로젝트 개요

Windows의 Overlapped I/O와 IOCP(입출력 완료 포트)를 활용하여  
동시 접속 처리 성능을 극대화한 TCP 기반 가위바위보 게임 서버입니다.  
클라이언트는 명령어 기반 텍스트 프로토콜로 서버와 통신하며,  
MySQL에 유저 정보와 전적을 저장·관리합니다.

---

## 주요 기능

- **회원 관리**  
  - 신규 가입 (`REGISTER`)  
  - 로그인 (`LOGIN`)  

- **전적 조회**  
  - 전체 게임 수, 승·무·패 통계 출력 (`STATS`)  

- **게임 방 관리**  
  - 방 생성 (`CREATE <roomId>`)  
  - 방 참여 (`JOIN <roomId>`)  

- **실시간 대결**  
  - 선택 전송 (`MOVE R/P/S`)  
  - 결과 통보 (`RESULT WIN/LOSE/DRAW`)  
  - 전적 업데이트  

- **비동기 네트워크**  
  - IOCP 기반 Worker 스레드 풀  
  - AcceptEx / WSARecv / WSASend  

---

## 기술 스택

| 영역            | 라이브러리 / 기술                   |
| --------------- | ----------------------------------- |
| 네트워크 I/O    | Winsock2, IOCP (AcceptEx, WSARecv)  |
| 동시성          | Windows Thread, `GetQueuedCompletionStatus` |
| 데이터베이스    | MySQL Server, C API (`mysql.h`)     |
| 직렬화/역직렬화 | PacketManager                       |
| 게임 로직       | GameLogic (가위바위보 규칙)         |
| 언어 / 빌드     | C++17, Visual Studio                |
| 동기화          | `std::mutex`, `std::lock_guard`     |

---

## 설치 및 실행

### 사전 준비

- Windows 10 이상  
- Visual Studio 2019/2022 (C++17 지원)  
- MySQL Server (5.7 이상 권장)  

### 데이터베이스 구성

```sql
CREATE DATABASE IF NOT EXISTS rpsgame_db;
USE rpsgame_db;

CREATE TABLE users (
  id INT AUTO_INCREMENT PRIMARY KEY,
  username VARCHAR(50) NOT NULL UNIQUE,
  password_hash VARCHAR(255) NOT NULL,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE user_status (
  user_id INT PRIMARY KEY,
  games_played INT DEFAULT 0,
  wins INT DEFAULT 0,
  draws INT DEFAULT 0,
  losses INT DEFAULT 0,
  updated_at TIMESTAMP
    DEFAULT CURRENT_TIMESTAMP
    ON UPDATE CURRENT_TIMESTAMP,
  FOREIGN KEY (user_id) REFERENCES users(id)
    ON DELETE CASCADE
) ENGINE=InnoDB;
```
---

## 사용 예시
### 게임 시작
1. 클라이언트 실행 후 메인 메뉴:
```
<가위바위보 게임>
1. 게임시작
2. 회원 가입
3. 전적 보기
선택하세요> 1
```
2. “1” 입력 → 로그인 UI → 성공 시:
```
로그인 성공! 게임 시작 메뉴로 이동합니다.

<가위바위보 게임>
1. 방 만들기
2. 방 참여
선택하세요> 1

방 ID를 입력하세요> room123
[서버] Waiting for User...
```

3. 다른 클라이언트가 동일 방 ID로 참여:
``` 
[서버] OPPONENT_JOINED
*** 상대가 입장했습니다! ***
[서버] START
*** 게임 시작! MOVE R/P/S 입력 ***
```

4. R/P/S 입력 예:
> R
``` 
[서버] RESULT WIN
```

5. - 메인으로 자동 복귀, 원할 때 전적 조회(3) 또는 재방 생성/참여(1)
   
### 회원 가입
- 프로세스
  ClientRegister()
  ID/PW 입력 → 서버에 REGISTER username password\n 전송
- 응답
  REGISTER_SUCCESS → 가입 완료 메시지 후 메인 메뉴
  ERROR DUPLICATE_USER → ID 중복 메시지 후 메인 메뉴
  기타 오류 → ERROR REG_FAIL 등 표시 후 메인 메뉴
- 예외 상황
  서버 연결 실패 → "서버 연결 실패"
  중복 ID → "ID가 중복되었습니다."

### 전적 보기
- 프로세스
  ClientStats() 호출
  ID/PW 입력 → 서버 접속 후 LOGIN → STATS\n 전송
- 응답
  STATS played wins draws losses → 화면 출력 후 메인 메뉴
  로그인 실패 → 메인 메뉴
  기타 오류 → "전적 불러오기 실패"
- 예외 상황
  서버 연결 실패 → "서버 연결 실패"
  로그인 또는 조회 오류 → 메인 메뉴 복귀

---

## 주요 로직
### IOCPManager
- Initialize(port, workerCount, dbMgr)
- Winsock 초기화 → listen 소켓 생성·바인드·리스닝
- AcceptEx, GetAcceptExAddrs 함수 포인터 로딩
- IOCP 생성 및 리슨 소켓 등록
- 워커 스레드 풀 기동 → GetQueuedCompletionStatus 루프 처리
- I/O 타입별 핸들러
- Accept → 새 Session 생성 및 첫 WSARecv 요청
- Read  → 메시지 파싱 → 명령 분기 → 비즈니스 로직 처리
- Write → 컨텍스트 해제
  
### Session
- 클라이언트 소켓, 세션ID, 로그인 상태, 소속 Room 관리
- Send/Receive 래퍼로 IOCPManager에 요청
### Room
- 방 ID, creator/joiner 포인터, 두 플레이어의 선택값, 게임 시작 플래그
- Join, MakeMove, HasBothMoves, Resolve API 제공
### GameLogic
- ResolveRps(a, b) : ‘R’, ‘P’, ‘S’ 두 수치를 비교해 0=무승부, 1=첫 플레이어 승리, -1=두 번째 승리 반환
### DatabaseManager
- MySQL 연결 관리 (Initialize, Shutdown)
- CRUD 쿼리 실행(ExecuteNonQuery, ExecuteQuery, ExecuteScalarInt)
- 사용자 존재 확인(UserExists), 마지막 삽입 ID 조회(GetLastInsertId)

---

## 예외 처리 및 주의사항
- 서버 연결 실패 → INVALID_SOCKET 처리 후 재시도 또는 종료
- REGISTER
 중복 ID → ERROR DUPLICATE_USER
  기타 실패 → ERROR REG_FAIL
- LOGIN
  비밀번호 불일치 → ERROR LOGIN_FAIL
- STATS
  로그인 실패 → 메인으로 복귀
- CREATE
  이미 존재 → ERROR Room already exists
- JOIN
  존재하지 않는 방 → ERROR No such room
  방 정원 초과 → ERROR Room full
- MOVE
  게임 시작 전 → ERROR Game not started
  알 수 없는 명령 → ERROR Unknown command
---

