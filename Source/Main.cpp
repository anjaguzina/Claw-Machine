// Autori: Anja Guzina
// Opis: 3D Model automata za hvatanje igračaka sa kandžom, polugom i kockama

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cmath>
#include <map>
#include <algorithm>
#include <tuple>
#include <exception>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

//GLM biblioteke
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../Header/Util.h"

// Struktura za materijal
struct Material {
    glm::vec3 Kd;  // Diffuse color
    glm::vec3 Ka;  // Ambient color
    glm::vec3 Ks;  // Specular color
    float Ns;      // Shininess
    float d;       // Dissolve (alpha)
};

// Struktura za čuvanje podataka o .obj modelu
struct OBJModel {
    std::vector<float> vertices;  // Interleaved: pos(3) + color(4) + tex(2) + normal(3) = 12 floats
    std::vector<unsigned int> indices;
    std::vector<unsigned int> materialIndices;  // Materijal ID za svaki index
    std::map<std::string, Material> materials;  // Mapa materijala po imenu
    unsigned int VAO = 0, VBO = 0, EBO = 0;
    unsigned int indexCount = 0;
};

// Funkcija za učitavanje .mtl fajla
std::map<std::string, Material> loadMTL(const char* mtlPath) {
    std::map<std::string, Material> materials;
    Material currentMaterial = {};  // Inicijalizuj na default vrednosti
    std::string currentMaterialName;
    
    std::ifstream file(mtlPath);
    if (!file.is_open()) {
        std::cout << "Greska pri otvaranju .mtl fajla: " << mtlPath << std::endl;
        return materials;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string type;
        iss >> type;
        
        if (type == "newmtl") {
            // Sačuvaj prethodni materijal ako postoji
            if (!currentMaterialName.empty()) {
                materials[currentMaterialName] = currentMaterial;
            }
            // Novi materijal
            iss >> currentMaterialName;
            // Resetuj na default vrednosti
            currentMaterial.Kd = glm::vec3(0.8f, 0.8f, 0.8f);
            currentMaterial.Ka = glm::vec3(0.2f, 0.2f, 0.2f);
            currentMaterial.Ks = glm::vec3(0.0f, 0.0f, 0.0f);
            currentMaterial.Ns = 10.0f;
            currentMaterial.d = 1.0f;
        }
        else if (type == "Kd" && !currentMaterialName.empty()) {
            iss >> currentMaterial.Kd.r >> currentMaterial.Kd.g >> currentMaterial.Kd.b;
        }
        else if (type == "Ka" && !currentMaterialName.empty()) {
            iss >> currentMaterial.Ka.r >> currentMaterial.Ka.g >> currentMaterial.Ka.b;
        }
        else if (type == "Ks" && !currentMaterialName.empty()) {
            iss >> currentMaterial.Ks.r >> currentMaterial.Ks.g >> currentMaterial.Ks.b;
        }
        else if (type == "Ns" && !currentMaterialName.empty()) {
            iss >> currentMaterial.Ns;
        }
        else if (type == "d" && !currentMaterialName.empty()) {
            iss >> currentMaterial.d;
        }
    }
    
    // Sačuvaj poslednji materijal
    if (!currentMaterialName.empty()) {
        materials[currentMaterialName] = currentMaterial;
    }
    
    file.close();
    std::cout << "Ucitano " << materials.size() << " materijala iz .mtl fajla!" << std::endl;
    
    return materials;
}

// Funkcija za učitavanje .obj fajla
OBJModel loadOBJ(const char* filePath) {
    OBJModel model;
    
    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> texCoords;
    std::vector<glm::vec3> normals;
    std::vector<unsigned int> posIndices, texIndices, normIndices;
    std::vector<unsigned int> materialIDs;  // Materijal ID za svaki trougao
    
    std::string currentMaterial = "";  // Trenutni materijal
    std::map<std::string, unsigned int> materialNameToID;  // Mapa imena materijala na ID
    unsigned int nextMaterialID = 0;
    
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cout << "Greska pri otvaranju .obj fajla: " << filePath << std::endl;
        return model;
    }
    
    std::string mtlFileName = "";
    std::string objDir = std::string(filePath);
    size_t lastSlash = objDir.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        objDir = objDir.substr(0, lastSlash + 1);
    } else {
        objDir = "";
    }
    
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string type;
        iss >> type;
        
        if (type == "mtllib") {
            iss >> mtlFileName;
            std::string mtlPath = objDir + mtlFileName;
            model.materials = loadMTL(mtlPath.c_str());
            // Osiguraj da dno automata uvek ima tamno metalno sivo (floor_metal)
            if (model.materials.find("floor_metal") == model.materials.end()) {
                Material floorMat;
                floorMat.Kd = glm::vec3(0.22f, 0.24f, 0.26f);
                floorMat.Ka = glm::vec3(0.12f, 0.14f, 0.16f);
                floorMat.Ks = glm::vec3(0.45f, 0.48f, 0.52f);
                floorMat.Ns = 100.0f;
                floorMat.d = 1.0f;
                model.materials["floor_metal"] = floorMat;
            }
            // Kreiraj mapu imena materijala na ID
            for (const auto& pair : model.materials) {
                materialNameToID[pair.first] = nextMaterialID++;
            }
        }
        else if (type == "usemtl") {
            std::string materialName;
            iss >> materialName;
            currentMaterial = materialName;
        }
        else if (type == "v") {
            glm::vec3 pos;
            iss >> pos.x >> pos.y >> pos.z;
            positions.push_back(pos);
        }
        else if (type == "vt") {
            glm::vec2 tex;
            iss >> tex.x >> tex.y;
            texCoords.push_back(tex);
        }
        else if (type == "vn") {
            glm::vec3 norm;
            iss >> norm.x >> norm.y >> norm.z;
            normals.push_back(norm);
        }
        else if (type == "f") {
            std::string vertex;
            std::vector<unsigned int> facePos, faceTex, faceNorm;
            
            while (iss >> vertex) {
                // Parsiranje formata v/vt/vn ili v//vn ili v/vt
                size_t firstSlash = vertex.find('/');
                if (firstSlash == std::string::npos) {
                    // Samo vertex index
                    unsigned int p = std::stoul(vertex);
                    facePos.push_back(p - 1);
                } else {
                    // Parsiranje v/vt/vn
                    unsigned int p = std::stoul(vertex.substr(0, firstSlash));
                    facePos.push_back(p - 1);
                    
                    size_t secondSlash = vertex.find('/', firstSlash + 1);
                    if (secondSlash != std::string::npos) {
                        // Ima texture i normal
                        std::string texStr = vertex.substr(firstSlash + 1, secondSlash - firstSlash - 1);
                        std::string normStr = vertex.substr(secondSlash + 1);
                        
                        if (!texStr.empty()) {
                            unsigned int t = std::stoul(texStr);
                            faceTex.push_back(t - 1);
                        }
                        if (!normStr.empty()) {
                            unsigned int n = std::stoul(normStr);
                            faceNorm.push_back(n - 1);
                        }
                    } else {
                        // Samo texture, nema normal
                        std::string texStr = vertex.substr(firstSlash + 1);
                        if (!texStr.empty()) {
                            unsigned int t = std::stoul(texStr);
                            faceTex.push_back(t - 1);
                        }
                    }
                }
            }
            
            // Triangulacija (pretvaranje kvadova u trouglove)
            if (facePos.size() >= 3) {
                // Pronađi materijal ID za trenutni materijal
                unsigned int matID = 0;  // Default materijal
                if (!currentMaterial.empty() && materialNameToID.find(currentMaterial) != materialNameToID.end()) {
                    matID = materialNameToID[currentMaterial];
                }
                
                for (size_t i = 1; i < facePos.size() - 1; ++i) {
                    posIndices.push_back(facePos[0]);
                    posIndices.push_back(facePos[i]);
                    posIndices.push_back(facePos[i + 1]);
                    
                    if (!faceTex.empty() && faceTex.size() == facePos.size()) {
                        texIndices.push_back(faceTex[0]);
                        texIndices.push_back(faceTex[i]);
                        texIndices.push_back(faceTex[i + 1]);
                    }
                    
                    if (!faceNorm.empty() && faceNorm.size() == facePos.size()) {
                        normIndices.push_back(faceNorm[0]);
                        normIndices.push_back(faceNorm[i]);
                        normIndices.push_back(faceNorm[i + 1]);
                    }
                    
                    // Dodeli materijal ID za svaki trougao (3 indeksa)
                    materialIDs.push_back(matID);
                    materialIDs.push_back(matID);
                    materialIDs.push_back(matID);
                }
            }
        }
    }
    
    file.close();
    
    // Kreiranje interleaved vertex buffer-a
    std::map<std::tuple<unsigned int, unsigned int, unsigned int>, unsigned int> vertexMap;
    unsigned int currentIndex = 0;
    
    for (size_t i = 0; i < posIndices.size(); ++i) {
        unsigned int posIdx = posIndices[i];
        unsigned int texIdx = (i < texIndices.size()) ? texIndices[i] : 0;
        unsigned int normIdx = (i < normIndices.size()) ? normIndices[i] : 0;
        
        std::tuple<unsigned int, unsigned int, unsigned int> key(posIdx, texIdx, normIdx);
        
        if (vertexMap.find(key) == vertexMap.end()) {
            vertexMap[key] = currentIndex++;
            
            // Pozicija
            if (posIdx < positions.size()) {
                model.vertices.push_back(positions[posIdx].x);
                model.vertices.push_back(positions[posIdx].y);
                model.vertices.push_back(positions[posIdx].z);
            } else {
                model.vertices.push_back(0.0f);
                model.vertices.push_back(0.0f);
                model.vertices.push_back(0.0f);
            }
            
            // Boja (default bela)
            model.vertices.push_back(1.0f);
            model.vertices.push_back(1.0f);
            model.vertices.push_back(1.0f);
            model.vertices.push_back(1.0f);
            
            // Tekstura koordinate
            if (texIdx < texCoords.size()) {
                model.vertices.push_back(texCoords[texIdx].x);
                model.vertices.push_back(texCoords[texIdx].y);
            } else {
                model.vertices.push_back(0.0f);
                model.vertices.push_back(0.0f);
            }
            
            // Normala
            if (normIdx < normals.size()) {
                model.vertices.push_back(normals[normIdx].x);
                model.vertices.push_back(normals[normIdx].y);
                model.vertices.push_back(normals[normIdx].z);
            } else {
                model.vertices.push_back(0.0f);
                model.vertices.push_back(1.0f);
                model.vertices.push_back(0.0f);
            }
        }
        
        model.indices.push_back(vertexMap[key]);
        // Sačuvaj materijal ID za ovaj index
        if (i < materialIDs.size()) {
            model.materialIndices.push_back(materialIDs[i]);
        } else {
            model.materialIndices.push_back(0);  // Default materijal
        }
    }
    
    model.indexCount = (unsigned int)model.indices.size();
    
    // Provera da li ima dovoljno podataka
    if (model.vertices.empty() || model.indices.empty()) {
        std::cout << "Greska: Model nema podataka!" << std::endl;
        std::cout << "Vertices: " << model.vertices.size() << ", Indices: " << model.indices.size() << std::endl;
        return model;
    }
    
    // Kreiranje VAO, VBO, EBO
    glGenVertexArrays(1, &model.VAO);
    glGenBuffers(1, &model.VBO);
    glGenBuffers(1, &model.EBO);
    
    glBindVertexArray(model.VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, model.VBO);
    glBufferData(GL_ARRAY_BUFFER, model.vertices.size() * sizeof(float), model.vertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, model.indices.size() * sizeof(unsigned int), model.indices.data(), GL_STATIC_DRAW);
    
    unsigned int stride = 12 * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(7 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, (void*)(10 * sizeof(float)));
    glEnableVertexAttribArray(3);
    
    glBindVertexArray(0);
    
    // Provera OpenGL greške
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cout << "OpenGL greska pri ucitavanju modela: " << error << std::endl;
    }
    
    std::cout << "Uspesno ucitano " << model.indexCount / 3 << " trouglova iz .obj fajla!" << std::endl;
    std::cout << "Broj vertex-a: " << model.vertices.size() / 12 << std::endl;
    
    return model;
}

