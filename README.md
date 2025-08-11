# 🎮 Rock–Paper–Scissors Server (IOCP 기반)

> Windows IOCP와 MySQL로 구현한 고성능 TCP 게임 서버 예제입니다.  
> 회원 가입·로그인, 방 생성·참여, 실시간 대결, 전적 조회를 지원합니다.

---

## 🔖 목차

- [🎮 Rock–Paper–Scissors Server (IOCP 기반)](#-rockpaperscissors-server-iocp-기반)
  - [🔖 목차](#-목차)
  - [프로젝트 개요](#프로젝트-개요)
  - [주요 기능](#주요-기능)
  - [기술 스택](#기술-스택)
  - [아키텍처](#아키텍처)
  - [설치 및 실행](#설치-및-실행)
    - [사전 준비](#사전-준비)
    - [데이터베이스 구성](#데이터베이스-구성)

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

## 아키텍처
+------------+          +------------+       +------------+ |  Client #1 | <------> |            |       |            | +------------+          |            |       |            | |  IOCP      | <---> | MySQL DB   | +------------+          |  Manager   |       |            | |  Client #2 | <------> |            |       |            | +------------+          +------------+       +------------+ Worker Threads pool


1. Listen 소켓에 AcceptEx  
2. I/O 완료 알림을 IOCP가 받아 Worker 스레드로 분배  
3. Session 객체별 Read / Write 처리  
4. DatabaseManager를 통해 MySQL 쿼리 실행  

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