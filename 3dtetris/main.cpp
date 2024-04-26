#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <vector>
#include <array>
#include <queue>
#include <deque>
#include <stack>
#include <functional>
#include <unordered_set>

#include "util.h"
#include "model.h"
#include "game.h"
#include "cpu.h"





#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800

// カメラの位置と回転
glm::vec3 cameraPosition(15.0f, 10.0f, 30.0f);
glm::vec3 cameraDirection(0.0f, 0.0f, -1.0f);
glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
float yaw = -90.0f;
float pitch = 0.0f;
bool firstMouse = true;
float lastMouseX = WINDOW_WIDTH / 2.0f;
float lastMouseY = WINDOW_HEIGHT / 2.0f;

// マウスのコールバック関数
void mouseCallback(GLFWwindow *window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastMouseX = xpos;
        lastMouseY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastMouseX;
    float yoffset = lastMouseY - ypos; // y座標は上下反転しているため逆にする
    lastMouseX = xpos;
    lastMouseY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    // ピッチの範囲を制限する
    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    // 新しいカメラの方向ベクトルを計算する
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraDirection = glm::normalize(front);
}

void keyboardCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    float sensitivity = 1.f;

    if (action == GLFW_PRESS)
        gKeyPressed[key]++;
    if (action == GLFW_RELEASE)
        gKeyPressed[key] = 0;
}

// シェーダソースコード
// 頂点シェーダー
const char *vertexShaderSource = R"(
    #version 330 core
    layout(location=0) in vec3 aPos;
    layout(location=1) in vec3 aNormal;
    layout(location=2) in vec3 aTexChoords;

    out vec3 Normal;
    out vec3 FragPos;
    out vec4 FragPosLightSpace;

    uniform mat4 MVP;
    uniform mat4 M;
    uniform mat4 lightSpaceMatrix;

    void main()
    {
        gl_Position = MVP * vec4(aPos, 1.0);
        FragPos = vec3(M * vec4(aPos, 1.0));
        FragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0);
        Normal = transpose(inverse(mat3(M))) * aNormal;
    }
)";

const char *fragmentShaderSource = R"(
    #version 330 core
    
    in vec3 Normal;
    in vec3 FragPos;
    in vec4 FragPosLightSpace;

    out vec4 FragColor;

    uniform vec3 lightPos;
    uniform vec3 objectColor;
    uniform sampler2D depthMap;

    float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
    {
        vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
        projCoords = projCoords * 0.5 + 0.5;
        float closestDepth = texture(depthMap, projCoords.xy).r; 
        float currentDepth = projCoords.z;
        float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005); 
        float shadow = 0.0;
        vec2 texelSize = 1.0 / textureSize(depthMap, 0);
        for(int x = -1; x <= 1; ++x)
        {
            for(int y = -1; y <= 1; ++y)
            {
                float pcfDepth = texture(depthMap, projCoords.xy + vec2(x, y) * texelSize).r; 
                shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
            }    
        }
        shadow /= 9.0;


        return shadow;
    }

    void main()
    {
        vec3 lightColor = vec3(1.0f, 1.0f, 1.0f);

        vec3 norm = normalize(Normal);
        vec3 lightDir = normalize(lightPos - FragPos);
        float diff = max(dot(norm,lightDir), 0.0);
        vec3 diffuse = diff * lightColor;

        // 環境光 = 背景色に合わせる
        float ambientStrength = 0.7;
        vec3 ambient = ambientStrength * lightColor;

        float shadow = ShadowCalculation(FragPosLightSpace, norm, lightDir);

        vec3 result = (ambient + (1-shadow) * diffuse) * objectColor;
        FragColor = vec4(result, 1.0);
    }  
)";

const char *shadowVertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;

    uniform mat4 lightSpaceMatrix;
    uniform mat4 M;

    void main()
    {
        gl_Position = lightSpaceMatrix * M * vec4(aPos, 1.0);
    }  

)";

const char *shadowFragmentShaderSource = R"(
    #version 330 core

    void main()
    {             
        // gl_FragDepth = gl_FragCoord.z;
    }
)";