// Crtanje OBJ modela na datoj matrici (samo neprozirni materijali)
static void drawOBJModel(const OBJModel& model, const glm::mat4& matrix, unsigned int modelLoc, unsigned int shader) {
    if (model.indices.empty()) return;
    std::map<unsigned int, std::vector<std::pair<size_t, size_t>>> materialRanges;
    unsigned int currentMatID = (model.materialIndices.size() > 0) ? model.materialIndices[0] : 0;
    size_t rangeStart = 0;
    for (size_t i = 0; i < model.indices.size(); ++i) {
        unsigned int matID = (i < model.materialIndices.size()) ? model.materialIndices[i] : 0;
        if (matID != currentMatID) {
            if (i > rangeStart)
                materialRanges[currentMatID].push_back(std::make_pair(rangeStart, i - rangeStart));
            currentMatID = matID;
            rangeStart = i;
        }
    }
    if (model.indices.size() > rangeStart)
        materialRanges[currentMatID].push_back(std::make_pair(rangeStart, model.indices.size() - rangeStart));
    std::map<unsigned int, std::string> matIDToName;
    unsigned int id = 0;
    for (const auto& pair : model.materials)
        matIDToName[id++] = pair.first;
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(matrix));
    glUniform1i(glGetUniformLocation(shader, "transparent"), 0);
    glUniform1i(glGetUniformLocation(shader, "useTex"), 0);
    glBindVertexArray(model.VAO);
    for (const auto& group : materialRanges) {
        std::string matName = (matIDToName.find(group.first) != matIDToName.end()) ? matIDToName[group.first] : "";
        const Material* mat = nullptr;
        if (model.materials.find(matName) != model.materials.end())
            mat = &model.materials.at(matName);
        if (!mat && !model.materials.empty())
            mat = &model.materials.begin()->second;
        if (!mat || mat->d < 1.0f) continue;
        glUniform3f(glGetUniformLocation(shader, "uMaterialAmbient"), mat->Ka.r, mat->Ka.g, mat->Ka.b);
        glUniform3f(glGetUniformLocation(shader, "uMaterialDiffuse"), mat->Kd.r, mat->Kd.g, mat->Kd.b);
        glUniform3f(glGetUniformLocation(shader, "uMaterialSpecular"), mat->Ks.r, mat->Ks.g, mat->Ks.b);
        glUniform1f(glGetUniformLocation(shader, "uMaterialShininess"), mat->Ns);
        glUniform4f(glGetUniformLocation(shader, "uColor"), mat->Kd.r, mat->Kd.g, mat->Kd.b, 1.0f);
        for (const auto& range : group.second)
            glDrawElements(GL_TRIANGLES, (unsigned int)range.second, GL_UNSIGNED_INT, (void*)(range.first * sizeof(unsigned int)));
    }
    glBindVertexArray(0);
}

// Globalne promenljive za kontrolu kamere
float cameraYaw = 0.0f;       // 0 = ispred automata; horizontalna rotacija (strelice + miš)
float cameraPitch = 20.0f;    // Vertikalna rotacija (ograničena 180° = -90 do 90)
float cameraDistance = 4.0f;  // Udaljenost od centra
bool mousePressed = false;
double lastMouseX = 0.0;
double lastMouseY = 0.0;
// Kamera je "ispred" automata kada je yaw u ovom opsegu – tada su mogući pomeranje kandže, ubacivanje žetona i preuzimanje igračke
const float cameraInFrontYawHalf = 50.0f;

// Podešavanje testiranja dubine (tasteri 1 i 2) i odstranjivanja naličja (tasteri 3 i 4) – kao na vežbama
bool depthTestEnabled = true;   // 1 = uključi, 2 = isključi
bool cullFaceEnabled = true;    // 3 = uključi, 4 = isključi

// Globalne promenljive za kandžu
bool clawOpen = false;  // false = zatvorena, true = otvorena
float clawOpenAngle = 0.0f;  // Ugao otvaranja prstiju (0 = zatvoreno, 30 = otvoreno)
float clawX = 0.0f;  // X pozicija kandže
float clawZ = 0.0f;  // Z pozicija kandže
float clawY = 0.5f;   // Y u model prostoru – gornja pozicija ispod krova (stap ne izlazi van)
float clawLowerSpeed = 0.04f;

