#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

//#include "stb_image.h"

#include <random>
#include <unordered_set>
#include <cmath>

#include "Texture.h"
#include "shaderClass.h"
#include "Vertex.h"
#include "VAO.h"
#include "VBO.h"
#include "EBO.h"
#include "Camera.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"


const unsigned int width = 800;
const unsigned int height = 600;




GLfloat lightVertices[] =
{ //     COORDINATES     //
    -1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f, -1.0f,
    1.0f, -1.0f, -1.0f,
    1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,
    1.0f,  1.0f, -1.0f,
    1.0f,  1.0f,  1.0f
};

GLuint lightIndices[] =
{
    0, 1, 2,
    0, 2, 3,
    0, 4, 7,
    0, 7, 3,
    3, 7, 6,
    3, 6, 2,
    2, 6, 5,
    2, 5, 1,
    1, 5, 4,
    1, 4, 0,
    4, 5, 6,
    4, 6, 7
};



bool loadObj(const std::string& objFilePath, std::vector<Vertex>& vertices, std::vector<GLuint>& indices) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    // Cargar el archivo .obj utilizando tinyobjloader
    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, objFilePath.c_str());

    if (!warn.empty()) {
        std::cout << "Warning loading OBJ: " << warn << std::endl;
    }

    if (!err.empty()) {
        std::cerr << "Error loading OBJ: " << err << std::endl;
    }

    if (!ret) {
        std::cerr << "Failed to load OBJ file: " << objFilePath << std::endl;
        return false;
    }

    // Procesar los vértices y los índices
    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex;

            // Posiciones
            vertex.position[0] = attrib.vertices[3 * index.vertex_index + 0];
            vertex.position[1] = attrib.vertices[3 * index.vertex_index + 1];
            vertex.position[2] = attrib.vertices[3 * index.vertex_index + 2];

            // Normales
            vertex.normal[0] = attrib.normals[3 * index.normal_index + 0];
            vertex.normal[1] = attrib.normals[3 * index.normal_index + 1];
            vertex.normal[2] = attrib.normals[3 * index.normal_index + 2];

            // Coordenadas de textura
            if (index.texcoord_index >= 0) {
                vertex.texCoord[0] = attrib.texcoords[2 * index.texcoord_index + 0];
                vertex.texCoord[1] = attrib.texcoords[2 * index.texcoord_index + 1];
            }

            // Colores (si están disponibles en tu modelo)
            vertex.color[0] = attrib.colors[3 * index.vertex_index + 0];
            vertex.color[1] = attrib.colors[3 * index.vertex_index + 1];
            vertex.color[2] = attrib.colors[3 * index.vertex_index + 2];

            vertices.push_back(vertex);
            indices.push_back(indices.size());
        }
    }

    return true;
}



struct Model {
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
    VAO vao;
    VBO vbo;
    EBO ebo;
    std::string ModelName;
    Texture texture;

