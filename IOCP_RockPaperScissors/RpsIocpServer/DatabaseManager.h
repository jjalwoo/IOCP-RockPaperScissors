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
      - MySQL ������ ������ �õ�
      - host: ���� ȣ��Ʈ (��: "127.0.0.1")
      - port: ��Ʈ ��ȣ (�⺻ 3306)
      - user: DB ����ڸ�
      - pass: DB ��й�ȣ
      - dbName: ����� �����ͺ��̽� ��Ű��
      - ��ȯ��: ���� ���� �� true, ���� �� false
    */
    bool Initialize(const std::string& host, const std::string& user, const std::string& pass, const std::string& dbName, unsigned int port);

    /*
     ExecuteNonQuery
      - INSERT, UPDATE, DELETE ���� ����� ��ȯ���� �ʴ� ���� ����
      - query: ������ SQL ���ڿ�
      - ��ȯ��: ���� ���� �� true, ���� �� false
    */
    bool ExecuteNonQuery(const std::string& query);

    /*
     ExecuteQuery
      - SELECT ���� ���� �� ����� �ܼ� ���
      - ���� ���� �������� ResultSet�� �ڵ鸵�ϵ��� ���� ����
      - query: ������ SQL SELECT ��
    */
    void ExecuteQuery(const std::string& query);

    /*
     Shutdown
      - ���� �ִ� MySQL ������ �ݰ� �����մϴ�.
    */
    void Shutdown();

private:
    MYSQL* m_conn;  // MySQL ���� �ڵ�
};