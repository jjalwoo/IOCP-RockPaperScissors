#include "DatabaseManager.h"
#include <iostream>



DatabaseManager::DatabaseManager()
    : m_conn(nullptr)
{
}

DatabaseManager::~DatabaseManager()
{
    Shutdown();
}

bool DatabaseManager::Initialize(const std::string& host, const std::string& user, const std::string& pass, const std::string& dbName, unsigned int port)
{
    // 1) MYSQL 구조체 초기화
    m_conn = mysql_init(nullptr);
    if (!m_conn)
    {
        std::cerr << "[DatabaseManager] mysql_init() 실패\n";
        return false;
    }

    // 2) 실제 서버 연결 수행
    if (mysql_real_connect(m_conn, host.c_str(), user.c_str(), pass.c_str(), dbName.c_str(), port, nullptr, 0) == nullptr)
    {
        std::cerr << "MySQL Connect 실패: " << mysql_error(m_conn) << std::endl;
        return false;
    }

    // 3) 연결 성공
    std::cout << "MySQL DB 연결 성공\n";
    return true;
}

bool DatabaseManager::ExecuteNonQuery(const std::string& query)
{
    // 쿼리 실행
    if (mysql_query(m_conn, query.c_str()))
    {
        std::cerr << "[DatabaseManager] ExecuteNonQuery 실패: "
            << mysql_error(m_conn) << "\n";
        return false;
    }
    return true;
}

void DatabaseManager::ExecuteQuery(const std::string& query)
{
    // 쿼리 실행
    if (mysql_query(m_conn, query.c_str()))
    {
        std::cerr << "[DatabaseManager] ExecuteQuery 실패: "
            << mysql_error(m_conn) << "\n";
        return;
    }

    // 결과 저장
    MYSQL_RES* result = mysql_store_result(m_conn);
    if (!result)
    {
        std::cerr << "[DatabaseManager] 결과 저장 실패: "
            << mysql_error(m_conn) << "\n";
        return;
    }

    // 컬럼 수 조회
    int numFields = mysql_num_fields(result);
    MYSQL_ROW row;

    // 결과 로우 순회 및 출력
    while ((row = mysql_fetch_row(result)))
    {
        for (int i = 0; i < numFields; ++i)
        {
            std::cout << (row[i] ? row[i] : "NULL") << "\t";
        }
        std::cout << "\n";
    }

    // 메모리 해제
    mysql_free_result(result);
}

// 단일 정수 반환 쿼리 처리
bool DatabaseManager::ExecuteScalarInt(const std::string& query, int& outValue)
{
    if (mysql_query(m_conn, query.c_str()))
    {
        std::cerr << "[DatabaseManager] ExecuteScalarInt 실패: "
            << mysql_error(m_conn) << "\n";
        return false;
    }

    MYSQL_RES* result = mysql_store_result(m_conn);
    if (!result)
    {
        std::cerr << "[DatabaseManager] ExecuteScalarInt 결과 저장 실패: "
            << mysql_error(m_conn) << "\n";
        return false;
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    if (row && row[0])
    {
        outValue = std::atoi(row[0]);
    }
    else
    {
        outValue = 0;
    }

    mysql_free_result(result);
    return true;
}

// 마지막 INSERT된 AUTO_INCREMENT ID 반환
unsigned long DatabaseManager::GetLastInsertId() const
{
    return static_cast<unsigned long>(mysql_insert_id(m_conn));
}

// users 테이블에 username 중복 여부 확인
bool DatabaseManager::UserExists(const std::string& username)
{
    int count = 0;
    std::string q = "SELECT COUNT(*) FROM users WHERE username = '"
        + username + "'";

    if (!ExecuteScalarInt(q, count))
    {
        return false;  // 쿼리 실패 시, 존재하지 않는 걸로 간주
    }

    return (count > 0);
}


void DatabaseManager::Shutdown()
{
    if (m_conn)
    {
        mysql_close(m_conn);
        m_conn = nullptr;
        std::cout << "[DatabaseManager] DB 연결 종료\n";
    }
}