// Igračka (roza kocka): pozicija na dnu, nosi je kandža, pa baci na dno ili u rupu
float toyCubeX = -0.03f;   // medved – ista pozicija kao ranije kocka
float toyCubeY = -0.12f;
float toyCubeZ = 0.0f;
bool toyCarried = false;   // da li kandža trenutno nosi igračku (kocku ili pticu)
bool toyWon = false;
bool toyCollected = false;  // true = kliknuto na medveda u pregradi, igračka nestane, pa se gasi automat
bool birdCarried = false;
bool birdWon = false;
bool birdCollected = false; // true = kliknuto na zeca u pregradi, igračka nestane, pa se gasi automat
bool birdOnFloor = false; // ptica bačena na sivo dno (ne u rupu) – crtamo je na birdToy poziciji
int carriedWhich = 0;     // 0 = ništa, 1 = medved, 2 = zec
const float toyFloorY = -0.16f;  // visina pada na dnu (privremeno -0.16 za test svetala)
// Granice poda – igračka ostaje unutar vidljivog dna (ne iza/van automata)
const float playFloorMinX = -0.1f, playFloorMaxX = 0.1f, playFloorMinZ = -0.1f, playFloorMaxZ = 0.1f;
// Rupa = kvadratni isečak S LEVE STRANE dna automata;
// kad mesto PADA igračke (dropX, dropZ) padne u ovu oblast → inHole, pregrada + trepćuće svetlo;
// inače igračka ostaje na podu.
const float holeMinX = -0.12f, holeMaxX = 0.02f;  // levo: šira oblast da sigurno uhvati rupu
const float holeMinZ = -0.02f, holeMaxZ = 0.12f;  // prednji levi kvadrat (rupa)
// Donja leva pregrada – obe igračke unutar pregrade; smanjen skala da ništa ne strši kroz metal (uvo zeke, ivice meda)
const float prizeX = -0.15f, prizeY = -0.41f, prizeZ = 0.18f;   // obe igračke u pregradi, spušteno na dno
float birdToyX = 0.06f, birdToyY = -0.12f, birdToyZ = 0.02f;   // zec – pomeren ulevo da uho ne viri iz stakla
const float prizeInCompartmentScale = 0.62f;  // u pregradi smanjeno da metal fizički „zakloni” ivice – ništa ne probija zid
const float grabRadiusWorld = 0.12f;  // radijus hvatanja (veći da se lakše uhvate medved i zec)
const float clawTipOffset = 0.32f;    // igračka niže kod pipaka da se vidi cela (clawY - offset)
const float machineScale = 0.1f;      // modelMatrix scale za automat

// Globalne promenljive za sijalicu i stanje automata
bool lightOn = false;       // sijalica upaljena (svetlo plava) kad je automat uključen
bool machineOn = true;      // false = automat isključen (tamno plava), tek crveno dugme ga ponovo uključi
bool prizeBlinking = false; // true = osvojena igračka u pregradi, sijalica treperi zeleno-crveno
float blinkTimer = 0.0f;    // tajmer za naizmenično zeleno/crveno na 0.5s
bool blinkGreen = true;     // trenutno zeleno ili crveno

// Pomocna: projekcija 3D tacke u ekranske koordinate (za klik na pregradu)
static void worldToScreen(const glm::vec3& world, int winW, int winH, double& outX, double& outY)
{
    float yawRad = glm::radians(cameraYaw);
    float pitchRad = glm::radians(cameraPitch);
    float camX = cameraDistance * cosf(pitchRad) * sinf(yawRad);
    float camY = cameraDistance * sinf(pitchRad);
    float camZ = cameraDistance * cosf(pitchRad) * cosf(yawRad);
    glm::vec3 cameraPos(camX, camY, camZ);
    glm::mat4 view = glm::lookAt(cameraPos, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float)winW / (float)winH, 0.1f, 100.0f);
    glm::vec4 clip = proj * view * glm::vec4(world, 1.0f);
    if (clip.w <= 0.0f) { outX = -1000; outY = -1000; return; }
    float ndcX = clip.x / clip.w, ndcY = clip.y / clip.w;
    outX = (ndcX + 1.0) * 0.5 * winW;
    outY = (1.0 - ndcY) * 0.5 * winH;
}

// Callback funkcija za miš
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        mousePressed = true;
        glfwGetCursorPos(window, &lastMouseX, &lastMouseY);
        // Ubacivanje žetona i preuzimanje igračke samo kad je kamera otprilike ispred automata
        bool inFront = (cameraYaw >= -cameraInFrontYawHalf && cameraYaw <= cameraInFrontYawHalf);
        if (!inFront) return;

        int width, height;
        glfwGetWindowSize(window, &width, &height);
        double mouseXNorm = lastMouseX / width;
        double mouseYNorm = lastMouseY / height;
        const double prizeHitRadius = 60.0; // pikseli

        // Klik na osvojenu igračku u pregradi: prvo igračka nestane (collected), pa se gasi automat
        if (prizeBlinking && (toyWon || birdWon))
        {
            if (toyWon && !toyCollected) {
                double sx, sy;
                worldToScreen(glm::vec3(prizeX, prizeY, prizeZ), width, height, sx, sy);
                if ((lastMouseX - sx) * (lastMouseX - sx) + (lastMouseY - sy) * (lastMouseY - sy) < prizeHitRadius * prizeHitRadius) {
                    toyCollected = true;  // igračka nestane
                    prizeBlinking = false;
                    machineOn = false;
                    lightOn = false;
                    return;
                }
            }
            if (birdWon && !birdCollected) {
                double sx, sy;
                worldToScreen(glm::vec3(prizeX, prizeY, prizeZ), width, height, sx, sy);
                if ((lastMouseX - sx) * (lastMouseX - sx) + (lastMouseY - sy) * (lastMouseY - sy) < prizeHitRadius * prizeHitRadius) {
                    birdCollected = true; // igračka nestane
                    prizeBlinking = false;
                    machineOn = false;
                    lightOn = false;
                    return;
                }
            }
        }

        // Klik na crveno dugme / ubacivanje žetona (leva strana ekrana – prikladan objekat)
        if (mouseXNorm < 0.6 && mouseXNorm > 0.0 && mouseYNorm > 0.2 && mouseYNorm < 0.8)
        {
            if (!machineOn) {
                machineOn = true;
                lightOn = true;  // ubacivanje žetona = uključivanje automata
            } else if (!prizeBlinking) {
                lightOn = !lightOn;
            }
        }
    }
    else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
        mousePressed = false;
}

// Callback funkcija za kretanje miša
void cursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (mousePressed)
    {
        double deltaX = xpos - lastMouseX;
        double deltaY = ypos - lastMouseY;
        
        cameraYaw += (float)(deltaX * 0.5);
        cameraPitch += (float)(deltaY * 0.5);
        
        // Ograničenje pogleda po Y osi na 180 stepeni (-90 do 90)
        if (cameraPitch > 90.0f) cameraPitch = 90.0f;
        if (cameraPitch < -90.0f) cameraPitch = -90.0f;
        
        lastMouseX = xpos;
        lastMouseY = ypos;
    }
}

// Callback funkcija za scroll (zoom)
void scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    cameraDistance -= (float)(yoffset * 0.2);
    if (cameraDistance < 1.0f) cameraDistance = 1.0f;
    if (cameraDistance > 10.0f) cameraDistance = 10.0f;
}

