#pragma once

#include "util.h"
#include "model.h"


class Game : public Entity
{
public:
    enum Action
    {
        RL_ACTION_NONE,
        RL_ACTION_RIGHT,
        RL_ACTION_LEFT,
        RL_ACTION_RIGHT2,
        RL_ACTION_LEFT2,
        RL_ACTION_ROTATE_RIGHT,
        RL_ACTION_ROTATE_LEFT,
    };

    Game()
    {
        for (int x = 0; x < 12; x++)
        {
            for (int y = 0; y < 21; y++)
            {
                stage[x][y] = -1; /*何もない*/
            }
        }
        for (int x = 0; x < 12; x++)
        {
            stage[x][0] = 10; /*壁*/
        }
        for (int y = 0; y < 21; y++)
        {
            stage[0][y] = 10;
            stage[11][y] = 10;
        }

        stageCube = new Cube();
        stageCube->scale = 0.9f;
        stageEntity = new Stage();
    }
    ~Game()
    {
        delete stageCube;
        delete stageEntity;
        delete fallingTet, nextTet;
    }

    void render(ShaderProgram *program, glm::mat4 &model, glm::mat4 &pers, glm::mat4 &view)
    {
        glm::mat4 thisModel;
        thisModel = glm::translate(model, this->position);

        stageEntity->render(program, thisModel, pers, view);
        if (fallingTet)
            fallingTet->render(program, thisModel, pers, view);
        if (nextTet)
            nextTet->render(program, thisModel, pers, view);

        for (int x = 0; x < 12; x++)
        {
            for (int y = 0; y < 21; y++)
            {
                if (0 <= stage[x][y] && stage[x][y] <= 7)
                {
                    glUniform3fv(program->getLocation("objectColor"), 1, glm::value_ptr(Tetrimino::colors[stage[x][y]]));
                    stageCube->position = glm::vec3(x, y, 0);
                    stageCube->render(program, thisModel, pers, view);
                }
            }
        }
    }

    bool step()
    {
        this->fallingTet->position.y--;

        // 衝突判定
        if (checkStageOverlap())
        {
            fallingTet->position.y++;
            int level = this->freeze();
            if (level >= 2 && enemyGame)
                enemyGame->attack(level-1);
            this->add();

            return true;
        }

        return false;
    }

    void update()
    {
        // update children
        stageEntity->update();
        fallingTet->update();

        Action userAction = RL_ACTION_NONE;
        if (isControllable)
        {
            if (gKeyPressed[GLFW_KEY_RIGHT] == 2 || gKeyPressed[GLFW_KEY_RIGHT] > 30)
            {
                userAction = RL_ACTION_RIGHT;
            }
            if (gKeyPressed[GLFW_KEY_LEFT] == 2 || gKeyPressed[GLFW_KEY_LEFT] > 30)
            {
                userAction = RL_ACTION_LEFT;
            }
            if (gKeyPressed[GLFW_KEY_UP] == 2)
            {
                userAction = RL_ACTION_ROTATE_RIGHT;
            }
            act(userAction);

            if (gKeyPressed[GLFW_KEY_DOWN] == 2)
            {
                this->step();
            }

            if (gKeyPressed[GLFW_KEY_ENTER] == 2)
            {
                while (!this->step()) {};
            }

        }
    }

    // ユーザー入力による行動を処理. 壁やブロックにめり込むなど、行動を巻き戻す必要があった場合はtrueを返す.
    bool act(Action action)
    {
        bool error = false;

        switch (action)
        {
        case RL_ACTION_RIGHT:
        {
            fallingTet->position.x++;
            if (checkStageOverlap())
            {
                error = true;
                fallingTet->position.x--;
            }
        }
        break;
        case RL_ACTION_LEFT:
        {
            fallingTet->position.x--;
            if (checkStageOverlap())
            {
                error = true;
                fallingTet->position.x++;
            }
        }
        break;
        /*コンピュータ用　プレイヤーもキーを２回叩けばできるので、不公平ではない*/
        case RL_ACTION_RIGHT2:
        {
            act(RL_ACTION_RIGHT);
            act(RL_ACTION_RIGHT);
        }
        break;
        case RL_ACTION_LEFT2:
        {
            act(RL_ACTION_LEFT);
            act(RL_ACTION_LEFT);
        }break;
        case RL_ACTION_ROTATE_RIGHT:
        {
            fallingTet->rotate();

            // 方式1：回転した先だけを見て入るならok, 入らないならno
            // if (checkStageOverlap())
            //     fallingTet->unrotate();

            // 方式2: Super Rotation System. ちょっとずらしてみて入るならokとする
            // https://tetrisch.github.io/main/srs.html
            if (checkStageOverlap())
            {
                fallingTet->position.x--;
                if (checkStageOverlap())
                {
                    fallingTet->position.y++;
                    if (checkStageOverlap())
                    {
                        fallingTet->position.x++;
                        fallingTet->position.y -= 3;
                        if (checkStageOverlap())
                        {
                            fallingTet->position.x--;
                            if (checkStageOverlap())
                            {
                                error = true;
                                fallingTet->unrotate();
                                fallingTet->position.x += 1;
                                fallingTet->position.y += 2;
                            }
                        }
                    }
                }
            }
        }
        break;
        }

        return error;
    }

