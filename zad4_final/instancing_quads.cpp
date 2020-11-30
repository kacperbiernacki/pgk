#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include "learnopengl/shader.h"
#include "learnopengl/camera.h"

#include <iostream>
#include <stdlib.h>
#include <vector>
#include <math.h>
#include <unistd.h>
#include <time.h>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
void makeSphere(float radius, float sectorCount, float stackCount, std::vector<float>& vertices, std::vector<float>& normals, std::vector<unsigned int>& indices);
void drawScene();

// settings
const unsigned int SCR_WIDTH = 1600;
const unsigned int SCR_HEIGHT = 900;

// camera
Camera camera(glm::vec3(0.9f, 0.9f, 0.9f));
float lastX = (float)SCR_WIDTH / 2.0;
float lastY = (float)SCR_HEIGHT / 2.0;
bool firstMouse = true;

// camera 2,3,4
Camera camera2(glm::vec3(3.0f, 0.0f, 0.0f));
Camera camera3(glm::vec3(0.01f, 3.0f, 0.0f));
Camera camera4(glm::vec3(0.0f, 0.0f, 3.0f));

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

glm::vec3 sphereMove = glm::vec3(0.9, 0.9, 0.0);

bool mouseCamera = false;

int keyClicked = 0;
bool debugMode = true;
bool multiScreenMode = true;
bool endGame = false;

struct Edge3
{

    glm::vec3 A;
    glm::vec3 B;
    glm::vec3 Delta;

    float LengthSquared;

    Edge3(){};

    Edge3(glm::vec3 a, glm::vec3 b)
    {
        A = a;
        B = b;
        Delta = b - a;
        LengthSquared = glm::length2(Delta);
    }

    glm::vec3 PointAt(float t)
    {
        return A + t * Delta;
    }

    float Project(glm::vec3 p)
    {
        return glm::dot(p-A, Delta) / LengthSquared;
    }
};

struct Plane
{
    glm::vec3 Point;
    glm::vec3 Direction;

    Plane(){};

    Plane(glm::vec3 point, glm::vec3 direction )
    {
        Point = point;
        Direction = direction;
    }

    bool IsAbove(glm::vec3 q)
    {
        return glm::dot(Direction, q - Point) > 0;
    }

    glm::vec3 Project(glm::vec3 p)
    {
        glm::vec3 normalizedDirecion = glm::normalize(Direction);
        return p - (glm::dot(p, normalizedDirecion) - glm::dot(Point, normalizedDirecion)) * normalizedDirecion;
    }
};

struct Triangle
{
    Edge3 EdgeAb;
    Edge3 EdgeBc;
    Edge3 EdgeCa;

    glm::vec3 A;
    glm::vec3 B;
    glm::vec3 C;

    glm::vec3 TriNorm;

    Plane TriPlane;
    Plane PlaneAb;
    Plane PlaneBc;
    Plane PlaneCa;

    Triangle(glm::vec3 a, glm::vec3 b, glm::vec3 c)
    {
        EdgeAb = Edge3( a, b );
        EdgeBc = Edge3( b, c );
        EdgeCa = Edge3( c, a );
        TriNorm = glm::cross(a - b, a - c);

        A = a;
        B = b;
        C = c;

        PlaneAb = Plane(a, glm::cross(TriNorm, EdgeAb.Delta ));
        PlaneBc = Plane(b, glm::cross(TriNorm, EdgeBc.Delta ));
        PlaneCa = Plane(c, glm::cross(TriNorm, EdgeCa.Delta ));

        TriPlane = Plane(A, TriNorm);
    }

    glm::vec3 ClosestPointTo(glm::vec3 p)
    {
        float uab = EdgeAb.Project( p );
        float uca = EdgeCa.Project( p );

        if (uca > 1 && uab < 0)
            return A;

        float ubc = EdgeBc.Project( p );

        if (uab > 1 && ubc < 0)
            return B;

        if (ubc > 1 && uca < 0)
            return C;

        if (uab >= 0 && uab <= 1 && !PlaneAb.IsAbove( p ))
            return EdgeAb.PointAt( uab );

        if (ubc >= 0 && ubc <= 1 && !PlaneBc.IsAbove( p ))
            return EdgeBc.PointAt( ubc );

        if (uca >= 0 && uca <= 1 && !PlaneCa.IsAbove( p ))
            return EdgeCa.PointAt( uca );

        return TriPlane.Project( p );
    }
};