int main(void)
{
    if (!glfwInit())
    {
        std::cout<<"GLFW Biblioteka se nije ucitala! :(\n";
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Full screen: koristi primarni monitor i rezoluciju ekrana
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    unsigned int wWidth = (unsigned int)mode->width;
    unsigned int wHeight = (unsigned int)mode->height;

    GLFWwindow* window;
    const char wTitle[] = "Bird in a Claw Machine";
    window = glfwCreateWindow(wWidth, wHeight, wTitle, monitor, NULL);
    
    if (window == NULL)
    {
        std::cout << "Prozor nije napravljen! :(\n";
        glfwTerminate();
        return 2;
    }
    
    glfwMakeContextCurrent(window);
    
    // Postavljamo callback funkcije za miš
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetScrollCallback(window, scrollCallback);
    
    // Postavljanje viewport-a
    glViewport(0, 0, wWidth, wHeight);

    if (glewInit() != GLEW_OK)
    {
        std::cout << "GLEW nije mogao da se ucita! :'(\n";
        return 3;
    }
    
    std::cout << "Prozor kreiran i OpenGL inicijalizovan!" << std::endl;
    
    // Kursori: coin kad je automat isključen (početak, posle preuzimanja igračke), poluga kad je uključen
    GLFWcursor* cursorCoin = loadImageToCursor("Resources/coin.png");
    GLFWcursor* cursorLever = loadImageToCursor("Resources/poluga.png");
    glfwSetCursor(window, cursorCoin);  // na početku svetlo je ugašeno
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST); // Uključeno testiranje dubine (taster 1/2 ga uključuje/isključuje)
    glCullFace(GL_BACK);     // Kada je uključeno odstranjivanje naličja (taster 3), uklanjaju se zadnja lica

    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++ KREIRANJE 3D MODELA +++++++++++++++++++++++++++++++++++++++++++++++++
    
    unsigned int unifiedShader = createShader("basic.vert", "basic.frag");
    glUseProgram(unifiedShader);
    glUniform1i(glGetUniformLocation(unifiedShader, "uTex"), 0);

    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++ UČITAVANJE .OBJ FAJLA +++++++++++++++++++++++++++++++++++++++++++++++++
    
    std::cout << "Ucitavam .obj fajl..." << std::endl;
    OBJModel clawMachine = loadOBJ("Resources/claw_machine.obj");
    
    if (clawMachine.indexCount == 0) {
        std::cout << "Greska: Model nije uspesno ucitano!" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    std::cout << "Model automata uspesno ucitano!" << std::endl;
    
    // Učitavanje kandže
    std::cout << "Ucitavam claw.obj fajl..." << std::endl;
    OBJModel claw = loadOBJ("Resources/claw.obj");
    
    if (claw.indexCount == 0) {
        std::cout << "Greska: Claw model nije uspesno ucitano!" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    std::cout << "Model kandze uspesno ucitano! Broj trouglova: " << claw.indexCount / 3 << std::endl;
    
    // Učitavanje medveda i zeca (igračke umesto kocke i ptice)
    OBJModel bearModel = loadOBJ("Resources/bearobj.obj");
    OBJModel rabbitModel = loadOBJ("Resources/rabbit.obj");
    const float bearScale = 0.03f;   // manje dimenzije
    const float rabbitScale = 0.022f; // zec manji da uho ne viri iz izloga/stakla
    
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++ UNIFORME +++++++++++++++++++++++++++++++++++++++++++++++++
    
    std::cout << "Kreiram uniforme..." << std::endl;
    unsigned int modelLoc = glGetUniformLocation(unifiedShader, "uM");
    
    glm::mat4 view;
    glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f); // Centar scene
    glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
    unsigned int viewLoc = glGetUniformLocation(unifiedShader, "uV");
    
    glm::mat4 projectionP = glm::perspective(glm::radians(45.0f), (float)wWidth / (float)wHeight, 0.1f, 100.0f);
    unsigned int projectionLoc = glGetUniformLocation(unifiedShader, "uP");
    
    // Model matrica za automat
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, 0.0f, 0.0f));
    modelMatrix = glm::scale(modelMatrix, glm::vec3(0.1f, 0.1f, 0.1f)); // Povećano skaliranje modela
    
    std::cout << "Uniforme kreirane!" << std::endl;
    
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++ RENDER LOOP - PETLJA ZA CRTANJE +++++++++++++++++++++++++++++++++++++++++++++++++
    std::cout << "Postavljam shader i clear color..." << std::endl;
    glUseProgram(unifiedShader);
    glClearColor(0.2f, 0.2f, 0.25f, 1.0f);
    std::cout << "Shader i clear color postavljeni!" << std::endl;


    
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++ KREIRANJE KOCKE ZA DRUGU IGRAČKU +++++++++++++++++++++++++++++++++++++++++++++++++
    
    // Kreiramo kocku za drugu igračku (roze kocka) - dimenzije slične ptičici
    float toyCubeSize = 0.1f; // Dimenzija kocke (povećana da bude vidljiva)
    float toyCubeVertices[] = {
        //X    Y    Z      R    G    B    A         S   T      NX NY NZ
        // Prednja strana
        toyCubeSize, toyCubeSize, toyCubeSize,   1.0f, 0.7f, 0.8f, 1.0f,   0.0f, 0.0f,    0.0f, 0.0f, 1.0f,
       -toyCubeSize, toyCubeSize, toyCubeSize,   1.0f, 0.7f, 0.8f, 1.0f,   1.0f, 0.0f,    0.0f, 0.0f, 1.0f,
       -toyCubeSize,-toyCubeSize, toyCubeSize,   1.0f, 0.7f, 0.8f, 1.0f,   1.0f, 1.0f,    0.0f, 0.0f, 1.0f,
        toyCubeSize,-toyCubeSize, toyCubeSize,   1.0f, 0.7f, 0.8f, 1.0f,   0.0f, 1.0f,    0.0f, 0.0f, 1.0f,
        // Leva strana
       -toyCubeSize, toyCubeSize, toyCubeSize,   1.0f, 0.7f, 0.8f, 1.0f,   0.0f, 0.0f,    -1.0f, 0.0f, 0.0f,
       -toyCubeSize, toyCubeSize,-toyCubeSize,   1.0f, 0.7f, 0.8f, 1.0f,   1.0f, 0.0f,    -1.0f, 0.0f, 0.0f,
       -toyCubeSize,-toyCubeSize,-toyCubeSize,   1.0f, 0.7f, 0.8f, 1.0f,   1.0f, 1.0f,    -1.0f, 0.0f, 0.0f,
       -toyCubeSize,-toyCubeSize, toyCubeSize,   1.0f, 0.7f, 0.8f, 1.0f,   0.0f, 1.0f,    -1.0f, 0.0f, 0.0f,
        // Donja strana
        toyCubeSize,-toyCubeSize, toyCubeSize,   1.0f, 0.7f, 0.8f, 1.0f,   0.0f, 0.0f,    0.0f, -1.0f, 0.0f,
       -toyCubeSize,-toyCubeSize, toyCubeSize,   1.0f, 0.7f, 0.8f, 1.0f,   1.0f, 0.0f,    0.0f, -1.0f, 0.0f,
       -toyCubeSize,-toyCubeSize,-toyCubeSize,   1.0f, 0.7f, 0.8f, 1.0f,   1.0f, 1.0f,    0.0f, -1.0f, 0.0f,
        toyCubeSize,-toyCubeSize,-toyCubeSize,   1.0f, 0.7f, 0.8f, 1.0f,   0.0f, 1.0f,    0.0f, -1.0f, 0.0f,
        // Gornja strana
        toyCubeSize, toyCubeSize, toyCubeSize,   1.0f, 0.7f, 0.8f, 1.0f,   0.0f, 0.0f,    0.0f, 1.0f, 0.0f,
        toyCubeSize, toyCubeSize,-toyCubeSize,   1.0f, 0.7f, 0.8f, 1.0f,   1.0f, 0.0f,    0.0f, 1.0f, 0.0f,
       -toyCubeSize, toyCubeSize,-toyCubeSize,   1.0f, 0.7f, 0.8f, 1.0f,   1.0f, 1.0f,    0.0f, 1.0f, 0.0f,
       -toyCubeSize, toyCubeSize, toyCubeSize,   1.0f, 0.7f, 0.8f, 1.0f,   0.0f, 1.0f,    0.0f, 1.0f, 0.0f,
        // Desna strana
        toyCubeSize, toyCubeSize, toyCubeSize,   1.0f, 0.7f, 0.8f, 1.0f,   0.0f, 0.0f,    1.0f, 0.0f, 0.0f,
        toyCubeSize,-toyCubeSize, toyCubeSize,   1.0f, 0.7f, 0.8f, 1.0f,   1.0f, 0.0f,    1.0f, 0.0f, 0.0f,
        toyCubeSize,-toyCubeSize,-toyCubeSize,   1.0f, 0.7f, 0.8f, 1.0f,   1.0f, 1.0f,    1.0f, 0.0f, 0.0f,
        toyCubeSize, toyCubeSize,-toyCubeSize,   1.0f, 0.7f, 0.8f, 1.0f,   0.0f, 1.0f,    1.0f, 0.0f, 0.0f,
        // Zadnja strana
        toyCubeSize, toyCubeSize,-toyCubeSize,   1.0f, 0.7f, 0.8f, 1.0f,   0.0f, 0.0f,    0.0f, 0.0f, -1.0f,
        toyCubeSize,-toyCubeSize,-toyCubeSize,   1.0f, 0.7f, 0.8f, 1.0f,   1.0f, 0.0f,    0.0f, 0.0f, -1.0f,
       -toyCubeSize,-toyCubeSize,-toyCubeSize,   1.0f, 0.7f, 0.8f, 1.0f,   1.0f, 1.0f,    0.0f, 0.0f, -1.0f,
       -toyCubeSize, toyCubeSize,-toyCubeSize,   1.0f, 0.7f, 0.8f, 1.0f,   0.0f, 1.0f,    0.0f, 0.0f, -1.0f,
    };
    
    unsigned int toyCubeVAO, toyCubeVBO;
    glGenVertexArrays(1, &toyCubeVAO);
    glBindVertexArray(toyCubeVAO);
    glGenBuffers(1, &toyCubeVBO);
    glBindBuffer(GL_ARRAY_BUFFER, toyCubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(toyCubeVertices), toyCubeVertices, GL_STATIC_DRAW);
    
    unsigned int stride = 12 * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(7 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, (void*)(10 * sizeof(float)));
    glEnableVertexAttribArray(3);
    glBindVertexArray(0);
    
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++ KREIRANJE SIJALICE +++++++++++++++++++++++++++++++++++++++++++++++++
    
    // Kreiramo sferu za sijalicu na vrhu automata
    float lightBulbRadius = 0.1f; // Poluprečnik sijalice - povećano da bude vidljivija
    int lightBulbSegments = 32; // Broj segmenata za glatku sferu
    
    std::vector<float> lightBulbVertices;
    std::vector<unsigned int> lightBulbIndices;
    
    // Generišemo sferu
    for (int lat = 0; lat <= lightBulbSegments; ++lat) {
        float theta = lat * 3.14159f / lightBulbSegments;
        float sinTheta = std::sin(theta);
        float cosTheta = std::cos(theta);
        
        for (int lon = 0; lon <= lightBulbSegments; ++lon) {
            float phi = lon * 2.0f * 3.14159f / lightBulbSegments;
            float sinPhi = std::sin(phi);
            float cosPhi = std::cos(phi);
            
            float x = lightBulbRadius * cosPhi * sinTheta;
            float y = lightBulbRadius * cosTheta;
            float z = lightBulbRadius * sinPhi * sinTheta;
            
            // Normalna za sferu
            float nx = x / lightBulbRadius;
            float ny = y / lightBulbRadius;
            float nz = z / lightBulbRadius;
            
            // Tekstura koordinate
            float u = (float)lon / lightBulbSegments;
            float v = (float)lat / lightBulbSegments;
            
            // Tamno plava boja
            lightBulbVertices.insert(lightBulbVertices.end(), {
                x, y, z,   0.1f, 0.2f, 0.4f, 1.0f,    u, v,    nx, ny, nz,
            });
        }
    }
    
    // Generišemo indices za sferu
    for (int lat = 0; lat < lightBulbSegments; ++lat) {
        for (int lon = 0; lon < lightBulbSegments; ++lon) {
            int first = lat * (lightBulbSegments + 1) + lon;
            int second = first + lightBulbSegments + 1;
            
            lightBulbIndices.push_back(first);
            lightBulbIndices.push_back(second);
            lightBulbIndices.push_back(first + 1);
            
            lightBulbIndices.push_back(second);
            lightBulbIndices.push_back(second + 1);
            lightBulbIndices.push_back(first + 1);
        }
    }
    
    unsigned int lightBulbVAO, lightBulbVBO, lightBulbEBO;
    glGenVertexArrays(1, &lightBulbVAO);
    glBindVertexArray(lightBulbVAO);
    glGenBuffers(1, &lightBulbVBO);
    glGenBuffers(1, &lightBulbEBO);
    
    glBindBuffer(GL_ARRAY_BUFFER, lightBulbVBO);
    glBufferData(GL_ARRAY_BUFFER, lightBulbVertices.size() * sizeof(float), lightBulbVertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lightBulbEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, lightBulbIndices.size() * sizeof(unsigned int), lightBulbIndices.data(), GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(7 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, (void*)(10 * sizeof(float)));
    glEnableVertexAttribArray(3);
    glBindVertexArray(0);
    
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++ OVERLAY – potpis (ime, prezime, indeks) u uglu +++++++++++++++++++++++++++++++++++++++++++++++++
    // Tekstura treba da sadrži "Anja Guzina RA 18/2022" velikim belim slovima (možete zameniti signature.png)
    unsigned int signatureTex = loadImageToTexture("Resources/signature.png");
    // Quad u NDC za donji levi ugao: ista širina, samo rastegnuto na gore da se bolje vide slova
    float overlayW = 0.42f, overlayH = 0.32f;   // samo visina povećana (rastegnuto na gore)
    float ox0 = -0.98f, oy0 = -0.94f;  // levo dole
    float overlayVerts[] = {
        ox0,         oy0,         0.0f,  1.0f, 1.0f, 1.0f, 1.0f,  0.0f, 0.0f,  0.0f, 1.0f, 0.0f,
        ox0,         oy0+overlayH, 0.0f,  1.0f, 1.0f, 1.0f, 1.0f,  0.0f, 1.0f,  0.0f, 1.0f, 0.0f,
        ox0+overlayW, oy0+overlayH, 0.0f,  1.0f, 1.0f, 1.0f, 1.0f,  1.0f, 1.0f,  0.0f, 1.0f, 0.0f,
        ox0+overlayW, oy0,         0.0f,  1.0f, 1.0f, 1.0f, 1.0f,  1.0f, 0.0f,  0.0f, 1.0f, 0.0f,
    };
    unsigned int overlayIndices[] = { 0, 1, 2,  0, 2, 3 };
    unsigned int overlayVAO, overlayVBO, overlayEBO;
    glGenVertexArrays(1, &overlayVAO);
    glGenBuffers(1, &overlayVBO);
    glGenBuffers(1, &overlayEBO);
    glBindVertexArray(overlayVAO);
    glBindBuffer(GL_ARRAY_BUFFER, overlayVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(overlayVerts), overlayVerts, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, overlayEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(overlayIndices), overlayIndices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(7 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, (void*)(10 * sizeof(float)));
    glEnableVertexAttribArray(3);
    glBindVertexArray(0);
    
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++ RENDER LOOP - PETLJA ZA CRTANJE +++++++++++++++++++++++++++++++++++++++++++++++++
    glUseProgram(unifiedShader);
    glClearColor(0.2f, 0.2f, 0.25f, 1.0f);
    
    if (clawMachine.indexCount == 0) {
        std::cout << "Greska: Model nije uspesno ucitan!" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    if (window == NULL) {
        std::cout << "Greska: Prozor ne postoji!" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    if (!glfwGetWindowAttrib(window, GLFW_VISIBLE)) {
        glfwShowWindow(window);
    }
    
    glViewport(0, 0, wWidth, wHeight);
    
    static double lastFrameTime = glfwGetTime();
    while (!glfwWindowShouldClose(window))
    {
        double currentTime = glfwGetTime();
        float dt = (float)(currentTime - lastFrameTime);
        lastFrameTime = currentTime;

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, GL_TRUE);
        }

        // Kursor: coin kad je svetlo ugašeno (automat isključen / početak / posle preuzimanja igračke), poluga kad je uključen
        if (lightOn)
            glfwSetCursor(window, cursorLever);
        else
            glfwSetCursor(window, cursorCoin);

        // Testiranje dubine (1 = uključi, 2 = isključi) i odstranjivanje naličja (3 = uključi, 4 = isključi) – kao na vežbama
        if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) depthTestEnabled = true;
        if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) depthTestEnabled = false;
        if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) cullFaceEnabled = true;
        if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS) cullFaceEnabled = false;

        // Strelice levo/desno – kamera se kreće po kružnoj putanji oko automata
        const float orbitSpeed = 55.0f;  // stepeni u sekundi
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)  cameraYaw += orbitSpeed * dt;
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) cameraYaw -= orbitSpeed * dt;

        // Treptanje zeleno-crveno samo dok čeka klik na osvojeru igračku (prizeBlinking); posle klika i ponovnog paljenja = svetlo plavo
        if (prizeBlinking)
        {
            blinkTimer += dt;
            if (blinkTimer >= 0.5f) { blinkTimer -= 0.5f; blinkGreen = !blinkGreen; }
        }
        
        // Test taster 'L' za ručno paljenje/gasenje sijalice
        static bool lKeyPressed = false;
        if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS && !lKeyPressed)
        {
            lightOn = !lightOn;
            std::cout << "Taster L - Sijalica " << (lightOn ? "UPALJENA" : "UGASENA") << "!" << std::endl;
            lKeyPressed = true;
        }
        if (glfwGetKey(window, GLFW_KEY_L) == GLFW_RELEASE)
        {
            lKeyPressed = false;
        }
        
        // Kontrole za kandžu – samo kad je kamera ispred automata, automat uključen, nema treptanja i sijalica je upaljena
        bool cameraInFront = (cameraYaw >= -cameraInFrontYawHalf && cameraYaw <= cameraInFrontYawHalf);
        static bool wPressed = false, aPressed = false, sPressed = false, dPressed = false;
        const float oneStep = 0.14f;  // veći korak po jednom pritisku WASD
        if (cameraInFront && machineOn && !prizeBlinking && lightOn)
        {
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS && !wPressed) { clawZ -= oneStep; wPressed = true; }
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_RELEASE) wPressed = false;
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS && !sPressed) { clawZ += oneStep; sPressed = true; }
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_RELEASE) sPressed = false;
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS && !aPressed) { clawX -= oneStep; aPressed = true; }
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_RELEASE) aPressed = false;
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS && !dPressed) { clawX += oneStep; dPressed = true; }
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_RELEASE) dPressed = false;
            static bool spacePressedPrev = false;
            bool carrying = (carriedWhich == 1 || carriedWhich == 2);
            if (carrying)
            {
                if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !spacePressedPrev)
                {
                    float cwx = machineScale * clawX, cwz = machineScale * clawZ;
                    // Mesto na koje igračka pada (ista visina kao pod – tu se odlučuje da li je „u rupi”)
                    float dropX = std::min(playFloorMaxX, std::max(playFloorMinX, cwx));
                    float dropZ = std::min(playFloorMaxZ, std::max(playFloorMinZ, cwz));
                    // inHole = da li mesto PADA (na dnu) ulazi u oblast rupe – tada ide u pregradu i trepće svetlo
                    bool inHole = (dropX >= holeMinX && dropX <= holeMaxX && dropZ >= holeMinZ && dropZ <= holeMaxZ);
                    std::cout << "[SPACE pustio] dropX=" << dropX << " dropZ=" << dropZ << " inHole=" << (inHole ? "DA" : "NE") << std::endl;
                    if (carriedWhich == 1)
                    {
                        if (inHole) {
                            toyWon = true; toyCubeX = prizeX; toyCubeY = prizeY; toyCubeZ = prizeZ;
                            prizeBlinking = true; blinkTimer = 0.0f; blinkGreen = true;
                            std::cout << "Osvojena igracka u pregradi -> sijalica treperi zeleno/crveno!" << std::endl;
                        }
                        else { toyCubeX = dropX; toyCubeY = toyFloorY; toyCubeZ = dropZ; }
                    }
                    else
                    {
                        if (inHole) {
                            birdWon = true; birdToyX = prizeX; birdToyY = prizeY; birdToyZ = prizeZ;
                            prizeBlinking = true; blinkTimer = 0.0f; blinkGreen = true;
                            std::cout << "Osvojena igracka u pregradi -> sijalica treperi zeleno/crveno!" << std::endl;
                        }
                        else { birdOnFloor = true; birdToyX = dropX; birdToyY = toyFloorY; birdToyZ = dropZ; }
                    }
                    carriedWhich = 0;
                }
                spacePressedPrev = (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS);
                if (clawY < 0.5f)
                {
                    clawY += clawLowerSpeed;
                    if (clawY > 0.5f) clawY = 0.5f;
                }
            }
            else
            {
                if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
                {
                    clawY -= clawLowerSpeed;
                    if (clawY < -1.28f) clawY = -1.28f;
                }
                else
                {
                    if (clawY < 0.5f)
                    {
                        if (clawY <= -1.22f)
                        {
                            float cwx = machineScale * clawX, cwz = machineScale * clawZ;
                            float dxC = cwx - toyCubeX, dzC = cwz - toyCubeZ;
                            float dxB = cwx - birdToyX, dzB = cwz - birdToyZ;
                            float r2 = grabRadiusWorld * grabRadiusWorld;
                            if (!toyWon && dxC*dxC + dzC*dzC < r2)
                                carriedWhich = 1;
                            else if (!birdWon && dxB*dxB + dzB*dzB < r2) {
                                carriedWhich = 2;
                                birdOnFloor = false;  // više nije na dnu, u kandži je
                            }
                        }
                        clawY += clawLowerSpeed;
                        if (clawY > 0.5f) clawY = 0.5f;
                    }
                }
                spacePressedPrev = (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS);
            }
        }
        
        // Granice = 4 ugla automata; kandža kreće od centra (0,0), ne sme van ovih granica
        const float cornerMinX = -1.3f;
        const float cornerMaxX =  1.3f;
        const float cornerMinZ = -1.3f;
        const float cornerMaxZ =  1.3f;
        if (clawX > cornerMaxX) clawX = cornerMaxX;
        if (clawX < cornerMinX) clawX = cornerMinX;
        if (clawZ > cornerMaxZ) clawZ = cornerMaxZ;
        if (clawZ < cornerMinZ) clawZ = cornerMinZ;

        // Računamo poziciju kamere na osnovu rotacije
        float yawRad = glm::radians(cameraYaw);
        float pitchRad = glm::radians(cameraPitch);
        
        glm::vec3 cameraPos;
        cameraPos.x = cameraTarget.x + cameraDistance * cos(pitchRad) * sin(yawRad);
        cameraPos.y = cameraTarget.y + cameraDistance * sin(pitchRad);
        cameraPos.z = cameraTarget.z + cameraDistance * cos(pitchRad) * cos(yawRad);
        
        // Ažuriramo view matricu
        view = glm::lookAt(cameraPos, cameraTarget, cameraUp);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glUseProgram(unifiedShader);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projectionP));

        // Osvetljenje
        glm::vec3 lightPos(2.0f, 3.0f, 2.0f);
        glm::vec3 viewPos = cameraPos; // Koristimo trenutnu poziciju kamere
        glm::vec3 lightColor(1.0f, 1.0f, 1.0f);
        glUniform3f(glGetUniformLocation(unifiedShader, "uLightPos"), lightPos.x, lightPos.y, lightPos.z);
        glUniform3f(glGetUniformLocation(unifiedShader, "uViewPos"), viewPos.x, viewPos.y, viewPos.z);
        glUniform3f(glGetUniformLocation(unifiedShader, "uLightColor"), lightColor.r, lightColor.g, lightColor.b);
        
        // Materijal
        glUniform3f(glGetUniformLocation(unifiedShader, "uMaterialAmbient"), 0.3f, 0.3f, 0.3f);
        glUniform3f(glGetUniformLocation(unifiedShader, "uMaterialDiffuse"), 0.8f, 0.8f, 0.8f);
        glUniform3f(glGetUniformLocation(unifiedShader, "uMaterialSpecular"), 0.5f, 0.5f, 0.5f);
        glUniform1f(glGetUniformLocation(unifiedShader, "uMaterialShininess"), 64.0f);
        
        // PRVO RENDERUJEMO NEprozirne objekte PRE automata
        
        // 1. Renderovanje sijalice na vrhu automata - PRVO
        glm::mat4 lightBulbMatrix = glm::mat4(1.0f);
        lightBulbMatrix = glm::translate(lightBulbMatrix, glm::vec3(0.0f, 0.5f, 0.0f)); // Na vrhu automata (u world space) - još više na vrhu
        lightBulbMatrix = glm::scale(lightBulbMatrix, glm::vec3(0.5f, 0.5f, 0.5f)); // Još povećano skaliranje da bude vidljivija
        
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(lightBulbMatrix));
        glUniform1i(glGetUniformLocation(unifiedShader, "transparent"), 0);
        glUniform1i(glGetUniformLocation(unifiedShader, "useTex"), 0);
        
        // Boja sijalice: zeleno-crveno samo dok treperi (čekanje klika na igračku); posle klika + crveno dugme = svetlo plavo
        if (prizeBlinking)
        {
            // Jako ambient + diffuse da sijalica bude dobro vidljiva (ne tamni od osvetljenja)
            if (blinkGreen) {
                glUniform4f(glGetUniformLocation(unifiedShader, "uColor"), 0.0f, 1.0f, 0.0f, 1.0f);
                glUniform3f(glGetUniformLocation(unifiedShader, "uMaterialAmbient"), 1.0f, 1.0f, 1.0f);
                glUniform3f(glGetUniformLocation(unifiedShader, "uMaterialDiffuse"), 1.0f, 1.0f, 1.0f);
            } else {
                glUniform4f(glGetUniformLocation(unifiedShader, "uColor"), 1.0f, 0.0f, 0.0f, 1.0f);
                glUniform3f(glGetUniformLocation(unifiedShader, "uMaterialAmbient"), 1.0f, 1.0f, 1.0f);
                glUniform3f(glGetUniformLocation(unifiedShader, "uMaterialDiffuse"), 1.0f, 1.0f, 1.0f);
            }
        }
        else if (!machineOn)
        {
            glUniform4f(glGetUniformLocation(unifiedShader, "uColor"), 0.1f, 0.2f, 0.4f, 1.0f); // Tamno plava = automat OFF
            glUniform3f(glGetUniformLocation(unifiedShader, "uMaterialAmbient"), 0.2f, 0.3f, 0.5f);
            glUniform3f(glGetUniformLocation(unifiedShader, "uMaterialDiffuse"), 0.3f, 0.4f, 0.6f);
        }
        else if (lightOn)
        {
            glUniform4f(glGetUniformLocation(unifiedShader, "uColor"), 0.0f, 0.8f, 1.0f, 1.0f); // Svetlo plava kada je upaljena
            glUniform3f(glGetUniformLocation(unifiedShader, "uMaterialAmbient"), 0.5f, 0.7f, 1.0f);
            glUniform3f(glGetUniformLocation(unifiedShader, "uMaterialDiffuse"), 0.6f, 0.8f, 1.0f);
        }
        else
        {
            glUniform4f(glGetUniformLocation(unifiedShader, "uColor"), 0.1f, 0.2f, 0.4f, 1.0f);
            glUniform3f(glGetUniformLocation(unifiedShader, "uMaterialAmbient"), 0.2f, 0.3f, 0.5f);
            glUniform3f(glGetUniformLocation(unifiedShader, "uMaterialDiffuse"), 0.3f, 0.4f, 0.6f);
        }
        // Material specular ostaje isti za oba stanja
        glUniform3f(glGetUniformLocation(unifiedShader, "uMaterialSpecular"), 0.6f, 0.7f, 0.9f);
        glUniform1f(glGetUniformLocation(unifiedShader, "uMaterialShininess"), 64.0f);
        
        glBindVertexArray(lightBulbVAO);
        glDrawElements(GL_TRIANGLES, (unsigned int)lightBulbIndices.size(), GL_UNSIGNED_INT, (void*)0);
        glBindVertexArray(0);
        
        // ZATIM RENDERUJEMO .obj model automata - PRVO NEPROZIRNI DELOVI (medved i zec se crtaju POSLE da ne budu zaklonjeni)
        // Resetujemo OpenGL stanja pre renderovanja automata (poštujemo tastere 1–4 za dubinu i culling)
        if (depthTestEnabled) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        if (cullFaceEnabled) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
        
        glUniform1i(glGetUniformLocation(unifiedShader, "useTex"), 0); // Ne koristimo teksturu za .obj model
        
        glBindVertexArray(clawMachine.VAO);
        
        // Grupiši indekse po materijalima - kreiraj mapu (matID -> lista (startIndex, count))
        std::map<unsigned int, std::vector<std::pair<size_t, size_t>>> materialRanges;
        unsigned int currentMatID = (clawMachine.materialIndices.size() > 0) ? clawMachine.materialIndices[0] : 0;
        size_t rangeStart = 0;
        
        for (size_t i = 0; i < clawMachine.indices.size(); ++i) {
            unsigned int matID = (i < clawMachine.materialIndices.size()) ? clawMachine.materialIndices[i] : 0;
            
            if (matID != currentMatID) {
                // Završi prethodni range
                if (i > rangeStart) {
                    materialRanges[currentMatID].push_back(std::make_pair(rangeStart, i - rangeStart));
                }
                // Počni novi range
                currentMatID = matID;
                rangeStart = i;
            }
        }
        // Završi poslednji range
        if (clawMachine.indices.size() > rangeStart) {
            materialRanges[currentMatID].push_back(std::make_pair(rangeStart, clawMachine.indices.size() - rangeStart));
        }
        
        // Pronađi materijal po ID - kreiraj mapu ID -> ime
        std::map<unsigned int, std::string> matIDToName;
        unsigned int id = 0;
        for (const auto& pair : clawMachine.materials) {
            matIDToName[id++] = pair.first;
        }
        
        // PRVO RENDERUJEMO NEPROZIRNE DELOVE AUTOMATA
        for (const auto& group : materialRanges) {
            unsigned int matID = group.first;
            const std::vector<std::pair<size_t, size_t>>& ranges = group.second;
            
            // Pronađi materijal
            Material* mat = nullptr;
            if (matIDToName.find(matID) != matIDToName.end()) {
                std::string matName = matIDToName[matID];
                if (clawMachine.materials.find(matName) != clawMachine.materials.end()) {
                    mat = &clawMachine.materials[matName];
                }
            }
            
            // Ako nema materijala, koristi default
            if (!mat && !clawMachine.materials.empty()) {
                mat = &clawMachine.materials.begin()->second;
            }
            
            if (!mat) continue;
            
            // Preskoči transparentne delove - renderujemo ih posle kandže
            if (mat->d < 1.0f) continue;
            
            std::string matName = (matIDToName.find(matID) != matIDToName.end()) ? matIDToName[matID] : "";
            if (matName == "pink_frame") continue;
            if (matName == "black") continue;
            if (matName == "bird" || matName == "bird_red") continue;  // pticu ne crtamo uopšte
            
            // Kandža (pink) se pomera; ostali delovi koriste modelMatrix
            if (matName == "pink") {
                glm::mat4 pinkMatrix = glm::translate(modelMatrix, glm::vec3(clawX, clawY, clawZ));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(pinkMatrix));
            } else {
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
            }
            // Dno automata - skin materijal kao u popravka.obj (opaque sivo)
            if (matName == "skin" || matName == "floor_metal") {
                glUniform3f(glGetUniformLocation(unifiedShader, "uMaterialAmbient"), mat->Ka.r, mat->Ka.g, mat->Ka.b);
                glUniform3f(glGetUniformLocation(unifiedShader, "uMaterialDiffuse"), mat->Kd.r, mat->Kd.g, mat->Kd.b);
                glUniform3f(glGetUniformLocation(unifiedShader, "uMaterialSpecular"), mat->Ks.r, mat->Ks.g, mat->Ks.b);
                glUniform1f(glGetUniformLocation(unifiedShader, "uMaterialShininess"), mat->Ns);
                glUniform4f(glGetUniformLocation(unifiedShader, "uColor"), mat->Kd.r, mat->Kd.g, mat->Kd.b, 1.0f);
                glDisable(GL_CULL_FACE);  // crtaj obe strane da se dno uvek vidi
            } else {
                glUniform3f(glGetUniformLocation(unifiedShader, "uMaterialAmbient"), mat->Ka.r, mat->Ka.g, mat->Ka.b);
                glUniform3f(glGetUniformLocation(unifiedShader, "uMaterialDiffuse"), mat->Kd.r, mat->Kd.g, mat->Kd.b);
                glUniform3f(glGetUniformLocation(unifiedShader, "uMaterialSpecular"), mat->Ks.r, mat->Ks.g, mat->Ks.b);
                glUniform1f(glGetUniformLocation(unifiedShader, "uMaterialShininess"), mat->Ns);
                glUniform4f(glGetUniformLocation(unifiedShader, "uColor"), mat->Kd.r, mat->Kd.g, mat->Kd.b, mat->d);
                if (cullFaceEnabled) glEnable(GL_CULL_FACE);
            }
            glUniform1i(glGetUniformLocation(unifiedShader, "transparent"), 0);
            
            // Renderuj sve range-ove za ovaj materijal
            for (const auto& range : ranges) {
                size_t startIdx = range.first;
                size_t count = range.second;
                glDrawElements(GL_TRIANGLES, (unsigned int)count, GL_UNSIGNED_INT, (void*)(startIdx * sizeof(unsigned int)));
            }
            if (matName == "skin" || matName == "floor_metal")
                if (cullFaceEnabled) glEnable(GL_CULL_FACE);  // vrati culling za sledeći materijal
        }
        
        glBindVertexArray(0);
        
        // Medved (prva igračka) – crtamo osim kad je u kandži ili kad je pokupljen (nestane)
        if (carriedWhich != 1 && bearModel.indexCount > 0 && !(toyWon && toyCollected))
        {
            float scale = toyWon ? (bearScale * prizeInCompartmentScale) : bearScale;  // u pregradi manje da ne strči kroz metal
            glm::mat4 bearMatrix = glm::mat4(1.0f);
            bearMatrix = glm::translate(bearMatrix, glm::vec3(toyCubeX, toyCubeY, toyCubeZ));
            bearMatrix = glm::scale(bearMatrix, glm::vec3(scale, scale, scale));
            drawOBJModel(bearModel, bearMatrix, modelLoc, unifiedShader);
        }
        
        // Igračka u kandži – medved ili zec na poziciji kandže
        if (carriedWhich == 1 || carriedWhich == 2)
        {
            float cwx = machineScale * clawX, cwy = machineScale * (clawY - clawTipOffset), cwz = machineScale * clawZ;
            glm::mat4 carriedMatrix = glm::mat4(1.0f);
            carriedMatrix = glm::translate(carriedMatrix, glm::vec3(cwx, cwy, cwz));
            if (carriedWhich == 1 && bearModel.indexCount > 0) {
                carriedMatrix = glm::scale(carriedMatrix, glm::vec3(bearScale, bearScale, bearScale));
                drawOBJModel(bearModel, carriedMatrix, modelLoc, unifiedShader);
            } else if (carriedWhich == 2 && rabbitModel.indexCount > 0) {
                carriedMatrix = glm::rotate(carriedMatrix, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                carriedMatrix = glm::scale(carriedMatrix, glm::vec3(rabbitScale, rabbitScale, rabbitScale));
                drawOBJModel(rabbitModel, carriedMatrix, modelLoc, unifiedShader);
            }
        }
        
        // Zec (druga igračka) – crtamo osim kad je u kandži ili kad je pokupljen (nestane)
        if (carriedWhich != 2 && rabbitModel.indexCount > 0 && !(birdWon && birdCollected))
        {
            float scale = birdWon ? (rabbitScale * prizeInCompartmentScale) : rabbitScale;  // u pregradi manje da uvo ne strči kroz metal
            glm::mat4 rabbitMatrix = glm::mat4(1.0f);
            rabbitMatrix = glm::translate(rabbitMatrix, glm::vec3(birdToyX, birdToyY, birdToyZ));
            rabbitMatrix = glm::rotate(rabbitMatrix, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            rabbitMatrix = glm::scale(rabbitMatrix, glm::vec3(scale, scale, scale));
            drawOBJModel(rabbitModel, rabbitMatrix, modelLoc, unifiedShader);
        }
        
        // NA KRAJU RENDERUJEMO TRANSPARENTNE DELOVE AUTOMATA (staklo) - da se kandža vidi kroz njih
        if (depthTestEnabled) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_CULL_FACE); // Isključeno za transparent objekte
        
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
        glUniform1i(glGetUniformLocation(unifiedShader, "useTex"), 0);
        
        glBindVertexArray(clawMachine.VAO);
        
        // Renderuj samo transparentne delove automata (staklo)
        for (const auto& group : materialRanges) {
            unsigned int matID = group.first;
            const std::vector<std::pair<size_t, size_t>>& ranges = group.second;
            
            // Pronađi materijal
            Material* mat = nullptr;
            if (matIDToName.find(matID) != matIDToName.end()) {
                std::string matName = matIDToName[matID];
                if (clawMachine.materials.find(matName) != clawMachine.materials.end()) {
                    mat = &clawMachine.materials[matName];
                }
            }
            
            if (!mat && !clawMachine.materials.empty()) {
                mat = &clawMachine.materials.begin()->second;
            }
            
            if (!mat) continue;
            
            // Renderuj samo transparentne delove
            if (mat->d >= 1.0f) continue;
            
            // Postavi materijal uniforme
            glUniform3f(glGetUniformLocation(unifiedShader, "uMaterialAmbient"), mat->Ka.r, mat->Ka.g, mat->Ka.b);
            glUniform3f(glGetUniformLocation(unifiedShader, "uMaterialDiffuse"), mat->Kd.r, mat->Kd.g, mat->Kd.b);
            glUniform3f(glGetUniformLocation(unifiedShader, "uMaterialSpecular"), mat->Ks.r, mat->Ks.g, mat->Ks.b);
            glUniform1f(glGetUniformLocation(unifiedShader, "uMaterialShininess"), mat->Ns);
            
            // Postavi boju i transparentnost
            glUniform4f(glGetUniformLocation(unifiedShader, "uColor"), mat->Kd.r, mat->Kd.g, mat->Kd.b, mat->d);
            glUniform1i(glGetUniformLocation(unifiedShader, "transparent"), 1);
            glDisable(GL_CULL_FACE);
            
            // Renderuj sve range-ove za ovaj materijal
            for (const auto& range : ranges) {
                size_t startIdx = range.first;
                size_t count = range.second;
                glDrawElements(GL_TRIANGLES, (unsigned int)count, GL_UNSIGNED_INT, (void*)(startIdx * sizeof(unsigned int)));
            }
        }
        
        glBindVertexArray(0);
        
        // Overlay – poluprovidna tekstura sa imenom, prezimenom i indeksom (donji levi ugao)
        {
            glm::mat4 savedProj = projectionP;
            glm::mat4 savedView = view;
            glm::mat4 orthoProj = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
            glm::mat4 identityView = glm::mat4(1.0f);
            glm::mat4 identityModel = glm::mat4(1.0f);
            glDisable(GL_DEPTH_TEST);
            glUseProgram(unifiedShader);
            glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(orthoProj));
            glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(identityView));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(identityModel));
            glUniform1i(glGetUniformLocation(unifiedShader, "useTex"), 1);
            glUniform1i(glGetUniformLocation(unifiedShader, "overlayMode"), 1);  // bez osvetljenja – tekstura direktno, slova oštra
            glUniform1i(glGetUniformLocation(unifiedShader, "transparent"), 0);
            glUniform4f(glGetUniformLocation(unifiedShader, "uColor"), 1.0f, 1.0f, 1.0f, 0.95f);  // skoro puna vidljivost
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, signatureTex);
            glBindVertexArray(overlayVAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
            if (depthTestEnabled) glEnable(GL_DEPTH_TEST);
            glUniform1i(glGetUniformLocation(unifiedShader, "overlayMode"), 0);  // vrati za 3D scenu
            glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(savedProj));
            glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(savedView));
        }
        
        glfwSwapBuffers(window);
        glfwPollEvents();

        // Frame limiter — 75 FPS
        {
            static double frameLimitStart = glfwGetTime();
            double targetFrameTime = 1.0 / 75.0;
            while (glfwGetTime() - frameLimitStart < targetFrameTime) {}
            frameLimitStart = glfwGetTime();
        }
    }
    
    // ========== KRAJ NOVOG RENDER LOOP-A ==========
    
 

    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++ POSPREMANJE +++++++++++++++++++++++++++++++++++++++++++++++++

    // Cleanup za .obj model automata
    glDeleteBuffers(1, &clawMachine.EBO);
    glDeleteBuffers(1, &clawMachine.VBO);
    glDeleteVertexArrays(1, &clawMachine.VAO);
    
    // Cleanup za kandžu
    glDeleteBuffers(1, &claw.EBO);
    glDeleteBuffers(1, &claw.VBO);
    glDeleteVertexArrays(1, &claw.VAO);
    
    // Cleanup za roze kocku (više se ne crta, ali VAO ostaje za kompatibilnost)
    glDeleteBuffers(1, &toyCubeVBO);
    glDeleteVertexArrays(1, &toyCubeVAO);
    
    // Cleanup za medveda i zeca
    glDeleteBuffers(1, &bearModel.EBO);
    glDeleteBuffers(1, &bearModel.VBO);
    glDeleteVertexArrays(1, &bearModel.VAO);
    glDeleteBuffers(1, &rabbitModel.EBO);
    glDeleteBuffers(1, &rabbitModel.VBO);
    glDeleteVertexArrays(1, &rabbitModel.VAO);
    
    // Cleanup za sijalicu
    glDeleteBuffers(1, &lightBulbEBO);
    glDeleteBuffers(1, &lightBulbVBO);
    glDeleteVertexArrays(1, &lightBulbVAO);
    
    // Cleanup za overlay (potpis)
    glDeleteBuffers(1, &overlayEBO);
    glDeleteBuffers(1, &overlayVBO);
    glDeleteVertexArrays(1, &overlayVAO);
    glDeleteTextures(1, &signatureTex);
    
    glDeleteProgram(unifiedShader);

    glfwDestroyCursor(cursorCoin);
    glfwDestroyCursor(cursorLever);
    glfwTerminate();
    return 0;
}
