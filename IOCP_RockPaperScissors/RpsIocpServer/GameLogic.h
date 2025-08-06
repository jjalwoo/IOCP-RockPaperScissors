#pragma once

/*
 RpsResult
  - 가위바위보 결과 타입
*/
enum class RpsResult
{
    Draw,       // 무승부
    FirstWin,   // 첫 번째 플레이어 승리
    SecondWin   // 두 번째 플레이어 승리
};


/*
 GameLogic
  - RPS 가위바위보 룰을 구현.
  - 다른 게임으로 확장하기 위한 추상화 포인트.
*/
class GameLogic
{
public:
    /*
     ResolveRps
      a, b : 'R', 'P', 'S'
      return :  0 = 비김
                1 = a 승리
               -1 = b 승리
    */
    static int ResolveRps(char a, char b);
};