int main( int argc, char** argv )
{
    int seed = 0;
    int N = 0;

    switch (argc)
    {
        case 2:
            seed = atoi(argv[1]);
            N = 10;
            break;

        case 3:
            seed = atoi(argv[1]);
            N = atoi(argv[2]);
            break;

        default:
            seed = 0;
            N = 10;
            break;
    }

    srand(seed);
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Initialize GLEW
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
    }

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // build and compile shaders
    // -------------------------
    Shader shader("10.1.instancing.vs", "10.1.instancing.fs");
    Shader sphereShader("sphereShader.vs", "sphereShader.fs");
    Shader cubeShader("cubeShader.vs", "cubeShader.fs");

    // ============================================================ trojkaty
    // ---------------------------------------------------------
    std::vector<Triangle> triangles;
    glm::vec3 baseX = glm::vec3(-0.05f,  0.05f, 0.0f);
    glm::vec3 baseY = glm::vec3( 0.05f, -0.05f, 0.0f);
    glm::vec3 baseZ = glm::vec3(-0.05f, -0.05f, 0.0f);
    glm::mat3 baseTriangle = glm::mat3(baseX, baseY, baseZ);

    glm::vec3 translations[N*N*N];
    glm::quat myQuat;
    glm::vec4 rotations[N*N*N];
    int index = 0;
    float offset = 1.0f/N;
    for (int z = -N; z < N; z+=2)
    {
        for (int y = -N; y < N; y += 2)
        {
            for (int x = -N; x < N; x += 2)
            {
                glm::vec3 translation;
                translation.x = (float)x / N + offset;
                translation.y = (float)y / N + offset;
                translation.z = (float)z / N + offset;
                translations[index] = translation;

                float rotx = rand() % 360;
                float roty = rand() % 360;
                float rotz = rand() % 360;
                myQuat = glm::quat(glm::vec3(glm::radians(rotx), glm::radians(roty), glm::radians(rotz)));
                rotations[index++] = glm::vec4(myQuat.x, myQuat.y, myQuat.z, myQuat.w);
                
                //glm::mat3 rotationMat = glm::toMat3(myQuat);

                glm::vec3 v_x = glm::vec3( 1.0f - (2.0f*(myQuat.y*myQuat.y)) - (2.0f*(myQuat.z*myQuat.z)),
                2.0f*myQuat.x*myQuat.y-2.0f*myQuat.z*myQuat.w,
                2.0f*myQuat.x*myQuat.z+2*myQuat.y*myQuat.w);

                glm::vec3 v_y = glm::vec3( 2.0f*myQuat.x*myQuat.y+2.0f*myQuat.z*myQuat.w, 
                1-(2.0f*(myQuat.x*myQuat.x))-(2.0f*(myQuat.z*myQuat.z)),
                2.0f*myQuat.y*myQuat.z-2*myQuat.x*myQuat.w);

                glm::vec3 v_z = glm::vec3( 2.0f*myQuat.x*myQuat.z-2.0f*myQuat.y*myQuat.w,
                2.0f*myQuat.y*myQuat.z+2.0f*myQuat.x*myQuat.w,
                1.0f-(2.0f*(myQuat.x*myQuat.x))-(2.0f*(myQuat.y*myQuat.y)));

                glm::mat3 rotationMat = glm::mat3(v_x, v_y, v_z);

                glm::mat3 tempBaseTriangle = baseTriangle;
                tempBaseTriangle = rotationMat * tempBaseTriangle;
                triangles.push_back(Triangle(translation + tempBaseTriangle[0], translation + tempBaseTriangle[1], translation + tempBaseTriangle[2]));
                //triangles.push_back(Triangle(translation + baseX, translation + baseY, translation + baseZ));
            }
        }
    }

    triangles.pop_back();

    std::cout << triangles.size() << std::endl;

    // store instance data in an array buffer
    // --------------------------------------
    unsigned int instanceVBO;
    glGenBuffers(1, &instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * (N*N*N - 1), &translations[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    unsigned int instanceVBO2;
    glGenBuffers(1, &instanceVBO2);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO2);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * (N*N*N - 1), &rotations[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 1);

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    float quadVertices[] = {
        // positions          // colors
        -0.05f,  0.05f, 0.0f, 1.0f, 0.0f, 0.0f,
         0.05f, -0.05f, 0.0f, 0.0f, 1.0f, 0.0f,
        -0.05f, -0.05f, 0.0f, 0.0f, 0.0f, 1.0f
    };

    unsigned int quadVAO, quadVBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

    // also set instance data
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO); // this attribute comes from a different vertex buffer
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glVertexAttribDivisor(2, 1); // tell OpenGL this is an instanced vertex attribute.

    glEnableVertexAttribArray(3);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO2); // this attribute comes from a different vertex buffer
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glBindBuffer(GL_ARRAY_BUFFER, 1);
    glVertexAttribDivisor(3, 1); // tell OpenGL this is an instanced vertex attribute.

    // ============================================================ trojkaty end

    // ============================================================ sphere
    std::vector<float> vertices;
    std::vector<float> normals;
    std::vector<unsigned int> indices;
    float sphereRadius = 0.05f * (10.0f/N);
    makeSphere(sphereRadius, 32, 32, vertices, normals, indices);
    
    float* verticesArray = &vertices[0];
    unsigned int* indicesArray = &indices[0];

    unsigned int vaoId, vboId, iboId;
    glGenVertexArrays(1, &vaoId);
    glGenBuffers(1, &vboId);
    glBindVertexArray(vaoId);
    glBindBuffer(GL_ARRAY_BUFFER, vboId);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), verticesArray, GL_STATIC_DRAW);

    glGenBuffers(1, &iboId);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboId);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indicesArray, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

    // ============================================================ sphere end

    // ============================================================ cube

    float cubeVertices[] = {
        -1.0f,  1.0f, -1.0f, 1.0f, 0.0f, 0.0f, //A 0
        -1.0f,  1.0f,  1.0f, 0.5f, 0.5f, 0.0f, //B 1
         1.0f,  1.0f,  1.0f, 0.0f, 0.5f, 0.5f, //C 2
         1.0f,  1.0f, -1.0f, 0.5f, 0.5f, 0.0f, //D 3
        -1.0f, -1.0f, -1.0f, 0.5f, 0.5f, 0.0f, //E 4
        -1.0f, -1.0f,  1.0f, 0.0f, 0.5f, 0.5f, //F 5
         1.0f, -1.0f,  1.0f, 0.0f, 0.0f, 1.0f, //G 6
         1.0f, -1.0f, -1.0f, 0.0f, 0.5f, 0.5f  //H 7
    };

    unsigned int cubeIndices[] = {
        0,2,1,
        0,1,5,
        0,4,5,
        0,3,7,
        0,7,4,
        0,3,2,
        6,5,4,
        6,4,7,
        6,7,3,
        6,3,2,
        6,2,1,
        6,1,5
    };

    unsigned int cubeVao, cubeVbo, cubeIbo;
    glGenVertexArrays(1, &cubeVao);
    glGenBuffers(1, &cubeVbo);
    glBindVertexArray(cubeVao);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVbo);
    glBufferData(GL_ARRAY_BUFFER, 8 * 6 * sizeof(float), cubeVertices, GL_STATIC_DRAW);

    glGenBuffers(1, &cubeIbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeIbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 12 * 3 * sizeof(unsigned int), cubeIndices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    // ============================================================ cube end

    glfwSetCursorPos(window, SCR_WIDTH/2, SCR_HEIGHT/2);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetWindowSizeCallback(window, framebuffer_size_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // fps limiter variables declaration
    double fpsTime = 0;
    const double fps = 60.0;

    double keyTime = glfwGetTime();

    double playTime = glfwGetTime();

    // render loop
    // -----------
    do {
        fpsTime = glfwGetTime();

        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        int winWidth = 0;
        int winHeight = 0;
        glfwGetWindowSize(window, &winWidth, &winHeight);

        if (multiScreenMode)
            glViewport(-winWidth/4, 0, winWidth, winHeight);
        else
            glViewport(0, 0, winWidth, winHeight);
        

        // input
        // -----
        glm::vec3 cameraLastPos = camera.getPosition();
        processInput(window);

        // render
        // ------
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // configure transformation matrices
        camera.setPosition(camera.getPosition() - (camera.GetFront() / glm::vec3(3, 3, 3)));
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);
        glm::mat4 view = camera.GetViewMatrix();
        camera.setPosition(camera.getPosition() + camera.GetFront() / glm::vec3(3, 3, 3));

        // draw N*N*N instanced teriangles
        shader.use();
        shader.setMat4("projection", projection);
        shader.setMat4("view", view);

        glBindVertexArray(quadVAO);
        glDrawArraysInstanced(GL_TRIANGLES, 0, 3, N*N*N); // 1000 triangles of 6 vertices each
        glBindVertexArray(0);

        //draw sphere
        glBindVertexArray(vaoId);
        glBindBuffer(GL_ARRAY_BUFFER, vboId);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboId);
        sphereShader.use();
        sphereShader.setMat4("projection", projection);
        sphereShader.setMat4("view", view);
        sphereShader.setVec3("move", sphereMove);
        sphereShader.setFloat("scale", 1);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, (void*)0);
        glBindVertexArray(0);

        //draw cube
        glBindVertexArray(cubeVao);
        glBindBuffer(GL_ARRAY_BUFFER, cubeVbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeIbo);
        cubeShader.use();
        cubeShader.setMat4("projection", projection);
        cubeShader.setMat4("view", view);
        glDrawElements(GL_TRIANGLES, 12*3, GL_UNSIGNED_INT, (void*)0);
        glBindVertexArray(0);

        //check collisions and draw closest point
        if (debugMode)
        {
            glBindVertexArray(vaoId);
            glBindBuffer(GL_ARRAY_BUFFER, vboId);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboId);
            sphereShader.use();
            sphereShader.setMat4("projection", projection);
            sphereShader.setMat4("view", view);
            sphereShader.setFloat("scale", 0.1);
        }

        for(int i = 0; i < triangles.size(); i++)
        {
            glm::vec3 closestPoint = triangles[i].ClosestPointTo(sphereMove);

            if (debugMode)
            {
                sphereShader.setVec3("move", closestPoint);
                glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, (void*)0);
            }
            
            if (glm::distance2(closestPoint, sphereMove) < sphereRadius * sphereRadius)
            {
                camera.setPosition(cameraLastPos);
                //std::cout << "collision" << std::endl;

                if (i == 0)
                {
                    std::cout << "win" << std::endl;
                    endGame = true;
                    std::cout << "play time: " << glfwGetTime() - playTime << "s" << std::endl;
                }                
            }
        }

        if (sphereMove.x >= (1.0f - sphereRadius) || sphereMove.x <= -(1.0f - sphereRadius) ||
            sphereMove.y >= (1.0f - sphereRadius) || sphereMove.y <= -(1.0f - sphereRadius) ||
            sphereMove.z >= (1.0f - sphereRadius) || sphereMove.z <= -(1.0f - sphereRadius))
        {
            camera.setPosition(cameraLastPos);
        }
        

        glBindVertexArray(0);

        if (multiScreenMode)
        {
            glDisable(GL_DEPTH_TEST);
            // small view 1
            glViewport((winWidth/4) * 3, (winHeight/4) * 3, winWidth/4, winHeight/4);

            projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 2000.0f);
            view = camera2.GetViewMatrix(glm::vec3(0,0,0));

            // draw 1000 instanced teriangles
            shader.use();
            shader.setMat4("projection", projection);
            shader.setMat4("view", view);

            glBindVertexArray(quadVAO);
            glDrawArraysInstanced(GL_TRIANGLES, 0, 3, N*N*N); // 1000 triangles of 6 vertices each
            glBindVertexArray(0);

            //draw sphere
            glBindVertexArray(vaoId);
            glBindBuffer(GL_ARRAY_BUFFER, vboId);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboId);
            sphereShader.use();
            sphereShader.setMat4("projection", projection);
            sphereShader.setMat4("view", view);
            sphereShader.setVec3("move", sphereMove);
            sphereShader.setFloat("scale", 1);
            glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, (void*)0);
            glBindVertexArray(0);

            // small view 2
            glViewport((winWidth/4) * 3, (winHeight/4) * 2, winWidth/4, winHeight/4);

            projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 2000.0f);
            view = camera3.GetViewMatrix(glm::vec3(0,0,0));

            // draw 1000 instanced teriangles
            shader.use();
            shader.setMat4("projection", projection);
            shader.setMat4("view", view);

            glBindVertexArray(quadVAO);
            glDrawArraysInstanced(GL_TRIANGLES, 0, 3, N*N*N); // 1000 triangles of 6 vertices each
            glBindVertexArray(0);

            //draw sphere
            glBindVertexArray(vaoId);
            glBindBuffer(GL_ARRAY_BUFFER, vboId);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboId);
            sphereShader.use();
            sphereShader.setMat4("projection", projection);
            sphereShader.setMat4("view", view);
            sphereShader.setVec3("move", sphereMove);
            sphereShader.setFloat("scale", 1);
            glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, (void*)0);
            glBindVertexArray(0);

            // small view 3
            glViewport((winWidth/4) * 3, (winHeight/4) * 1, winWidth/4, winHeight/4);

            projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 2000.0f);
            view = camera4.GetViewMatrix(glm::vec3(0,0,0));

            // draw 1000 instanced teriangles
            shader.use();
            shader.setMat4("projection", projection);
            shader.setMat4("view", view);

            glBindVertexArray(quadVAO);
            glDrawArraysInstanced(GL_TRIANGLES, 0, 3, N*N*N); // 1000 triangles of 6 vertices each
            glBindVertexArray(0);

            //draw sphere
            glBindVertexArray(vaoId);
            glBindBuffer(GL_ARRAY_BUFFER, vboId);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboId);
            sphereShader.use();
            sphereShader.setMat4("projection", projection);
            sphereShader.setMat4("view", view);
            sphereShader.setVec3("move", sphereMove);
            sphereShader.setFloat("scale", 1);
            glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, (void*)0);
            glBindVertexArray(0);
            
            glEnable(GL_DEPTH_TEST);
        }

        double fpsElapsedTime = glfwGetTime() - fpsTime;
        usleep((100/fps - fpsElapsedTime) * 1000);

        if (glfwGetTime() - keyTime > 1)
        {
            keyClicked = 0;
            keyTime = glfwGetTime();
        }
        

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
                glfwWindowShouldClose(window) == 0 &&
                !endGame);

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &quadVBO);

    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    std::vector<glm::vec3> move;

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    {
        move = camera.ProcessKeyboard(FORWARD, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)
    {
        move = camera.ProcessKeyboard(BACKWARD, deltaTime);
    }
    /*if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    {
        move = camera.ProcessKeyboard(LEFT, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    {
        move = camera.ProcessKeyboard(RIGHT, deltaTime);
    }*/
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
    {
        camera.ProcessMouseMovement(0, 2);
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
    {
        camera.ProcessMouseMovement(0, -2);
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
    {
        camera.ProcessMouseMovement(-2, 0);
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
    {
        camera.ProcessMouseMovement(2, 0);
    }
    if (!move.empty())
    {
        sphereMove = sphereMove + move[0] + move[1];
    }
    sphereMove = camera.getPosition();

    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS && keyClicked != 1)
    {
        if (mouseCamera)
            mouseCamera = false;
        else
            mouseCamera = true;

        keyClicked = 1;
    }

    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS && keyClicked != 2)
    {
        if (debugMode)
            debugMode = false;
        else
            debugMode = true;

        keyClicked = 2;
    }

    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS && keyClicked != 3)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        keyClicked = 3;
    }

    if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS && keyClicked != 4)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        keyClicked = 4;
    }

    if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS && keyClicked != 5)
    {
        if (multiScreenMode)
            multiScreenMode = false;
        else
            multiScreenMode = true;
        
        keyClicked = 5;
    }
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (mouseCamera)
    {
        int winWidth = 0;
        int winHeight = 0;
        glfwGetWindowSize(window, &winWidth, &winHeight);
        /*if (firstMouse)
        {
            lastX = xpos;
            lastY = ypos;
            firstMouse = false;
        }*/

        bool screenEdge = false;
        double screenEdgeLastX = 0;
        double screenEdgeLasty = 0;

        int mouseBorders = 50;

        if (xpos < mouseBorders)
        {
            glfwSetCursorPos(window, winWidth - mouseBorders, ypos);
            screenEdge = true;
            screenEdgeLastX = winWidth - mouseBorders;
        }
        if (xpos > winWidth - mouseBorders)
        {
            glfwSetCursorPos(window, mouseBorders, ypos);
            screenEdge = true;
            screenEdgeLastX = mouseBorders;
        }
        if (ypos < mouseBorders)
        {
            glfwSetCursorPos(window, xpos, winHeight - mouseBorders);
            screenEdge = true;
            screenEdgeLasty = winHeight - mouseBorders;
        }
        if (ypos > winHeight - mouseBorders)
        {
            glfwSetCursorPos(window, xpos, mouseBorders);
            screenEdge = true;
            screenEdgeLasty = mouseBorders;
        }
        

        if (!screenEdge)
        {
            float xoffset = xpos - lastX;
            float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

            lastX = xpos;
            lastY = ypos;
            camera.ProcessMouseMovement(xoffset, yoffset);
        }
        else
        {
            if (screenEdgeLastX == 0)
            {
                lastX = xpos;
            }
            else
            {
                lastX = screenEdgeLastX;
            }
            if (screenEdgeLasty == 0)
            {
                lastY = ypos;
            }
            else
            {
                lastY = screenEdgeLasty;
            }
        }
    }
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(yoffset);
}