    int freeze()
    {
        int eliminatedRows = 0;

        std::cout << std::endl;
        for (glm::vec3 relpos : Tetrimino::positions[fallingTet->type])
        {
            for (int i = 0; i < fallingTet->rotnum; i++)
            {
                int tmpx = relpos.x;
                relpos.x = relpos.y;
                relpos.y = -tmpx;
            }
            glm::vec3 blockPos = this->fallingTet->position + relpos;
            int x = (int)blockPos.x;
            int y = (int)blockPos.y;

            if (x < 0 || 12 <= x || y < 0 || 21 <= y)
            {
                std::cout << "overflow freeze " << x << " " << y << std::endl;
                // よくない
            }

            std::cout << "stage put " << x << " " << y << std::endl;
            stage[x][y] = fallingTet->type;
        }

        for (int y = 1; y < 21; y++)
        {
            bool yay = true;
            for (int x = 0; x < 12; x++)
            {
                if (stage[x][y] == -1)
                    yay = false;
            }
            if (yay)
            {
                eliminatedRows++;
                for (int j = y + 1; j < 21; j++)
                {
                    for (int x = 0; x < 12; x++)
                    {

                        stage[x][j - 1] = stage[x][j];
                    }
                }
                y--;
            }
        }
        return eliminatedRows;
    }

    virtual void add()
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dis(0, 6);

        if (fallingTet)
            delete fallingTet;
        this->fallingTet = new Tetrimino(nextTet ? nextTet->type : diceNext());
        fallingTet->scale = 0.9f;
        this->fallingTet->position = glm::vec3(6, 19, 0);

        if (nextTet)
            delete nextTet;
        // nextTet = new Tetrimino(0);
        nextTet = new Tetrimino(diceNext());
        nextTet->scale = 0.9f;
        nextTet->position = glm::vec3(14, 18, 0);

        // 追加してすぐ重なるようなら、負け
        if (checkStageOverlap())
        {
            winFlag = false;
        }
    }

    bool checkStageOverlap()
    {
        for (glm::vec3 relpos : Tetrimino::positions[fallingTet->type])
        {
            for (int i = 0; i < fallingTet->rotnum; i++)
            {
                int tmpx = relpos.x;
                relpos.x = relpos.y;
                relpos.y = -tmpx;
            }
            glm::vec3 blockPos = this->fallingTet->position + relpos;
            int x = (int)blockPos.x;
            int y = (int)blockPos.y;

            if (x < 0 || 12 < x || y < 0 || 21 < y)
                return true;

            if (stage[x][y] >= 0)
            {
                return true;
            }
        }
        return false;
    }

    void reset()
    {
        winFlag = true;
        for (int x = 0; x < 12; x++)
        {
            for (int y = 0; y < 21; y++)
            {
                stage[x][y] = -1; /*何もない*/
            }
        }
        for (int x = 0; x < 12; x++)
        {
            stage[x][0] = 10; /*壁*/
        }
        for (int y = 0; y < 21; y++)
        {
            stage[0][y] = 10;
            stage[11][y] = 10;
        }

        this->add();
    }

    virtual void attack(int level)
    {
        for (int y = 20; y >= 1; y--)
        {
            if (y-level <= 0) continue;
            for (int x = 1; x <= 10; x++) {
                stage[x][y] = stage[x][y-level];
            }
        }

        for (int y = 1; y < 1+level; y++)
        {
            int space = getRandomInt(1, 10);
            for (int x = 1; x <= 10; x++) {
                stage[x][y] = (x == space) ? -1 : 7;
            }
        }
    }

    bool winFlag = true;
    bool isControllable = true;
    Game *enemyGame;

protected:
    int diceNext() {
        // 方式1: 完全ランダム
        // return getRandomInt(0,6);

        // 方式2: 
        if (nextStore.size() > 0) {
            int randidx = getRandomInt(0, nextStore.size()-1);
            int result = nextStore.at(randidx);
            nextStore.erase(nextStore.begin() + randidx);
            return result;
        } else {
            for (int i = 0; i <= 6; ++i) {
                nextStore.push_back(i);
            }
            return diceNext();
        }
    }
    std::array<std::array<int, 21>, 12> stage;
    std::vector<int> nextStore{};

    Tetrimino *nextTet = nullptr;
    Tetrimino *fallingTet = nullptr;
    Stage *stageEntity;
    Cube *stageCube;
};