int main()
{
    // GLFWの初期化とウィンドウの作成
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    GLFWwindow *window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "OpenGL Cube", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetKeyCallback(window, keyboardCallback);

    // GLEWの初期化
    if (glewInit() != GLEW_OK)
    {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        glfwTerminate();
        return -1;
    }

    std::cout << "initialized" << std::endl;

    Cube *cube1 = new Cube();
    Cube *cube2 = new Cube();
    Grid *grid = new Grid();
    cube2->position = glm::vec3(5.f, 0.f, 0.f);
    Tetrimino *tet0 = new Tetrimino(0);
    Tetrimino *tet1 = new Tetrimino(1);
    Tetrimino *tet2 = new Tetrimino(2);
    Tetrimino *tet3 = new Tetrimino(3);
    Tetrimino *tet4 = new Tetrimino(4);
    Tetrimino *tet5 = new Tetrimino(5);
    Tetrimino *tet6 = new Tetrimino(6);
    tet6->position = glm::vec3(0, 0, -2);
    tet4->position = glm::vec3(0, -3, -1);

    Game *game1 = new Game(); game1->position = glm::vec3(0,0,0);
    CPUGame *game2 = new CPUGame(); game2->position = glm::vec3(18,0,0);
    game1->add();
    game2->add();
    game1->enemyGame = game2;
    game2->enemyGame = game1;

    ShaderProgram program;
    program.addShader(GL_VERTEX_SHADER, vertexShaderSource);
    program.addShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    program.link();

    ShaderProgram shadowProgram;
    shadowProgram.addShader(GL_VERTEX_SHADER, shadowVertexShaderSource);
    shadowProgram.addShader(GL_FRAGMENT_SHADER, shadowFragmentShaderSource);
    shadowProgram.link();
    glm::mat4 ident = glm::mat4(1);

    GLuint depthMapFBO;
    glGenFramebuffers(1, &depthMapFBO);
    const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
    GLuint depthMap;
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    /* シャドウマップの拡大方式の指定 */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    //  glEnable(GL_CULL_FACE);
    //  initOpenGLDebug();

    // メインループ
    double previousTime = glfwGetTime();
    unsigned int stepCounter = 0;
    while (!glfwWindowShouldClose(window))
    {
        double currentTime = glfwGetTime();
        double deltaTime = currentTime - previousTime;
        previousTime = currentTime;
        stepCounter++;

        for (int i = 0; i < 512; i++)
            if (gKeyPressed[i] != 0)
                gKeyPressed[i]++;

        // ウィンドウのクリア
        glClearColor(0.9f, 0.9f, 0.9f, 1.0f);
        // glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (stepCounter % 20 == 0)
        {
            game1->step();
            game2->step();
        }
        else
        {
            game1->update();
            game2->update();
        }

        if (!game1->winFlag || !game2->winFlag) {
            game1->reset();
            game2->reset();
        }

        // tet0->rotation = glm::quat(rotation.x(), rotation.y(), rotation.z(), rotation.w());
        // tet0->position = glm::vec3(position.x(), position.y(), position.z());
        // std::cout << position.y() << std::endl;

        float sensitivity = 0.1f;
        if (gKeyPressed[GLFW_KEY_A] > 0)
        {
            cameraPosition.x -= sensitivity;
        }
        if (gKeyPressed[GLFW_KEY_D] > 0)
        {
            cameraPosition.x += sensitivity;
        }
        if (gKeyPressed[GLFW_KEY_W] > 0)
        {
            cameraPosition += sensitivity * cameraDirection;
        }
        if (gKeyPressed[GLFW_KEY_S] > 0)
        {
            cameraPosition -= sensitivity * cameraDirection;
        }
        if (gKeyPressed[GLFW_KEY_SPACE] > 0)
        {
            cameraPosition.y += sensitivity;
        }
        if (gKeyPressed[GLFW_KEY_LEFT_SHIFT] > 0)
        {
            cameraPosition.y -= sensitivity;
        }

        if (gKeyPressed[GLFW_KEY_U] == 2)
        {
        }

        float near_plane = 0.1f, far_plane = 40.5f;
        // glm::vec3 lightPosition = glm::vec3(0, 0, 5);
        glm::vec3 lightPosition = cameraPosition;
        glm::vec3 lightDirection = cameraDirection;
        // glm::vec3 lightPosition = glm::vec3(6, 20, 5);
        // glm::vec3 lightDirection = glm::vec3(0, -1, -0.5f);
        glm::mat4 lightProjection = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, near_plane, far_plane);
        glm::mat4 lightView = glm::lookAt(lightPosition, lightPosition + lightDirection, worldUp);
        glm::mat4 lightSpaceMatrix = lightProjection * lightView;

        // 1. first render to depth map
        shadowProgram.use();
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        unsigned int location = shadowProgram.getLocation("lightSpaceMatrix");
        glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
        game1->render(&shadowProgram, ident, ident, ident);
        game2->render(&shadowProgram, ident, ident, ident);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        program.use();
        glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glm::mat4 pers = glm::perspective(glm::radians(45.f), (float)(WINDOW_WIDTH) / WINDOW_HEIGHT, 0.1f, 1000.0f);
        glm::mat4 view = glm::lookAt(cameraPosition, cameraPosition + cameraDirection, worldUp);
        glUniform3fv(program.getLocation("lightPos"), 1, glm::value_ptr(cameraPosition));
        glUniformMatrix4fv(program.getLocation("lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
        glBindTexture(GL_TEXTURE_2D, depthMap);
        // tet1.render(&program, ident, pers, view);
        game1->render(&program, ident, pers, view);
        game2->render(&program, ident, pers, view);
        // grid->render(&program, ident, pers, view);
        // cube2.render(&program, ident, pers, view);

        // ダブルバッファリング
        glfwSwapBuffers(window);

        // イベントのポーリング
        glfwPollEvents();
    }

    delete grid, cube1, cube2, tet0, tet1, tet2, tet3, tet4, tet5, tet6, game1, game2;

    // GLFWの終了処理
    glfwTerminate();

    return 0;
}
