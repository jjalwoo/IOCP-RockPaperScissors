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
    // 1) MYSQL ����ü �ʱ�ȭ
    m_conn = mysql_init(nullptr);
    if (!m_conn)
    {
        std::cerr << "[DatabaseManager] mysql_init() ����\n";
        return false;
    }

    // 2) ���� ���� ���� ����
    if (mysql_real_connect(m_conn, host.c_str(), user.c_str(), pass.c_str(), dbName.c_str(), port, nullptr, 0) == nullptr)
    {
        std::cerr << "MySQL Connect ����: " << mysql_error(m_conn) << std::endl;
        return false;
    }

    // 3) ���� ����
    std::cout << "MySQL DB ���� ����\n";
    return true;
}

bool DatabaseManager::ExecuteNonQuery(const std::string& query)
{
    // ���� ����
    if (mysql_query(m_conn, query.c_str()))
    {
        std::cerr << "[DatabaseManager] ExecuteNonQuery ����: "
            << mysql_error(m_conn) << "\n";
        return false;
    }
    return true;
}

void DatabaseManager::ExecuteQuery(const std::string& query)
{
    // ���� ����
    if (mysql_query(m_conn, query.c_str()))
    {
        std::cerr << "[DatabaseManager] ExecuteQuery ����: "
            << mysql_error(m_conn) << "\n";
        return;
    }

    // ��� ����
    MYSQL_RES* result = mysql_store_result(m_conn);
    if (!result)
    {
        std::cerr << "[DatabaseManager] ��� ���� ����: "
            << mysql_error(m_conn) << "\n";
        return;
    }

    // �÷� �� ��ȸ
    int numFields = mysql_num_fields(result);
    MYSQL_ROW row;

    // ��� �ο� ��ȸ �� ���
    while ((row = mysql_fetch_row(result)))
    {
        for (int i = 0; i < numFields; ++i)
        {
            std::cout << (row[i] ? row[i] : "NULL") << "\t";
        }
        std::cout << "\n";
    }

    // �޸� ����
    mysql_free_result(result);
}

// ���� ���� ��ȯ ���� ó��
bool DatabaseManager::ExecuteScalarInt(const std::string& query, int& outValue)
{
    if (mysql_query(m_conn, query.c_str()))
    {
        std::cerr << "[DatabaseManager] ExecuteScalarInt ����: "
            << mysql_error(m_conn) << "\n";
        return false;
    }

    MYSQL_RES* result = mysql_store_result(m_conn);
    if (!result)
    {
        std::cerr << "[DatabaseManager] ExecuteScalarInt ��� ���� ����: "
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

// ������ INSERT�� AUTO_INCREMENT ID ��ȯ
unsigned long DatabaseManager::GetLastInsertId() const
{
    return static_cast<unsigned long>(mysql_insert_id(m_conn));
}

// users ���̺� username �ߺ� ���� Ȯ��
bool DatabaseManager::UserExists(const std::string& username)
{
    int count = 0;
    std::string q = "SELECT COUNT(*) FROM users WHERE username = '"
        + username + "'";

    if (!ExecuteScalarInt(q, count))
    {
        return false;  // ���� ���� ��, �������� �ʴ� �ɷ� ����
    }

    return (count > 0);
}


void DatabaseManager::Shutdown()
{
    if (m_conn)
    {
        mysql_close(m_conn);
        m_conn = nullptr;
        std::cout << "[DatabaseManager] DB ���� ����\n";
    }
}