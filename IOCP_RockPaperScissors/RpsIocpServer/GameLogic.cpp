#include "GameLogic.h"

int GameLogic::ResolveRps(char a, char b)
{
    if (a == b)
    {
        return 0;
    }

    if ((a == 'R' && b == 'S') || (a == 'S' && b == 'P') || (a == 'P' && b == 'R'))
    {
        return 1;
    }

    return -1;
}