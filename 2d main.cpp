#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <cmath>

#include "../Header/Util.h"

// =============================
//  MEHANIKA KANDŽE – VARIJABLE
// =============================

// Kandža
float clawX = 0.0f;
float ropeLength = 0.40f;
float clawY = 0.90f - 0.40f - 0.25f;

bool isDropping = false;
bool isReturning = false;
bool mouseWasPressed = false;  // EDGE TRIGGER ZA MIŠ

const float dropSpeed = 0.35f;
const float clawSpeed = 0.50f;

// Dno kutije
const float bottomY = -0.30f;
const float bottomToyY = bottomY + 0.10f;

// Igračke
float toy1X = -0.25f, toy1Y = -0.15f;
float toy2X = 0.22f, toy2Y = -0.15f;
const float toySize = 0.20f;

bool grabbedToy1 = false;
bool grabbedToy2 = false;
bool holdingToy = false;

// Fizika padanja
bool toy1Falling = false, toy2Falling = false;
float toy1Vy = 0.0f, toy2Vy = 0.0f;
bool toy1Won = false, toy2Won = false;
bool toy1OnShelf = false, toy2OnShelf = false;
bool toy1Collected = false, toy2Collected = false;

const float gravity = 1.5f;

// Rupa & polica
const float holeX = 0.37f;
const float holeY = -0.30f;
const float holeRx = 0.10f;
const float holeRy = 0.08f;

const float shelfX = 0.32f;
const float shelfY = -0.50f;

// Igra / lampica
bool gameRunning = false;
bool lightOn = false;

bool blinkActive = false;
float blinkTimer = 0.0f;
bool blinkGreen = true;

// =============================
// EDGE-TRIGGER ZA TASTER S
// =============================
bool sWasPressed = false;
bool sPressedEvent = false;

