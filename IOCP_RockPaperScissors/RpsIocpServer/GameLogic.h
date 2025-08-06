#pragma once

/*
 RpsResult
  - ���������� ��� Ÿ��
*/
enum class RpsResult
{
    Draw,       // ���º�
    FirstWin,   // ù ��° �÷��̾� �¸�
    SecondWin   // �� ��° �÷��̾� �¸�
};


/*
 GameLogic
  - RPS ���������� ���� ����.
  - �ٸ� �������� Ȯ���ϱ� ���� �߻�ȭ ����Ʈ.
*/
class GameLogic
{
public:
    /*
     ResolveRps
      a, b : 'R', 'P', 'S'
      return :  0 = ���
                1 = a �¸�
               -1 = b �¸�
    */
    static int ResolveRps(char a, char b);
};