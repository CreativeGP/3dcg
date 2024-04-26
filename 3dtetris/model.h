#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <vector>
#include <random>
#include <array>
#include <queue>
#include <deque>
#include <stack>
#include <functional>
#include <unordered_set>

#include "util.h"

class ShaderProgram
{
public:
    ShaderProgram()
    {
        program = glCreateProgram();
    }
    ~ShaderProgram()
    {
        glDeleteProgram(program);
    }

    void addShader(GLenum shaderType, const char *souceCode)
    {
        GLuint shader = glCreateShader(shaderType);
        glShaderSource(shader, 1, &souceCode, nullptr);
        glCompileShader(shader);

        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (success == GL_FALSE)
        {
            GLint logLength;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
            std::vector<GLchar> log(logLength);
            glGetShaderInfoLog(shader, logLength, nullptr, log.data());
            std::cerr << "シェーダーコンパイルエラー:\n"
                      << log.data() << std::endl;
            glDeleteShader(shader);
        }
        else
        {
            glAttachShader(this->program, shader);
            glDeleteShader(shader);
        }
    }

    void link()
    {
        glLinkProgram(this->program);
    }

    void use()
    {
        glUseProgram(this->program);
    }

    GLuint getLocation(const char *location_name)
    {
        return glGetUniformLocation(this->program, location_name);
    }

private:
    GLuint program;
};

class Entity
{
public:
    Entity(){};
    ~Entity(){};

    virtual void update() = 0;
    virtual void render(ShaderProgram *program, glm::mat4 &model, glm::mat4 &pers, glm::mat4 &view) = 0;

    glm::vec3 position{0, 0, 0};
    glm::quat rotation{1, 0, 0, 0};
    float scale = 1.0f;

private:
};

class Cube : public Entity
{
public:
    Cube()
    {
        // 頂点バッファオブジェクト（VBO）の作成とデータの設定
        GLuint vbo;
        glGenBuffers(1, &this->vbo);
        glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        // 頂点配列オブジェクト（VAO）の作成
        glGenVertexArrays(1, &this->vao);
        glBindVertexArray(this->vao);

        // 頂点属性の設定
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), nullptr);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void *)(sizeof(float) * 3));
        glEnableVertexAttribArray(1);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        
        checkGLError();
    }
    ~Cube()
    {
        glDeleteBuffers(1, &vbo);
        glDeleteBuffers(1, &ibo);

        glDeleteVertexArrays(1, &vao);
    }

    void update()
    {
    }

    void render(ShaderProgram *program, glm::mat4 &model, glm::mat4 &pers, glm::mat4 &view)
    {
        // MVP行列
        glm::mat4 thisModel = glm::translate(model, this->position);
        thisModel = glm::mat4_cast(this->rotation) * thisModel;
        thisModel = glm::scale(thisModel, glm::vec3(scale));
        glUniformMatrix4fv(program->getLocation("MVP"), 1, GL_FALSE, glm::value_ptr(pers * view * thisModel));
        glUniformMatrix4fv(program->getLocation("M"), 1, GL_FALSE, glm::value_ptr(thisModel));

        // シェーダプログラムの使用開始
        program->use();

        // 描画コール
        glBindVertexArray(vao);
        // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->ibo);
        // glDrawArrays(GL_TRIANGLES, 0, sizeof(vertices) / sizeof(GLfloat));
        glDrawArrays(GL_TRIANGLES, 0, 32);
        // glDrawElements(GL_TRIANGLES, sizeof(indices) / sizeof(GLuint), GL_UNSIGNED_INT, nullptr);
    }