// ============================================================
// PROVERA KOLIZIJE
// ============================================================
bool checkCollision(float ax, float ay, float bx, float by, float range)
{
    return (std::fabs(ax - bx) < range && std::fabs(ay - by) < range);
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // === FULL SCREEN PROZOR ===
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);

    int screenWidth = mode->width;
    int screenHeight = mode->height;

    GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight,
        "Kandza - 2D", monitor, NULL);

    if (!window) return endProgram("Prozor nije uspeo da se kreira.");

    glfwMakeContextCurrent(window);

    // === VIEWPORT KOJI NE RAZVLAČKI SCENU ===
    glViewport(0, 0, screenWidth, screenHeight);

    float aspect = (float)screenWidth / (float)screenHeight;

    if (aspect > 1.0f) {
        glViewport((screenWidth - screenHeight) / 2, 0, screenHeight, screenHeight);
    }
    else {
        glViewport(0, (screenHeight - screenWidth) / 2, screenWidth, screenWidth);
    }

    if (!window) return endProgram("Prozor nije uspeo da se kreira.");
    glfwMakeContextCurrent(window);

    // =======================
    //    CUSTOM CURSORS
    // =======================

    GLFWcursor* cursorCoin = loadImageToCursor("Resources/coin.png");
    GLFWcursor* cursorLever = loadImageToCursor("Resources/poluga.png");

    glfwSetCursor(window, cursorCoin);

    if (glewInit() != GLEW_OK) return endProgram("GLEW init fail.");

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);

    // === SHADERI
    unsigned int shader = createShader("basic.vert", "basic.frag");
    glUseProgram(shader);

    int locTex = glGetUniformLocation(shader, "uTex");
    int locOffset = glGetUniformLocation(shader, "uOffset");
    int locScale = glGetUniformLocation(shader, "uScale");
    int locColor = glGetUniformLocation(shader, "uColor");
    int locUseTex = glGetUniformLocation(shader, "uUseTexture");
    int signatureTex = loadImageToTexture("Resources/signature.png");


    glUniform1i(locTex, 0);

    // === PRAVOUGAONIK VAO
    float quadVertices[] =
    {
        -0.5f,  0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.0f, 0.0f,
         0.5f, -0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  1.0f, 1.0f
    };

    unsigned int indices[] = { 0,1,2, 0,2,3 };

    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices),
        quadVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices),
        indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
        4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
        4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    // === KRUG ZA LAMPICU I RUPU
    const int segments = 64;
    float circleVerts[(segments + 2) * 2];

    circleVerts[0] = 0;
    circleVerts[1] = 0;

    for (int i = 0; i <= segments; i++)
    {
        float a = i * (6.283185f / segments);
        circleVerts[(i + 1) * 2] = std::cos(a);
        circleVerts[(i + 1) * 2 + 1] = std::sin(a);
    }

    unsigned int circleVAO, circleVBO;
    glGenVertexArrays(1, &circleVAO);
    glGenBuffers(1, &circleVBO);

    glBindVertexArray(circleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, circleVBO);

    glBufferData(GL_ARRAY_BUFFER, sizeof(circleVerts),
        circleVerts, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
        2 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    // === END 1/3 ===
    // === TEKSTURE
    unsigned int toy1Tex = loadImageToTexture("Resources/toy1.png");
    unsigned int toy2Tex = loadImageToTexture("Resources/toy2.png");
    unsigned int closedClawTex = loadImageToTexture("Resources/closed-claw.png");
    unsigned int openClawTex = loadImageToTexture("Resources/claw.jpg");
    unsigned int ropeTex = loadImageToTexture("Resources/rope.jpg");
    unsigned int slotTex = loadImageToTexture("Resources/slot.jpg");

    double lastTime = glfwGetTime();

    // ============================== MAIN LOOP ==============================
    while (!glfwWindowShouldClose(window))
    {
        double now = glfwGetTime();
        float dt = float(now - lastTime);
        lastTime = now;

        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(shader);

        // ============================================
        // EDGE TRIGGER ZA TASTER S (klik, ne držanje)
        // ============================================
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        {
            if (!sWasPressed)
            {
                sPressedEvent = true; // desio se "klik" na S
                sWasPressed = true;
            }
        }
        else
        {
            sWasPressed = false;
        }

        // =========================================================
        //  LEVI KLIK MIŠA – EDGE TRIGGER (samo jednom po kliku!)
        // =========================================================
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
        {
            if (!mouseWasPressed)  // SAMO NA POČETKU KLIKA!
            {
                double mx, my;
                glfwGetCursorPos(window, &mx, &my);

                float nx = float(mx / screenWidth) * 2.0f - 1.0f;
                float ny = 1.0f - float(my / screenHeight) * 2.0f;

                // 1) Klik na osvojenu igračku na polici
                if (blinkActive && (toy1OnShelf || toy2OnShelf))
                {
                    if (nx > shelfX - 0.15f && nx < shelfX + 0.15f &&
                        ny > shelfY - 0.15f && ny < shelfY + 0.15f)
                    {
                        if (toy1OnShelf) { toy1OnShelf = false; toy1Collected = true; }
                        if (toy2OnShelf) { toy2OnShelf = false; toy2Collected = true; }

                        // ✅ NASILNI RESET SVE!
                        lightOn = false;
                        gameRunning = false;
                        blinkActive = false;
                        blinkTimer = 0.0f;
                        blinkGreen = true;

                        // Reset kandže i igračaka
                        ropeLength = 0.40f;
                        clawY = 0.90f - 0.40f - 0.25f;
                        isDropping = false;
                        isReturning = false;

                        toy1Won = toy2Won = false;
                        toy1Falling = toy2Falling = false;
                        toy1Vy = toy2Vy = 0.0f;
                        holdingToy = false;
                        grabbedToy1 = grabbedToy2 = false;

                        glfwSetCursor(window, cursorCoin);
                    }
                }
                // 2) Klik na SLOT – start igre
                else if (!gameRunning && !blinkActive)
                {
                    if (nx > -0.25f && nx < 0.25f &&
                        ny > -0.80f && ny < -0.45f)
                    {
                        lightOn = true;
                        gameRunning = true;
                        isDropping = false;
                        isReturning = false;
                        holdingToy = false;
                        grabbedToy1 = grabbedToy2 = false;
                        ropeLength = 0.40f;
                        clawY = 0.90f - ropeLength - 0.25f;
                    }
                }

                mouseWasPressed = true;  // Sprečava ponavljanje u istom kliku
            }
        }
        else
        {
            mouseWasPressed = false;  // Reset kada nema klika
        }

        // =========================================================
        //   UPDATE BLINKA LAMPICE (KADA JE NAGRADA OSVOJENA)
        // =========================================================
        if (blinkActive)
        {
            blinkTimer += dt;
            if (blinkTimer >= 0.5f)
            {
                blinkTimer = 0.0f;
                blinkGreen = !blinkGreen;
            }
        }

        // =========================================================
        //      KURSOR SE POSTAVLJA PREMA lightOn
        // =========================================================
        if (!lightOn)
            glfwSetCursor(window, cursorCoin);
        else
            glfwSetCursor(window, cursorLever);

        // =========================================================
        //      MEHANIKA KANDŽE – SAMO KAD JE IGRA AKTIVNA
        // =========================================================

        if (gameRunning)
        {
            // --- A / D pomeranje (kada se ne spušta) ---
            if (!isDropping)
            {
                if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
                    clawX -= clawSpeed * dt;
                if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
                    clawX += clawSpeed * dt;
            }

            // UVEK nosi igračku kad je držimo
            if (holdingToy)
            {
                if (grabbedToy1)
                {
                    toy1X = clawX;
                    toy1Y = clawY - 0.10f;
                }
                if (grabbedToy2)
                {
                    toy2X = clawX;
                    toy2Y = clawY - 0.10f;
                }
            }

            // ograničenje da kandža ostane u širini kutije
            clawX = std::fmax(-0.35f, std::fmin(0.35f, clawX));

            // ========================================================
            //  LOGIKA ZA TASTER S: RADI SAMO KAD JE KANDŽA GORE
            // ========================================================
            if (sPressedEvent)
            {
                sPressedEvent = false; // potroši event

                bool clawAtTop = !isDropping && !isReturning &&
                    std::fabs(ropeLength - 0.40f) < 0.0001f;

                if (clawAtTop)
                {
                    // DRŽIMO IGRAČKU → PUŠTANJE
                    if (holdingToy)
                    {
                        if (grabbedToy1)
                        {
                            toy1Falling = true;
                            toy1Vy = 0.0f;
                        }
                        if (grabbedToy2)
                        {
                            toy2Falling = true;
                            toy2Vy = 0.0f;
                        }

                        holdingToy = false;
                        grabbedToy1 = grabbedToy2 = false;
                    }
                    else
                    {
                        // NE DRŽIMO IGRAČKU → SPUŠTANJE KANDŽE
                        isDropping = true;
                    }
                }
            }

            // ----------------------------
            //   SPUŠTANJE KANDŽE
            // ----------------------------
            if (isDropping)
            {
                ropeLength += dropSpeed * dt;
                clawY = 0.90f - ropeLength - 0.25f;

                // kolizija sa dnom
                if (clawY <= -0.15f)
                {
                    clawY = bottomY;
                    isDropping = false;
                    isReturning = true;
                }

                // kolizija sa igračkom 1
                if (!holdingToy && !toy1Collected &&
                    checkCollision(clawX, clawY, toy1X, toy1Y, toySize))
                {
                    holdingToy = true;
                    grabbedToy1 = true;
                    isDropping = false;
                    isReturning = true;
                }

                // kolizija sa igračkom 2
                if (!holdingToy && !toy2Collected &&
                    checkCollision(clawX, clawY, toy2X, toy2Y, toySize))
                {
                    holdingToy = true;
                    grabbedToy2 = true;
                    isDropping = false;
                    isReturning = true;
                }
            }

            // ----------------------------
            //      POVRATAK NAGORE
            // ----------------------------
            if (isReturning)
            {
                ropeLength -= dropSpeed * dt;

                if (ropeLength <= 0.40f)
                {
                    ropeLength = 0.40f;
                    isReturning = false;
                }

                if (grabbedToy1)
                {
                    toy1X = clawX;
                    toy1Y = clawY - 0.10f;
                }
                if (grabbedToy2)
                {
                    toy2X = clawX;
                    toy2Y = clawY - 0.10f;
                }

                clawY = 0.90f - ropeLength - 0.25f;
            }
        }
        // =========================================================
        //      PADANJE IGRAČAKA (POSLE PUŠTANJA S)
        // =========================================================

        // -------------------------
        // TOY 1 – padanje
        // -------------------------
        if (toy1Falling)
        {
            toy1Vy -= gravity * dt;
            toy1Y += toy1Vy * dt;

            // upada u rupu
            if (!toy1Won &&
                std::fabs(toy1X - holeX) < holeRx &&
                std::fabs(toy1Y - holeY) < holeRy)
            {
                toy1Won = true;
                toy1Falling = true;
                gameRunning = false;
            }

            // udara dno ako nije osvojena
            if (!toy1Won &&
                toy1Y <= -0.15f &&
                !(toy1X > holeX - holeRx && toy1X < holeX + holeRx))
            {
                toy1Y = -0.15f;
                toy1Falling = false;
                toy1Vy = 0.0f;
            }
        }

        // -------------------------
        // TOY 1 – spuštanje na policu
        // -------------------------
        if (toy1Won && !toy1OnShelf)
        {
            toy1Vy -= gravity * dt;
            toy1Y += toy1Vy * dt;

            if (toy1Y <= shelfY)
            {
                toy1Y = shelfY;
                toy1OnShelf = true;
                toy1Falling = false;
                toy1Vy = 0.0f;

                blinkActive = true;
                blinkTimer = 0.0f;
                blinkGreen = true;
            }
        }

        // -------------------------
        // TOY 2 – padanje
        // -------------------------
        if (toy2Falling)
        {
            toy2Vy -= gravity * dt;
            toy2Y += toy2Vy * dt;

            if (!toy2Won &&
                std::fabs(toy2X - holeX) < holeRx &&
                std::fabs(toy2Y - holeY) < holeRy)
            {
                toy2Won = true;
                toy2Falling = true;
                gameRunning = false;
            }

            if (!toy2Won &&
                toy2Y <= -0.15f &&
                !(toy2X > holeX - holeRx && toy2X < holeX + holeRx))
            {
                toy2Y = -0.15f;
                toy2Falling = false;
                toy2Vy = 0.0f;
            }
        }

        // -------------------------
        // TOY 2 – spuštanje na policu
        // -------------------------
        if (toy2Won && !toy2OnShelf)
        {
            toy2Vy -= gravity * dt;
            toy2Y += toy2Vy * dt;

            if (toy2Y <= shelfY)
            {
                toy2Y = shelfY;
                toy2OnShelf = true;
                toy2Falling = false;
                toy2Vy = 0.0f;

                blinkActive = true;
                blinkTimer = 0.0f;
                blinkGreen = true;
            }
        }

        // ============================================================
        //                          RENDER
        // ============================================================

        // 1) STAKLENA KUTIJA
        glBindVertexArray(VAO);
        glUniform1i(locUseTex, GL_FALSE);
        glUniform4f(locColor, 1, 1, 1, 0.20f);
        glUniform2f(locScale, 1.0f, 0.80f);
        glUniform2f(locOffset, 0.0f, 0.10f);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // 2) IGRAČKE
        glUniform1i(locUseTex, GL_TRUE);
        glUniform4f(locColor, 1, 1, 1, 1);

        if (!toy1Collected)
        {
            glBindTexture(GL_TEXTURE_2D, toy1Tex);
            glUniform2f(locScale, 0.20f, 0.20f);
            glUniform2f(locOffset, toy1X, toy1Y);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }

        if (!toy2Collected)
        {
            glBindTexture(GL_TEXTURE_2D, toy2Tex);
            glUniform2f(locScale, 0.20f, 0.20f);
            glUniform2f(locOffset, toy2X, toy2Y);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }

        // 3) KONOPAC
        glBindTexture(GL_TEXTURE_2D, ropeTex);
        glUniform2f(locScale, 0.03f, ropeLength);
        glUniform2f(locOffset, clawX, 0.90f - ropeLength / 2.0f);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // 4) KANDŽA
        if (!gameRunning || holdingToy)
            glBindTexture(GL_TEXTURE_2D, closedClawTex);
        else
            glBindTexture(GL_TEXTURE_2D, openClawTex);
        glUniform2f(locScale, 0.18f, 0.22f);
        glUniform2f(locOffset, clawX, clawY);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // 5) SLOT
        glBindVertexArray(VAO);
        glUniform1i(locUseTex, GL_TRUE);
        glUniform4f(locColor, 1, 1, 1, 1);
        glBindTexture(GL_TEXTURE_2D, slotTex);
        glUniform2f(locScale, 0.25f, 0.45f);
        glUniform2f(locOffset, 0.0f, -0.62f);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // 6) RUPA
        glBindVertexArray(circleVAO);
        glUniform1i(locUseTex, GL_FALSE);
        glUniform4f(locColor, 0, 0, 0, 1);
        glUniform2f(locScale, 0.10f, 0.08f);
        glUniform2f(locOffset, holeX, holeY);
        glDrawArrays(GL_TRIANGLE_FAN, 0, segments + 2);

        // 7) POLICA
        glBindVertexArray(VAO);
        glUniform4f(locColor, 0.30f, 0.30f, 0.30f, 1);
        glUniform2f(locScale, 0.20f, 0.05f);
        glUniform2f(locOffset, shelfX, shelfY - 0.08f);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // 8) LAMPICA
        glBindVertexArray(circleVAO);
        glUniform1i(locUseTex, GL_FALSE);

        if (blinkActive)
        {
            if (blinkGreen)
                glUniform4f(locColor, 0.0f, 0.8f, 0.0f, 1.0f);
            else
                glUniform4f(locColor, 0.9f, 0.1f, 0.1f, 1.0f);
        }
        else if (lightOn)
        {
            glUniform4f(locColor, 0.0f, 0.6f, 1.0f, 1.0f);
        }
        else
        {
            glUniform4f(locColor, 0.0f, 0.2f, 0.4f, 1.0f);
        }

        glUniform2f(locScale, 0.06f, 0.06f);
        glUniform2f(locOffset, 0.0f, 0.60f);
        glDrawArrays(GL_TRIANGLE_FAN, 0, segments + 2);

        // 9) POTPIS – ANJA GUZINA (poluprovidno u uglu)
        glBindVertexArray(VAO);
        glUniform1i(locUseTex, GL_TRUE);
        glUniform4f(locColor, 1.0f, 1.0f, 1.0f, 0.55f);

        glBindTexture(GL_TEXTURE_2D, signatureTex);

        glUniform2f(locScale, 0.85f, 0.50f);
        glUniform2f(locOffset, -0.75f, 0.85f);

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // ============================================================
        // KRAJ FREJMA
        // ============================================================
        glfwSwapBuffers(window);
        glfwPollEvents();

        // ============================================================
        // FRAME LIMITER — 75 FPS
        // ============================================================
        static double lastFrameStart = glfwGetTime();
        double currentFrame = glfwGetTime();
        double frameDuration = currentFrame - lastFrameStart;
        double targetFrameTime = 1.0 / 75.0;

        if (frameDuration < targetFrameTime)
        {
            while (glfwGetTime() - lastFrameStart < targetFrameTime) {}
        }

        lastFrameStart = glfwGetTime();

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);
    }

    // =======================
    //   UNLOAD CURSORS
    // =======================
    glfwDestroyCursor(cursorCoin);
    glfwDestroyCursor(cursorLever);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
