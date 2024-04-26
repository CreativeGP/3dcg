#pragma once

#include "game.h"


class CPUGame : public Game
{
public:
    CPUGame() { isControllable = false; }
    ~CPUGame() {}

    void step()
    {
        if (fallingTet->position.y == 19)
            this->stageTraversal();
        if (registeredActions.size() != 0)
        {
            act(registeredActions.back());
            registeredActions.pop_back();
        }
        Game::step();
    }

    void add() override
    {
        Game::add();
        if (registeredActions.size() > 0) {
        registeredActions.clear();
            
        }
        stageTraversal();
    }

    void attack(int level) override {
        Game::attack(level);
        stageTraversal();
        std::cout << "attacked " << std::endl;
    }

    void stageTraversal()
    {
        int count = 0;
        int maxScore = 0;
        std::vector<Action> maxActions;

        reachedHashes.clear();

        std::deque<std::vector<Action>> que;
        que.push_front({RL_ACTION_LEFT2});
        que.push_front({RL_ACTION_RIGHT2});

        while (!que.empty())
        {
            std::vector<Action> actions;
            if (getRandomInt(0,1) == 0) {
                actions = que.front();
                que.pop_front();  
            } else {
                actions = que.back();
                que.pop_back();
            }

            int score = getActionsScore(actions);
            if (score == -2 || score == -3 || actions.size() > 20)
            {
                // skip
            }
            else if (score > 0)
            {
                // 盤面の評価値
                count++;
                if (score > maxScore)
                {
                    maxScore = score;
                    maxActions = actions;
                }
            }
            else
            {
                std::vector<Action> preferredNextAction;
                if (actions.size() < 4) {
                    if (actions[0] == RL_ACTION_LEFT2) {
                        preferredNextAction = {RL_ACTION_LEFT, RL_ACTION_ROTATE_RIGHT, RL_ACTION_LEFT2, RL_ACTION_NONE};
                    }
                    else if (actions[0] == RL_ACTION_RIGHT2) {
                        preferredNextAction = {RL_ACTION_RIGHT, RL_ACTION_ROTATE_RIGHT, RL_ACTION_RIGHT2, RL_ACTION_NONE};
                    }
                } else {
                    preferredNextAction = {RL_ACTION_NONE, RL_ACTION_ROTATE_RIGHT, RL_ACTION_LEFT, RL_ACTION_RIGHT};
                }

                for (auto nextAction : std::vector<Action>(preferredNextAction.rbegin(), preferredNextAction.rend())) {
                    actions.push_back(nextAction);
                    if (actions[0] == RL_ACTION_LEFT2) {
                        que.push_back(actions);
                    } else {
                        que.push_front(actions);
                    }
                    actions.pop_back();
                }
            }
        }

        std::cout << std::endl
                  << "MAX SCORE: " << maxScore << std::endl;
        for (Action cpuAction : maxActions)
        {
            switch (cpuAction)
            {
            case RL_ACTION_LEFT:
                std::cout << "LEFT ";
                break;
            case RL_ACTION_RIGHT:
                std::cout << "RIGHT ";
                break;
            case RL_ACTION_LEFT2:
                std::cout << "LL ";
                break;
            case RL_ACTION_RIGHT2:
                std::cout << "RR ";
                break;
            case RL_ACTION_ROTATE_RIGHT:
                std::cout << "ROT ";
                break;
            case RL_ACTION_NONE:
                std::cout << "- ";
                break;
            }
        }
        reachedHashes.clear();
        if (maxScore != getActionsScore(maxActions)) {
        std::cout << std::endl
                  << "MAX SCORE: " << maxScore << std::endl;
        }
        std::cout << std::endl;

        registeredActions = maxActions;
        std::reverse(registeredActions.begin(), registeredActions.end());
    }

