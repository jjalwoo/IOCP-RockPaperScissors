#include "IOCPManager.h"
#include "DatabaseManager.h"
#include "DBConfig.h"

int main()
{
    // IOCPManager
    IOCPManager server;

	// MySQL 데이터베이스 매니저
	DatabaseManager dbManager;   

	// DBConfig 객체 생성
	DBConfig dbConfig;    
    if (!dbConfig.LoadConfig("config.json"))
    {
        std::cerr << "Failed to load DB config!" << std::endl;
        return -1;
    }

    // MySQL 데이터베이스 연결 초기화
	// rpsgame_db 데이터베이스에 연결
    if (!dbManager.Initialize(dbConfig.host, dbConfig.user, dbConfig.password, dbConfig.dbname, dbConfig.port))
    {
        return -1;
    }

    // 포트 9000, 워커 스레드 4개로 초기화
    if (!server.Initialize(9000, 4))
    {
        return -1;
    }	

    // 서버 실행 (워커 스레드 대기)
    server.Run();

    // DB 연결 종료
	dbManager.Shutdown();
    return 0;
}