    Model(const std::vector<Vertex>& vertices, const std::vector<GLuint>& indices, const std::string& modelName, const Texture& texture)
    : vertices(vertices), indices(indices), texture(texture) ,ModelName(modelName),
    vao(), vbo(vertices), ebo(indices) {
        vao.Bind();
        vbo.Bind();
        vao.LinkAttrib(vbo, 0, 3, GL_FLOAT, sizeof(Vertex), (void*)offsetof(Vertex, position));
        vao.LinkAttrib(vbo, 1, 3, GL_FLOAT, sizeof(Vertex), (void*)offsetof(Vertex, color));
        vao.LinkAttrib(vbo, 2, 2, GL_FLOAT, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
        vao.LinkAttrib(vbo, 3, 3, GL_FLOAT, sizeof(Vertex), (void*)offsetof(Vertex, normal));
        ebo.Bind();
        vao.Unbind();
        vbo.Unbind();
        ebo.Unbind();
    }
};







// Función para verificar si la nueva posición está a una distancia mínima de las existentes.
bool isFarEnough(const glm::vec3& newPos, const std::vector<glm::vec3>& positions, float minDist) {
    for (const auto& pos : positions) {
        if (glm::distance(newPos, pos) < minDist) {
            return false;
        }
    }
    return true;
}


// Función para verificar si la nueva roca está tocando dos o tres rocas existentes y devolver las rocas tocadas.
std::vector<glm::vec3> findTouchingRocks(const glm::vec3& newPos, const std::vector<glm::vec3>& positions, float touchDist) {
    std::vector<glm::vec3> touchingRocks;
    for (const auto& pos : positions) {
        if (glm::distance(newPos, pos) < touchDist) {
            touchingRocks.push_back(pos);
            if (touchingRocks.size() >= 3) {
                break;
            }
        }
    }
    return touchingRocks;
}





void updateWaveModel(Model& model, float time) {
    float amplitude = 100.0f; // Amplitud de las olas
    float frequency = 0.5f; // Frecuencia de las olas
    float speed = 10.0f;     // Velocidad de las olas

    int index = 1; // Comenzamos desde el segundo elemento en el arreglo de vértices (índice 1)
    for (int i = 0; i < model.vertices.size(); i += 3) {
        // Modificar la coordenada Y de cada vértice usando una función sinusoidal en función del tiempo
            model.vertices[i].position[1] = amplitude * sin(frequency * (model.vertices[i].position[0] + model.vertices[i].position[2] + time * speed));

    }

    // Actualizar el VBO con los nuevos vértices
    model.vbo.Bind();
    glBufferData(GL_ARRAY_BUFFER, model.vertices.size() * sizeof(Vertex),  model.vertices.data(), GL_STATIC_DRAW);
    model.vbo.Unbind();
}







Model loadModel(const std::string& objFilePath, const std::string& texturePath, bool isPNG, Shader& shaderProgram, GLuint i);
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}
int main()
{	
	
	// Inicializa GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Configura GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // Mayor versión de OpenGL
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3); // Menor versión de OpenGL
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // Usar el perfil core de OpenGL

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Necesario para MacOS
#endif

    // Crea una ventana de GLFW
    GLFWwindow* window = glfwCreateWindow(800, 600, "Multiple OBJ Loader", NULL, NULL);
    if (window == NULL) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Haz que el contexto de OpenGL sea el contexto actual
    glfwMakeContextCurrent(window);

    // Configura el callback para redimensionar la ventana
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // Inicializa GLAD
    gladLoadGL(glfwGetProcAddress);


    // Especifica el viewport de OpenGL en la ventana
    glViewport(0, 0, 800, 600);


	
    // Generates Shader object using shaders default.vert and default.frag
    Shader shaderProgram("default.vert", "default.frag");

	//std::cout << "Probando donde esta el error "<< std::endl;


    // Inicializa el generador de números aleatorios.
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> distPosXZ(-1300.0f, 1300.0f); // Rango para posiciones X y Z.
    std::uniform_real_distribution<float> distPosY(-200.0f, 1000.0f); // Rango para posiciones Y de los peces.
    std::uniform_real_distribution<float> distXZ(-1000.0f, 1000.0f); // Rango para posiciones X y Z de corales.
    // Distribución para el ángulo theta y el radio.
    std::uniform_real_distribution<float> distTheta(0.0f, 2 * M_PI);
    std::uniform_real_distribution<float> distRadius(1000.0f, 1800.0f); // Radios más grandes para estar cerca de los bordes.
    std::uniform_int_distribution<int> distRockIndex(0, 49); // Distribucion para seleccionar rocas de forma aleatoria


    std::vector<glm::vec3> fishPositions;
    std::vector<glm::vec3> rockPositions = { glm::vec3(0.0f, -700.0f, 0.0f) };
    std::vector<glm::vec3> coralPositions;
    std::unordered_set<int> usedRockIndices; // para no mas de 2 corales en la misma roca


    // Generar posiciones para los peces.
    for (int i = 0; i < 14; ++i) {
        glm::vec3 newPos;
        do {
            newPos = glm::vec3(distPosXZ(gen), distPosY(gen), distPosXZ(gen));
        } while (!isFarEnough(newPos, fishPositions, 400.0f));
        fishPositions.push_back(newPos);
    }

    // Generar posiciones para las rocas distribuidas cerca de los bordes de un área circular.
    for (int i = 0; i < 99; ++i) {
        glm::vec3 newPos;
        do {
            float theta = distTheta(gen); // Ángulo en radianes
            float radius = distRadius(gen); // Distancia desde el centro
            newPos = glm::vec3(radius * cos(theta), -700.0f, radius * sin(theta));

            // Verificar si la nueva roca está tocando dos o tres rocas existentes.
            auto touchingRocks = findTouchingRocks(newPos, rockPositions, 200.0f);
            if (touchingRocks.size() == 2 || touchingRocks.size() == 3) {
                //std::cout << "Encontro" << std::endl;
                glm::vec3 centerPos(0.0f);
                for (const auto& rock : touchingRocks) {
                    centerPos += rock;
                }
                centerPos /= static_cast<float>(touchingRocks.size());
                newPos = centerPos + glm::vec3(0.0f, 100.0f, 0.0f); // Colocar la nueva roca encima.
            }

        } while (!isFarEnough(newPos, rockPositions, 100.0f)); // Asegurarse de que las rocas no se sobrepongan mucho.
        rockPositions.push_back(newPos);
    }


    // Generar posiciones para corales encima de las rocas.
    for (int i = 0; i < 10; ++i) { // Asumiendo que hay 10 corales
        int rockIndex;
        bool validRockFound = false;

        // Intentar encontrar una roca válida que no tenga otra roca encima.
        for (int attempt = 0; attempt < 100; ++attempt) { // Limitar el número de intentos para evitar un bucle infinito.
            rockIndex = distRockIndex(gen); // Seleccionar una roca aleatoria
            if (usedRockIndices.find(rockIndex) == usedRockIndices.end()) { // Asegurarse de no reutilizar la misma roca
                // Verificar si esta roca tiene otra roca encima.
                bool hasRockAbove = false;
                glm::vec3 rockPos = rockPositions[rockIndex];
                for (const auto& pos : rockPositions) {
                    if (pos != rockPos && glm::distance(rockPos, pos) < 150.0f && pos.y > rockPos.y) {
                        hasRockAbove = true;
                        break;
                    }
                }

                if (!hasRockAbove) {
                    usedRockIndices.insert(rockIndex); // Marcar esta roca como usada
                    glm::vec3 coralPos = rockPos + glm::vec3(0.0f, 100.0f, 0.0f); // Ajustar la altura
                    coralPositions.push_back(coralPos);
                    validRockFound = true;
                    break;
                }
            }
        }

        if (!validRockFound) {
            std::cerr << "No valid rock found for coral placement" << std::endl;
        }
    }



    // Generar posiciones para piso y agua (si son necesarias).
    glm::vec3 floorPosition = glm::vec3(0.0f, -800.0f, 0.0f);
    glm::vec3 superPosition = glm::vec3(-2000.0f, 1250.0f, -1800.0f);
    glm::vec3 waterPosition = glm::vec3(0.0f, 300.0f, 0.0f);
    glm::vec3 tablePosition = glm::vec3(0.0f, -2400.0f, 0.0f);
    glm::vec3 pared1Position = glm::vec3(-10000.0f, -4000.0f, -8000.0f);
    glm::vec3 pared2Position = glm::vec3(-10000.0f, -4000.0f, 8000.0f);
    glm::vec3 pared3Position = glm::vec3(10000.0f, -4000.0f, -10000.0f);
    glm::vec3 pared4Position = glm::vec3(-10000.0f, -4000.0f, -10000.0f);
    glm::vec3 pisoPosition = glm::vec3(-10000.0f, -3800.0f, -10000.0f);
    glm::vec3 techoPosition = glm::vec3(-10000.0f, 6000.0f, -10000.0f);




    // Combinar todas las posiciones en un solo vector.
    std::vector<glm::vec3> allPositions = fishPositions;
    allPositions.insert(allPositions.end(), rockPositions.begin(), rockPositions.end());
    allPositions.insert(allPositions.end(), coralPositions.begin(), coralPositions.end());
    allPositions.push_back(floorPosition);
    allPositions.push_back(tablePosition);
    allPositions.push_back(pared1Position);
    allPositions.push_back(pared2Position);
    allPositions.push_back(pared3Position);
    allPositions.push_back(pared4Position);
    allPositions.push_back(pisoPosition);
    allPositions.push_back(techoPosition);

    allPositions.push_back(superPosition);
    allPositions.push_back(waterPosition);




    std::vector<Model> models;
    models.push_back(loadModel("Models/TropicalFish01.obj", "Models/TropicalFish01.jpg", false, shaderProgram, 0));
    models.push_back(loadModel("Models/TropicalFish02.obj", "Models/TropicalFish02.jpg", false, shaderProgram, 0));
    models.push_back(loadModel("Models/TropicalFish03.obj", "Models/TropicalFish03.jpg", false, shaderProgram, 0));
    models.push_back(loadModel("Models/TropicalFish04.obj", "Models/TropicalFish04.jpg", false, shaderProgram, 0));
    models.push_back(loadModel("Models/TropicalFish05.obj", "Models/TropicalFish05.jpg", false, shaderProgram, 0));
    models.push_back(loadModel("Models/TropicalFish06.obj", "Models/TropicalFish06.jpg", false ,shaderProgram, 0));
    models.push_back(loadModel("Models/TropicalFish07.obj", "Models/TropicalFish07.jpg", false, shaderProgram, 0));
    models.push_back(loadModel("Models/TropicalFish08.obj", "Models/TropicalFish08.jpg", false,  shaderProgram, 0));
    models.push_back(loadModel("Models/TropicalFish09.obj", "Models/TropicalFish09.jpg", false, shaderProgram, 0));
    models.push_back(loadModel("Models/TropicalFish10.obj", "Models/TropicalFish10.jpg", false, shaderProgram, 0));
    models.push_back(loadModel("Models/TropicalFish11.obj", "Models/TropicalFish11.jpg", false, shaderProgram, 0));
    models.push_back(loadModel("Models/TropicalFish12.obj", "Models/TropicalFish12.jpg", false, shaderProgram, 0));
    models.push_back(loadModel("Models/TropicalFish13.obj", "Models/TropicalFish13.jpg", false, shaderProgram, 0));
    models.push_back(loadModel("Models/TropicalFish14.obj", "Models/TropicalFish14.jpg", false, shaderProgram, 0));


    Model rocas = loadModel("Models/Roca-Test.obj", "Models/Rock-Texture-Surface.jpg", false,  shaderProgram, 0);
    int conta = 0;
    for (int i = 0; i < 100; i++) {
        //rocas.position =  allPositions[14+i+1];
        models.push_back(rocas);
        conta = 14 + i + 1;
    }


   Model coral1 = loadModel("Models/coral_v1.obj", "Models/coral01.jpg", false, shaderProgram, 0);

    Model coral2 = loadModel("Models/coral2.obj", "Models/coral2.jpg", false, shaderProgram, 0);
    for (int i = 0; i < 5; i++) {
        //coral1.position = allPositions[117 + i];
        models.push_back(coral1);
    }

    for (int i = 0; i < 5; i++) {
        //coral1.position = allPositions[122 + i];
        models.push_back(coral2);
    }



   // models.push_back(loadModel("Models/82_vray_and_corona_2014.obj", "Models/arena.jpg", shaderProgram, 0));

    models.push_back(loadModel("Models/base.obj", "Models/arena.jpg",false, shaderProgram, 0));

    models.push_back(loadModel("Models/table2.obj", "Models/WoodSeemles1.jpg", false, shaderProgram, 0));

    models.push_back(loadModel("Models/vertical_square.obj", "Models/pared.jpg", false, shaderProgram, 0));
    models.push_back(loadModel("Models/vertical_square.obj", "Models/pared.jpg", false, shaderProgram, 0));
    models.push_back(loadModel("Models/vertical_square2.obj", "Models/pared.jpg", false, shaderProgram, 0));
    models.push_back(loadModel("Models/vertical_square2.obj", "Models/pared.jpg", false, shaderProgram, 0));


    models.push_back(loadModel("Models/piso.obj", "Models/piso.jpg", false ,shaderProgram, 0));
    models.push_back(loadModel("Models/piso.obj", "Models/techo.jpeg", false, shaderProgram, 0));

    models.push_back(loadModel("Models/superficie2.obj", "Models/celeste.png", true,shaderProgram, 0));


    models.push_back(loadModel("Models/finalcube.obj", "Models/azul.png", true, shaderProgram, 0));



    // Shader for light cube
    Shader lightShader("light.vert", "light.frag");
    // Generates Vertex Array Object and binds it
    VAO lightVAO;
    lightVAO.Bind();
    // Generates Vertex Buffer Object and links it to vertices
    VBO lightVBO(lightVertices, sizeof(lightVertices));
    // Generates Element Buffer Object and links it to indices
    EBO lightEBO(lightIndices, sizeof(lightIndices));
    // Links VBO attributes such as coordinates and colors to VAO
    lightVAO.LinkAttrib(lightVBO, 0, 3, GL_FLOAT, 3 * sizeof(float), (void*)0);
    // Unbind all to prevent accidentally modifying them
    lightVAO.Unbind();
    lightVBO.Unbind();
    lightEBO.Unbind();



    glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    glm::vec3 lightPos = glm::vec3(0.0f, 5.0f, 0.0f);
    glm::mat4 lightModel = glm::mat4(1.0f);
    lightModel = glm::translate(lightModel, lightPos);

    glm::vec3 pyramidPos = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::mat4 pyramidModel = glm::mat4(1.0f);
    pyramidModel = glm::translate(pyramidModel, pyramidPos);


    lightShader.Activate();
    glUniformMatrix4fv(glGetUniformLocation(lightShader.ID, "model"), 1, GL_FALSE, glm::value_ptr(lightModel));
    glUniform4f(glGetUniformLocation(lightShader.ID, "lightColor"), lightColor.x, lightColor.y, lightColor.z, lightColor.w);
    shaderProgram.Activate();
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram.ID, "model"), 1, GL_FALSE, glm::value_ptr(pyramidModel));
    glUniform4f(glGetUniformLocation(shaderProgram.ID, "lightColor"), lightColor.x, lightColor.y, lightColor.z, lightColor.w);
    glUniform3f(glGetUniformLocation(shaderProgram.ID, "lightPos"), lightPos.x, lightPos.y, lightPos.z);





    // Enables the Depth Buffer
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);



    // Creates camera object
    Camera camera(width, height, glm::vec3(0.0f, 40.0f, 70.0f));


    // Main while loop
    while (!glfwWindowShouldClose(window))
    {
        // Specify the color of the background
        glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
        // Clean the back buffer and depth buffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


        // Handles camera inputs
        camera.Inputs(window);
        // Updates and exports the camera matrix to the Vertex Shader
        camera.updateMatrix(45.0f, 0.2f, 1000.0f);



        // Tells OpenGL which Shader Program we want to use
        lightShader.Activate();
        // Export the camMatrix to the Vertex Shader of the light cube
        camera.Matrix(lightShader, "camMatrix");
        // Bind the VAO so OpenGL knows to use it
        lightVAO.Bind();
        // Draw primitives, number of indices, datatype of indices, index of indices
        glDrawElements(GL_TRIANGLES, sizeof(lightIndices) / sizeof(int), GL_UNSIGNED_INT, 0);

        // Tells OpenGL which Shader Program we want to use
        shaderProgram.Activate();
        // Exports the camera Position to the Fragment Shader for specular lighting
        glUniform3f(glGetUniformLocation(shaderProgram.ID, "camPos"), camera.Position.x, camera.Position.y, camera.Position.z);
        // Export the camMatrix to the Vertex Shader of the pyramid
        camera.Matrix(shaderProgram, "camMatrix");
        // Binds texture so that is appears in rendering



        // Variables para la rotación
        float angleV2 = glfwGetTime(); // Usa el tiempo actual para animar la rotación
        glm::mat4 rotationMat = glm::rotate(glm::mat4(1.0f), angleV2, glm::vec3(0.0f, 1.0f, 0.0f));
        // Ajusta la distancia máxima permitida para el movimiento circular/elíptico
        float maxDistance = 1000.0f;
        float maxDistanceSquared = maxDistance * maxDistance;
		
		

        int i = 1;
        for (auto& model:models){
            glm::mat4 modelMat = glm::mat4(1.0f);

            modelMat = glm::scale(modelMat, glm::vec3(0.01f)); // Escalar el modelo al 10% de su tamaño original
            modelMat = glm::translate(modelMat, allPositions[i]);

            if (model.ModelName != "Models/finalcube.obj" || model.ModelName != "Models/superficie2.obj") {
                glDisable(GL_BLEND);
            }

            if (model.ModelName == "Models/coral_v1.obj") { // El segundo coral es el 18º modelo, índice 17
                modelMat = glm::rotate(modelMat, glm::radians(270.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            }


            if (model.ModelName == "Models/cube.obj") { // El segundo coral es el 18º modelo, índice 17
                std::cout << i << std::endl;
            }



            if (i < 7) {
                float angleTest = atan2(allPositions[i][0], allPositions[i][2]) * 180 /M_PI;
                modelMat = glm::rotate(modelMat, glm::radians(angleTest), glm::vec3(0.0f, 1.0f, 0.0f));
                // Calcula la distancia al centro

                float distanceToCenter = sqrt(allPositions[i][0] * allPositions[i][0] + allPositions[i][2] * allPositions[i][2]);
                float rotationSpeed = 1000 / distanceToCenter;


                if (allPositions[i][0] > 0.0f) {
                    modelMat = glm::rotate(modelMat, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                    rotationMat = glm::rotate(glm::mat4(1.0f), angleV2 * rotationSpeed, glm::vec3(0.0f, 1.0f, 0.0f));
                    modelMat = rotationMat * modelMat;
                } else {
                    modelMat = glm::rotate(modelMat, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                    rotationMat = glm::rotate(glm::mat4(1.0f), -angleV2 * rotationSpeed, glm::vec3(0.0f, 1.0f, 0.0f));
                    modelMat = rotationMat * modelMat;
                }

                // Supongamos que la posición de la cabeza está en headOffset respecto al centro del modelo
                glm::vec3 headOffset = glm::vec3(0.0f, 0.0f, -300.0f); // ejemplo: 1.8 unidades arriba del centro del cuerpo

                // Desplazar el modelo de manera que la cabeza esté en el origen
                modelMat = glm::translate(modelMat, -headOffset);

                // Aplicar la rotación alrededor del eje Y desde la posición de la cabeza
                float zOffset = 100.0f * sin(glfwGetTime() * 20.0f);
                float tiltAngleZ = glm::radians(zOffset / 200.0f * 15.0f); // 15 grados es el ángulo máximo de giro en Z
                modelMat = glm::rotate(modelMat, tiltAngleZ, glm::vec3(0.0f, 1.0f, 0.0f));

                // Desplazar el modelo de regreso a su posición original
                modelMat = glm::translate(modelMat, headOffset);



            }



            // Aplicar rotación alrededor del origen (0,0,0) para los peces
            if (i > 6 && i < 10) {
                // Cálculo del ángulo y la velocidad de rotación
                float angleTest = atan2(allPositions[i][0], allPositions[i][2]) * 180 /M_PI;
                modelMat = glm::rotate(modelMat, glm::radians(angleTest), glm::vec3(0.0f, 1.0f, 0.0f));
                float distanceToCenter = sqrt(allPositions[i][0] * allPositions[i][0] + allPositions[i][2] * allPositions[i][2]);
                float rotationSpeed = 1000 / distanceToCenter;

                // Aplicar la rotación horizontal
                if (allPositions[i][0] > 0.0f) {
                    modelMat = glm::rotate(modelMat, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                    rotationMat = glm::rotate(glm::mat4(1.0f), angleV2 * rotationSpeed, glm::vec3(0.0f, 1.0f, 0.0f));
                    modelMat = rotationMat * modelMat;
                } else {
                    modelMat = glm::rotate(modelMat, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                    rotationMat = glm::rotate(glm::mat4(1.0f), -angleV2 * rotationSpeed, glm::vec3(0.0f, 1.0f, 0.0f));
                    modelMat = rotationMat * modelMat;
                }

                // Aplicar movimiento sinusoidal para la coordenada Y
                float yOffset = 250.0f * sin(glfwGetTime() * 5.0f); // 5.0f es la frecuencia de oscilación
                float zOffset = 100.0f * sin(glfwGetTime() * 20.0f);


                // Añadir rotación en el eje X para inclinar el pez hacia arriba o abajo
                float tiltAngleX = glm::radians(yOffset / 200.0f * 20.0f); // 45 grados es el ángulo máximo de inclinación

                // Añadir rotación en el eje Z para girar el pez hacia arriba o abajo
                float tiltAngleZ = glm::radians(zOffset / 200.0f * 15.0f); // 15 grados es el ángulo máximo de giro en Z

                // Aplicar las rotaciones centradas
                modelMat = glm::translate(modelMat, glm::vec3(0.0f, yOffset, 0.0f));
                modelMat = glm::rotate(modelMat, tiltAngleX, glm::vec3(1.0f, 0.0f, 0.0f));
                modelMat = glm::rotate(modelMat, tiltAngleZ, glm::vec3(0.0f, 1.0f, 0.0f));

            }





            if (i > 9 && i < 14) {
                // Cálculo del ángulo y la velocidad de rotación
                float angleTest = atan2(allPositions[i][0], allPositions[i][2]) * 180 /M_PI;
                modelMat = glm::rotate(modelMat, glm::radians(angleTest), glm::vec3(0.0f, 1.0f, 0.0f));
                float distanceToCenter = sqrt(allPositions[i][0] * allPositions[i][0] + allPositions[i][2] * allPositions[i][2]);
                float rotationSpeed = 1000 / distanceToCenter;

                // Aplicar la rotación horizontal
                if (allPositions[i][0] > 0.0f) {
                    modelMat = glm::rotate(modelMat, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                    rotationMat = glm::rotate(glm::mat4(1.0f), angleV2 * rotationSpeed, glm::vec3(0.0f, 1.0f, 0.0f));
                    modelMat = rotationMat * modelMat;
                } else {
                    modelMat = glm::rotate(modelMat, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                    rotationMat = glm::rotate(glm::mat4(1.0f), -angleV2 * rotationSpeed, glm::vec3(0.0f, 1.0f, 0.0f));
                    modelMat = rotationMat * modelMat;
                }

                // Calcular la posición en forma de ocho
                float x = 500.0f * sin(2.0f * glfwGetTime()); // 2.0f es la frecuencia en x
                float y = 0.0f;
                float z = 250.0f * cos(4.0f * glfwGetTime()); // 4.0f es la frecuencia en y

                glm::vec3 newPosition = glm::vec3(x, y, z);
                modelMat = glm::translate(modelMat, newPosition);

                // Supongamos que la posición de la cabeza está en headOffset respecto al centro del modelo
                glm::vec3 headOffset = glm::vec3(0.0f, 0.0f, -300.0f); // ejemplo: 1.8 unidades arriba del centro del cuerpo

                // Desplazar el modelo de manera que la cabeza esté en el origen
                modelMat = glm::translate(modelMat, -headOffset);

                // Aplicar la rotación alrededor del eje Y desde la posición de la cabeza
                float zOffset = 100.0f * sin(glfwGetTime() * 20.0f);
                float tiltAngleZ = glm::radians(zOffset / 200.0f * 15.0f); // 15 grados es el ángulo máximo de giro en Z
                modelMat = glm::rotate(modelMat, tiltAngleZ, glm::vec3(0.0f, 1.0f, 0.0f));

                // Desplazar el modelo de regreso a su posición original
                modelMat = glm::translate(modelMat, headOffset);

            }





            if (model.ModelName == "Models/finalcube.obj" || model.ModelName == "Models/superficie2.obj") {
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            }


            float currentTime = glfwGetTime();
            if (model.ModelName == "Models/superficie2.obj") {

                updateWaveModel(model, currentTime);

            }









            model.texture.Bind();
            // Bind the VAO so OpenGL knows to use it

            model.vao.Bind();

            // Draw primitives, number of indices, datatype of indices, index of indices
            glDrawElements(GL_TRIANGLES, model.indices.size(), GL_UNSIGNED_INT, 0);
            i++;
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram.ID, "model"), 1, GL_FALSE, glm::value_ptr(modelMat));
            glUniform4f(glGetUniformLocation(shaderProgram.ID, "lightColor"), lightColor.x, lightColor.y, lightColor.z, lightColor.w);
            glUniform3f(glGetUniformLocation(shaderProgram.ID, "lightPos"), lightPos.x, lightPos.y, lightPos.z);

        }






        glBindVertexArray(0);
        // Swap the back buffer with the front buffer
        glfwSwapBuffers(window);
        // Take care of all GLFW events
        glfwPollEvents();
    }

    // Delete window before ending the program
    glfwDestroyWindow(window);
    // Terminate GLFW before ending the program
    glfwTerminate();
    return 0;
}

int con = 0;

Model loadModel(const std::string& objFilePath, const std::string& texturePath, bool isPNG ,Shader& shaderProgram, GLuint i) {
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;

    if (!loadObj(objFilePath, vertices, indices)) {
        std::cerr << "Error loading OBJ file: " << objFilePath << std::endl;
    }

    if (isPNG) {
        Texture Tex(texturePath.c_str(), GL_TEXTURE_2D, GL_TEXTURE0, GL_RGBA, GL_UNSIGNED_BYTE);
        Tex.texUnit(shaderProgram, "tex0", i);
        Model model(vertices, indices, objFilePath, Tex);
        return model;
    } else {
        Texture Tex(texturePath.c_str(), GL_TEXTURE_2D, GL_TEXTURE0, GL_RGB, GL_UNSIGNED_BYTE);
        Tex.texUnit(shaderProgram, "tex0", i);
        Model model(vertices, indices, objFilePath, Tex);
        return model;
    }




    con++;


}


