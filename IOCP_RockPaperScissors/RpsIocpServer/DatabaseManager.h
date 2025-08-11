#pragma once
#include <mysql.h>
#include <string>

class DatabaseManager
{
public:
    DatabaseManager();
    ~DatabaseManager();

    /*
     Initialize
      - MySQL 서버에 연결을 시도
      - host: 접속 호스트 (예: "127.0.0.1")
      - port: 포트 번호 (기본 3306)
      - user: DB 사용자명
      - pass: DB 비밀번호
      - dbName: 사용할 데이터베이스 스키마
      - 반환값: 연결 성공 시 true, 실패 시 false
    */
    bool Initialize(const std::string& host, const std::string& user, const std::string& pass, const std::string& dbName, unsigned int port);

    /*
     ExecuteNonQuery
      - INSERT, UPDATE, DELETE 같은 결과를 반환하지 않는 쿼리 실행
      - query: 실행할 SQL 문자열
      - 반환값: 쿼리 성공 시 true, 실패 시 false
    */
    bool ExecuteNonQuery(const std::string& query);

    /*
     ExecuteQuery
      - SELECT 쿼리 실행 및 결과를 콘솔 출력
      - 실제 서버 로직에선 ResultSet을 핸들링하도록 수정 가능
      - query: 실행할 SQL SELECT 문
    */
    void ExecuteQuery(const std::string& query);
    
    // 단일 정수 반환 쿼리 (예: COUNT, SELECT id …)
    bool ExecuteScalarInt(const std::string& query, int& outValue);

    // 마지막 INSERT 된 AUTO_INCREMENT 값 반환
    unsigned long GetLastInsertId() const;

    // username 중복 체크
    bool UserExists(const std::string& username);


    /*
     Shutdown
      - 열려 있는 MySQL 연결을 닫고 정리합니다.
    */
    void Shutdown();

private:
    MYSQL* m_conn;  // MySQL 세션 핸들
};