    /* actionsを適用した盤面に対して、freezeするか確かめて、freezeする場合は盤面の評価をする
        -1 --- freezeしない
        -2 --- 枝刈り（エラー操作）
        -3 --- 枝刈り（重複する途中の盤面）
        正の値 --- freezeする（有効な盤面）
    */
    int getActionsScore(const std::vector<Action> &actions)
    {
        int score = -1;

        glm::vec3 remPosition = fallingTet->position;
        int remRotnum = fallingTet->rotnum;

        for (Action cpuAction : actions)
        {
            // 1step 1回行動と仮定する
            if (act(cpuAction))
            {
                score = -2; /* action erro */
                goto rewind_and_return;
            }
            fallingTet->position.y -= 1;
        }

        if (reachedHashes.count(getHash()) == 0)
        {
            reachedHashes.insert(getHash());
        }
        else
        {
            score = -3; /*already reached*/
            goto rewind_and_return;
        }

        if (checkStageOverlap())
        {
            stageBackup = stage;
            fallingTet->position.y += 1;
            freeze();
            score = evaluateStage();
            stage = stageBackup;
        }

    rewind_and_return:
        fallingTet->position = remPosition;
        fallingTet->rotnum = remRotnum;
        return score;
    }

    int evaluateStage2()
    {
        int score = 50000;
        const int holePenalty = 30;
        const int topPenalty = 7;
        const int stepPenalty = 1;

        // 基本的には、上面が揃っている方が良い
        std::array<int, 11> tops;
        int neighborTop = 0;
        for (int x = 1; x <= 10; x++)
        {
            int top = 0;
            for (int y = 1; y < 21; y++)
            {
                if (stage[x][y] >= 0)
                    top = y;
            }
            score -= abs(neighborTop - top)*top*stepPenalty;
            score -= top * top * topPenalty;
            neighborTop = top;
            tops[x] = top;
        }

        // 穴が空いていたら減点する
        int hole = 0;
        for (int x = 1; x <= 10; x++)
        {
            for (int y = 1; y < 21; y++)
            {
                if (y < tops[x] && stage[x][y] == -1)
                {
                    std::cout << "hole " << x << " " << y << " " << stage[x][y] << " " << tops[x] << std::endl;
                    hole++;
                }
            }
        }
        std::cout << hole << " HOLES" << std::endl;
        score -= hole * holePenalty;

        return score;
    }

    int evaluateStage()
    {
        int score = 50000;
        const int holePenalty = 30;
        const int topPenalty = 7;
        const int stepPenalty = 1;

        // 基本的には、上面が揃っている方が良い
        std::array<int, 11> tops;
        int neighborTop = 0;
        for (int x = 1; x <= 10; x++)
        {
            int top = 0;
            for (int y = 1; y < 21; y++)
            {
                if (stage[x][y] >= 0)
                    top = y;
            }
            score -= abs(neighborTop - top)*stepPenalty;
            score -= top * topPenalty;
            neighborTop = top;
            tops[x] = top;
        }

        // 穴が空いていたら減点する
        int hole = 0;
        for (int x = 1; x <= 10; x++)
        {
            for (int y = 1; y < 21; y++)
            {
                if (y < tops[x] && stage[x][y] == -1)
                {
                    std::cout << "hole " << x << " " << y << " " << stage[x][y] << " " << tops[x] << std::endl;
                    hole++;
                }
            }
        }
        std::cout << hole << " HOLES" << std::endl;
        score -= hole * holePenalty;

        return score;
    }

    //
    size_t getHash()
    {
        std::hash<std::string> hasher;
        std::string input;

        // for (int x = 1; x <= 10; x++)
        // {
        //     int top = 0;
        //     for (int y = 1; y < 21; y++)
        //     {
        //         if (stage[x][y] > 0) input += '1';
        //         else input += '0';
        //     }
        //     input += "\n";
        // }

        input += std::to_string(fallingTet->position.x) + " ";
        input += std::to_string(fallingTet->position.y) + " ";
        input += std::to_string(fallingTet->position.z) + " ";
        input += std::to_string(fallingTet->rotnum) + " ";

        return hasher(input);
    }

private:
    std::array<std::array<int, 21>, 12> stageBackup;
    std::unordered_set<size_t> reachedHashes;
    std::vector<Action> registeredActions;
};