private:
    GLuint vao, vbo, ibo;

    // 頂点データ
    static inline GLfloat vertices[] = {
        /*blue*/
        0.5f,
        -0.5f,
        0.5f,
        0.0f,
        0.0f,
        1.0f,
        -0.5f,
        -0.5f,
        0.5f,
        0.0f,
        0.0f,
        1.0f,
        0.5f,
        0.5f,
        0.5f,
        0.0f,
        0.0f,
        1.0f,
        -0.5f,
        0.5f,
        0.5f,
        0.0f,
        0.0f,
        1.0f,
        0.5f,
        0.5f,
        0.5f,
        0.0f,
        0.0f,
        1.0f,
        -0.5f,
        -0.5f,
        0.5f,
        0.0f,
        0.0f,
        1.0f,

        -0.5f,
        0.5f,
        0.5f,
        -1.0f,
        0.0f,
        0.0f,
        -0.5f,
        0.5f,
        -0.5f,
        -1.0f,
        0.0f,
        0.0f,
        -0.5f,
        -0.5f,
        -0.5f,
        -1.0f,
        0.0f,
        0.0f,
        -0.5f,
        -0.5f,
        -0.5f,
        -1.0f,
        0.0f,
        0.0f,
        -0.5f,
        -0.5f,
        0.5f,
        -1.0f,
        0.0f,
        0.0f,
        -0.5f,
        0.5f,
        0.5f,
        -1.0f,
        0.0f,
        0.0f,

        /*red*/
        0.5f,
        0.5f,
        -0.5f,
        1.0f,
        0.0f,
        0.0f,
        0.5f,
        0.5f,
        0.5f,
        1.0f,
        0.0f,
        0.0f,
        0.5f,
        -0.5f,
        -0.5f,
        1.0f,
        0.0f,
        0.0f,
        0.5f,
        -0.5f,
        0.5f,
        1.0f,
        0.0f,
        0.0f,
        0.5f,
        -0.5f,
        -0.5f,
        1.0f,
        0.0f,
        0.0f,
        0.5f,
        0.5f,
        0.5f,
        1.0f,
        0.0f,
        0.0f,

        -0.5f,
        -0.5f,
        -0.5f,
        0.0f,
        -1.0f,
        0.0f,
        0.5f,
        -0.5f,
        -0.5f,
        0.0f,
        -1.0f,
        0.0f,
        0.5f,
        -0.5f,
        0.5f,
        0.0f,
        -1.0f,
        0.0f,
        0.5f,
        -0.5f,
        0.5f,
        0.0f,
        -1.0f,
        0.0f,
        -0.5f,
        -0.5f,
        0.5f,
        0.0f,
        -1.0f,
        0.0f,
        -0.5f,
        -0.5f,
        -0.5f,
        0.0f,
        -1.0f,
        0.0f,
        /*green*/

        0.5f,
        0.5f,
        -0.5f,
        0.0f,
        1.0f,
        0.0f,
        
        -0.5f,
        0.5f,
        -0.5f,
        0.0f,
        1.0f,
        0.0f,

        0.5f,
        0.5f,
        0.5f,
        0.0f,
        1.0f,
        0.0f,


        0.5f,
        0.5f,
        0.5f,
        0.0f,
        1.0f,
        0.0f,

        -0.5f,
        0.5f,
        0.5f,
        0.0f,
        1.0f,
        0.0f,


        -0.5f,
        0.5f,
        -0.5f,
        0.0f,
        1.0f,
        0.0f,

        - 0.5f,
        -0.5f,
        -0.5f,
        0.0f,
        0.0f,
        -1.0f,
        0.5f,
        -0.5f,
        -0.5f,
        0.0f,
        0.0f,
        -1.0f,
        0.5f,
        0.5f,
        -0.5f,
        0.0f,
        0.0f,
        -1.0f,
        0.5f,
        0.5f,
        -0.5f,
        0.0f,
        0.0f,
        -1.0f,
        -0.5f,
        0.5f,
        -0.5f,
        0.0f,
        0.0f,
        -1.0f,
        -0.5f,
        -0.5f,
        -0.5f,
        0.0f,
        0.0f,
        -1.0f,
    };

};

class Tetrimino : public Entity
{
public:
    Tetrimino(int type) : type(type)
    {
        for (auto pos : Tetrimino::positions[type])
        {
            Cube *cube = new Cube();
            cube->position = pos;
            cube->scale = 0.9f;
            entities.push_back(cube);
        }
    }
    ~Tetrimino()
    {
        for (auto entity : entities)
        {
            delete entity;
        }
    }

    void rotate()
    {
        rotnum = (rotnum + 1) % 4;
        rotLerp = 1;
    }

    void unrotate()
    {
        rotnum = (rotnum - 1) % 4;
        rotLerp = 0;
    }
    void update()
    {
        if (rotLerp >= 0)
            rotLerp -= 0.3f;
        if (rotLerp <= 0)
            rotLerp = 0;
        for (auto entity : entities)
        {
            entity->update();
        }
    }

    void render(ShaderProgram *program, glm::mat4 &model, glm::mat4 &pers, glm::mat4 &view)
    {
        glm::mat4 thisModel = glm::translate(model, this->position);
        thisModel = glm::rotate(thisModel, glm::radians((rotnum - rotLerp) * (-90.f)), glm::vec3(0, 0, 1));
        for (auto entity : entities)
        {
            glUniform3fv(program->getLocation("objectColor"), 1, glm::value_ptr(this->colors[this->type]));
            entity->render(program, thisModel, pers, view);
        }
    }

    int type;
    int rotnum = 0;    /* 0, 1, 2, 3 */
    float rotLerp = 0; /* to 100 */

