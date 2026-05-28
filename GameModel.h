#ifndef GAMEMODEL_H
#define GAMEMODEL_H

#include <vector>

// 游戏状态
enum GameStatus
{
    PLAYING,
    WIN,
    DEAD
};

// 技能类型枚举（为你后续实现技能预留）
enum SkillType {
    SKILL_SKIP,   // 跳过
    SKILL_BLOCK,  // 封锁
    SKILL_HINT    // 提示
};

// 棋盘尺寸
const int kBoardSizeNum = 15;

class GameModel
{
public:
    GameModel();

public:
    // 存储当前游戏棋盘情况：0为空白，1为玩家1（黑子），-1为玩家2（白子）
    // （后续写技能规则时，可以扩展 2 和 -2 代表双方的封锁转态）
    std::vector<std::vector<int>> gameMapVec;

    bool playerFlag;       // 标示下棋方：true 为玩家1(黑)，false 为玩家2(白)
    GameStatus gameStatus; // 游戏状态

    // 玩家能量与连答数据（技能规则核心，为你后续开发预留位置）
    int p1Energy;
    int p2Energy;
    int p1Combo;
    int p2Combo;

    void startGame();                      // 开始/重置游戏
    void actionByPerson(int row, int col); // 玩家落子执行
    bool isWin(int row, int col);          // 判断游戏是否胜利
    bool isDeadGame();                     // 判断是否和棋
    // 检查某个位置是否允许落子（不能有子，也不能是禁手点）
    bool isValidMove(int row, int col) {
        if (row < 0 || row >= kBoardSizeNum || col < 0 || col >= kBoardSizeNum) return false;
        return gameMapVec[row][col] == 0; // 只有完全是 0 的空白处才能下
    }

    // 标记某个位置为禁手点
    void markAsForbidden(int row, int col) {
        gameMapVec[row][col] = 2; // 2 代表因为答错而被封锁的格子
    }

    // 获取连成五子的所有坐标（用于高亮显示）
    std::vector<std::pair<int,int>> getWinLine(int row, int col) const;
};

#endif // GAMEMODEL_H
