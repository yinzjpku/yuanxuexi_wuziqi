#include <utility>
#include "GameModel.h"

GameModel::GameModel()
{
    p1Energy = 0;
    p2Energy = 0;
    p1Combo = 0;
    p2Combo = 0;
}

void GameModel::startGame()
{
    // 初始化/清空棋盘
    gameMapVec.clear();
    for (int i = 0; i < kBoardSizeNum; i++)
    {
        std::vector<int> lineBoard(kBoardSizeNum, 0); // 直接初始化为0
        gameMapVec.push_back(lineBoard);
    }

    // 重置回合与状态
    playerFlag = true; // 默认黑方（玩家1）先手
    gameStatus = PLAYING;

    // 重置能量与连答
    p1Energy = 0;
    p2Energy = 0;
    p1Combo = 0;
    p2Combo = 0;
}

void GameModel::actionByPerson(int row, int col)
{
    // 基础落子规则：根据当前落子方填入对应棋子标识
    if (playerFlag)
        gameMapVec[row][col] = -1; // 原项目逻辑中 playerFlag 为 true 落下的是黑子(-1)
    else
        gameMapVec[row][col] = 1;  // playerFlag 为 false 落下的是白子(1)

    // 注意：当前版本的逻辑是点击即换手
    // 后面结合数据同学的“答题功能”时，你需要把下面这句移到答题结算函数里
    playerFlag = !playerFlag;
}

bool GameModel::isWin(int row, int col)
{
    // 保持你原有的四方向延伸5子判定算法，逻辑非常完整，无需变动
    // 水平方向
    for (int i = 0; i < 5; i++)
    {
        if (col - i >= 0 &&
            col - i + 4 < kBoardSizeNum &&
            gameMapVec[row][col - i] == gameMapVec[row][col - i + 1] &&
            gameMapVec[row][col - i] == gameMapVec[row][col - i + 2] &&
            gameMapVec[row][col - i] == gameMapVec[row][col - i + 3] &&
            gameMapVec[row][col - i] == gameMapVec[row][col - i + 4])
            return true;
    }

    // 竖直方向
    for (int i = 0; i < 5; i++)
    {
        if (row - i >= 0 &&
            row - i + 4 < kBoardSizeNum &&
            gameMapVec[row - i][col] == gameMapVec[row - i + 1][col] &&
            gameMapVec[row - i][col] == gameMapVec[row - i + 2][col] &&
            gameMapVec[row - i][col] == gameMapVec[row - i + 3][col] &&
            gameMapVec[row - i][col] == gameMapVec[row - i + 4][col])
            return true;
    }

    // 左斜方向
    for (int i = 0; i < 5; i++)
    {
        if (row + i < kBoardSizeNum &&
            row + i - 4 >= 0 &&
            col - i >= 0 &&
            col - i + 4 < kBoardSizeNum &&
            gameMapVec[row + i][col - i] == gameMapVec[row + i - 1][col - i + 1] &&
            gameMapVec[row + i][col - i] == gameMapVec[row + i - 2][col - i + 2] &&
            gameMapVec[row + i][col - i] == gameMapVec[row + i - 3][col - i + 3] &&
            gameMapVec[row + i][col - i] == gameMapVec[row + i - 4][col - i + 4])
            return true;
    }

    // 右斜方向
    for (int i = 0; i < 5; i++)
    {
        if (row - i >= 0 &&
            row - i + 4 < kBoardSizeNum &&
            col - i >= 0 &&
            col - i + 4 < kBoardSizeNum &&
            gameMapVec[row - i][col - i] == gameMapVec[row - i + 1][col - i + 1] &&
            gameMapVec[row - i][col - i] == gameMapVec[row - i + 2][col - i + 2] &&
            gameMapVec[row - i][col - i] == gameMapVec[row - i + 3][col - i + 3] &&
            gameMapVec[row - i][col - i] == gameMapVec[row - i + 4][col - i + 4])
            return true;
    }

    return false;
}

bool GameModel::isDeadGame()
{
    for (int i = 0; i < kBoardSizeNum; i++)
        for (int j = 0; j < kBoardSizeNum; j++)
        {
            if (!(gameMapVec[i][j] == 1 || gameMapVec[i][j] == -1))
                return false;
        }
    return true;
}