    inline const static std::vector<std::vector<glm::vec3>> positions = {
        {
            /*purple*/
            glm::vec3(0, 0, 0),
            glm::vec3(0, 1, 0),
            glm::vec3(-1, 0, 0),
            glm::vec3(1, 0, 0),
        },
        {
            /*cyan*/
            glm::vec3(0, 0, 0),
            glm::vec3(-1, 0, 0),
            glm::vec3(1, 0, 0),
            glm::vec3(2, 0, 0),
        },
        {
            /*green*/
            glm::vec3(0, 0, 0),
            glm::vec3(0, 1, 0),
            glm::vec3(1, 0, 0),
            glm::vec3(1, -1, 0),
        },
        {
            /*red*/
            glm::vec3(0, 0, 0),
            glm::vec3(0, -1, 0),
            glm::vec3(1, 0, 0),
            glm::vec3(1, 1, 0),
        },
        {
            /*orange*/
            glm::vec3(0, 0, 0),
            glm::vec3(0, 1, 0),
            glm::vec3(0, -1, 0),
            glm::vec3(-1, 1, 0),
        },
        {
            /*blue*/
            glm::vec3(0, 0, 0),
            glm::vec3(0, 1, 0),
            glm::vec3(0, -1, 0),
            glm::vec3(1, 1, 0),
        },
        {
            glm::vec3(0, 0, 0),
            glm::vec3(0, 1, 0),
            glm::vec3(1, 0, 0),
            glm::vec3(1, 1, 0),
        },
    };

    static inline const std::vector<glm::vec3> colors = {
        glm::vec3(.31f, 0, 0.31f), /*purple*/
        glm::vec3(0, .31f, 0.31f), /*cyan*/
        glm::vec3(0, 1.f, 0),      /*green*/
        glm::vec3(1.f, 0, 0),      /*red*/
        glm::vec3(1.f, .49f, 0),   /*orange*/
        glm::vec3(0, 0, 1.f),      /*blue*/
        glm::vec3(1.f, 1.f, 0),    /*yellow*/
        glm::vec3(0, 0, 0),        /*black*/
    };

private:
    std::vector<Entity *> entities;
};

class Grid : public Entity
{
public:
    Grid()
    {
        instance_cube = new Cube();
    }
    ~Grid()
    {
        delete instance_cube;
    }

    void update()
    {
    }

    void render(ShaderProgram *program, glm::mat4 &model, glm::mat4 &pers, glm::mat4 &view)
    {
        for (int x = -1; x < 2; x++)
        {
            for (int y = -1; y < 2; y++)
            {
                for (int z = -1; z < 2; z++)
                {
                    glm::mat4 thisModel = glm::translate(model, this->position);
                    glUniform3fv(program->getLocation("objectColor"), 1, glm::value_ptr(glm::vec3(0, 0, 0)));
                    instance_cube->render(program, thisModel, pers, view);
                }
            }
        }
    }

private:
    Cube *instance_cube;
};

class Stage : public Entity
{
public:
    Stage()
    {
        instance_cube = new Cube();
    }
    ~Stage()
    {
        delete instance_cube;
    }

    void update()
    {
    }

    void render(ShaderProgram *program, glm::mat4 &model, glm::mat4 &pers, glm::mat4 &view)
    {
        glUniform3fv(program->getLocation("objectColor"), 1, glm::value_ptr(glm::vec3(0.7f, 0.7f, 0.7f)));
        for (int x = 0; x < 12; x++)
        {
            for (int y = 0; y < 21; y++)
            {
                glm::mat4 thisModel = glm::translate(model, glm::vec3(x, y, -1));
                thisModel = glm::mat4_cast(this->rotation) * thisModel;
                instance_cube->render(program, thisModel, pers, view);
            }
        }

        for (int y = 0; y < 21; y++)
        {
            glm::mat4 thisModel = glm::translate(model, glm::vec3(0, y, 0));
            thisModel = glm::mat4_cast(this->rotation) * thisModel;
            instance_cube->render(program, thisModel, pers, view);
        }

        for (int y = 0; y < 21; y++)
        {
            glm::mat4 thisModel = glm::translate(model, glm::vec3(11, y, 0));
            thisModel = glm::mat4_cast(this->rotation) * thisModel;
            instance_cube->render(program, thisModel, pers, view);
        }

        for (int x = 0; x < 12; x++)
        {
            glm::mat4 thisModel = glm::translate(model, glm::vec3(x, 0, 0));
            thisModel = glm::mat4_cast(this->rotation) * thisModel;
            instance_cube->render(program, thisModel, pers, view);
        }
    }

    int type;

private:
    Cube *instance_cube;
};