void makeSphere(float radius, float sectorCount, float stackCount,
                std::vector<float>& vertices, std::vector<float>& normals, std::vector<unsigned int>& indices)
{
    float x, y, z, xy;                              // vertex position
    float nx, ny, nz, lengthInv = 1.0f / radius;    // vertex normal

    float sectorStep = 2 * M_PI / sectorCount;
    float stackStep = M_PI / stackCount;
    float sectorAngle, stackAngle;
    float hue = 0.0;
    float hueIncrement = 0.01;

    for (int i = 0; i <= stackCount; ++i)
    {
        stackAngle = M_PI / 2 - i * stackStep;        // starting from pi/2 to -pi/2
        xy = radius * cosf(stackAngle);             // r * cos(u)
        z = radius * sinf(stackAngle);              // r * sin(u)

        // add (sectorCount+1) vertices per stack
        // the first and last vertices have same position and normal, but different tex coords
        for(int j = 0; j <= sectorCount; ++j)
        {
            sectorAngle = j * sectorStep;           // starting from 0 to 2pi

            // vertex position (x, y, z)
            x = xy * cosf(sectorAngle);             // r * cos(u) * cos(v)
            y = xy * sinf(sectorAngle);             // r * cos(u) * sin(v)
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);

            //colors
            vertices.push_back(hue);
            vertices.push_back(1.0);
            vertices.push_back(1.0);
            hue += hueIncrement;
            if (hue >= 1.0)
                hueIncrement = (-hueIncrement);
            if (hue <= 0.0)
                hueIncrement = (-hueIncrement);
            
            

            // normalized vertex normal (nx, ny, nz)
            nx = x * lengthInv;
            ny = y * lengthInv;
            nz = z * lengthInv;
            normals.push_back(nx);
            normals.push_back(ny);
            normals.push_back(nz);
        }
    }    
    
    // generate CCW index list of sphere triangles
    int k1, k2;
    for(int i = 0; i < stackCount; ++i)
    {
        k1 = i * (sectorCount + 1);     // beginning of current stack
        k2 = k1 + sectorCount + 1;      // beginning of next stack

        for(int j = 0; j < sectorCount; ++j, ++k1, ++k2)
        {
            // 2 triangles per sector excluding first and last stacks
            // k1 => k2 => k1+1
            if(i != 0)
            {
                indices.push_back(k1);
                indices.push_back(k2);
                indices.push_back(k1 + 1);
            }

            // k1+1 => k2 => k2+1
            if(i != (stackCount-1))
            {
                indices.push_back(k1 + 1);
                indices.push_back(k2);
                indices.push_back(k2 + 1);
            }
        }
    }
}
