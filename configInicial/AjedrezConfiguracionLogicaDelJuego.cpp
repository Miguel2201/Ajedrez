#include <iostream>
#include <cmath>

// GLEW
#include <GL/glew.h>

// GLFW
#include <GLFW/glfw3.h>

// Other Libs
#include "stb_image.h"

// GLM Mathematics
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

//Load Models
#include "SOIL2/SOIL2.h"


// Other includes
#include "Shader.h"
#include "Camera.h"
#include "Model.h"

//Estructura de Piezas
// --- Añadido para Lógica de Ajedrez ---

#include <vector> // Necesario para std::vector
#include <map>   // Necesario para std::map (opcional pero útil)

//Para los movimientos del ajedrez
#include <cmath> // Necesario para std::abs

// Tipos de Pieza
enum PieceType { EMPTY, PAWN, ROOK, KNIGHT, BISHOP, QUEEN, KING };
// Colores de Pieza
enum PieceColor { NONE, WHITE, BLACK };

// Estructura para representar una Pieza de Ajedrez
struct ChessPiece {
	PieceType type = EMPTY;
	PieceColor color = NONE;
	Model* model = nullptr; // Puntero al modelo 3D asociado
	int row = -1;           // Fila actual en el tablero (0-7)
	int col = -1;           // Columna actual en el tablero (0-7)
	glm::vec3 positionOffset = glm::vec3(0.0f); // Ajuste fino de posición si es necesario
	glm::vec3 scale = glm::vec3(1.0f);         // Escala específica de la pieza
	float rotationY = 1.0f; // Rotación específica si es necesaria
	bool isSelected = false; // Nuevo campo para indicar si está seleccionada
};

// Representación del tablero (8x8)
ChessPiece board[8][8];

// Variables para manejar la selección y movimiento
ChessPiece* selectedPiece = nullptr; // Puntero a la pieza seleccionada
int selectedRow = -1;
int selectedCol = -1;
PieceColor currentPlayer = WHITE; // Empieza el blanco

// Dimensiones y posición del tablero en el mundo 3D (¡AJUSTA ESTOS VALORES!)
// Necesitas saber dónde está la esquina (0,0) de tu tablero y el tamaño de cada casilla
const float TILE_SIZE = 5.0f;   // Ancho/Profundidad de una casilla (Ajusta según tu modelo 'Piso') 4.75
const float BOARD_OFFSET_X = -20.25f; // Posición X de la esquina A1 (col 0)
const float BOARD_OFFSET_Z = 1.0f;    // Posición Z de la esquina A1 (row 0)
const float PIECE_Y_OFFSET = -2.0f;   // Altura Y base donde se colocan las piezas

// --- Fin de Añadido para Lógica de Ajedrez ---

// Function prototypes
void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mode);
void MouseCallback(GLFWwindow *window, double xPos, double yPos);
void DoMovement();
void InitializeBoard(
	Model* pPeonW, Model* pTorreW, Model* pCaballoW, Model* pAlfilW, Model* pReinaW, Model* pReyW,
	Model* pPeonB, Model* pTorreB, Model* pCaballoB, Model* pAlfilB, Model* pReinaB, Model* pReyB
);
glm::vec3 GetWorldCoordinates(int row, int col);
bool IsPathClear(int startRow, int startCol, int endRow, int endCol);
bool IsValidMove(ChessPiece* piece, int targetRow, int targetCol);
bool WorldToBoardCoordinates(const glm::vec3& worldPos, int& row, int& col); // Asumo que esta también existe
glm::vec3 CalculateMouseRay(GLFWwindow* window, double xpos, double ypos, const Camera& cam, const glm::mat4& projectionMatrix);
float RayPlaneIntersection(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, const glm::vec3& planePoint, const glm::vec3& planeNormal);
glm::vec3 GetBoardIntersectionPoint(GLFWwindow* window, double xpos, double ypos, const Camera& cam);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods, Shader& lightingShader);

// Window dimensions
const GLuint WIDTH = 1200, HEIGHT = 1000;
int SCREEN_WIDTH, SCREEN_HEIGHT;

// Camera
Camera  camera(glm::vec3(0.0f, 0.0f, 15.0f));
GLfloat lastX = WIDTH / 2.0;
GLfloat lastY = HEIGHT / 2.0;
bool keys[1024];
bool firstMouse = true;
// Light attributes
glm::vec3 lightPos(0.0f, 0.0f, 0.0f);
bool active;

// Positions of the point lights  //**arrglo de de las luces en el origen
glm::vec3 pointLightPositions[] = {
	glm::vec3(-2.0f,0.0f, -2.0f),
	glm::vec3(2.0f,0.0f, 2.0f),
	glm::vec3(2.0f,0.0f,  -2.0f),
	glm::vec3(-2.0f,0.0f, 2.0f)
};

float vertices[] = {
	 -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
	   -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
	   -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

	   -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
	   -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
	   -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,

	   -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
	   -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
	   -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
	   -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
	   -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
	   -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

		0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
		0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
		0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
		0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
		0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
		0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

	   -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
		0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
		0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
		0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
	   -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
	   -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

	   -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
		0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
	   -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
	   -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
};



glm::vec3 Light1 = glm::vec3(0);


// Deltatime
GLfloat deltaTime = 0.0f;	// Time between current frame and last frame
GLfloat lastFrame = 0.0f;  	// Time of last frame

int main()
{
	// Init GLFW
	glfwInit();
	// Set all the required options for GLFW
	/*glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);*/

	// Create a GLFWwindow object that we can use for GLFW's functions
	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Fuentes de luz", nullptr, nullptr);

	if (nullptr == window)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();

		return EXIT_FAILURE;
	}

	glfwMakeContextCurrent(window);

	glfwGetFramebufferSize(window, &SCREEN_WIDTH, &SCREEN_HEIGHT);

	// Set the required callback functions
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetCursorPosCallback(window, MouseCallback);

	// GLFW Options
	//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Set this to true so GLEW knows to use a modern approach to retrieving function pointers and extensions
	glewExperimental = GL_TRUE;
	// Initialize GLEW to setup the OpenGL Function pointers
	if (GLEW_OK != glewInit())
	{
		std::cout << "Failed to initialize GLEW" << std::endl;
		return EXIT_FAILURE;
	}

	// Define the viewport dimensions
	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);



	Shader lightingShader("Shader/lighting.vs", "Shader/lighting.frag");  //**shaders
	Shader lampShader("Shader/lamp.vs", "Shader/lamp.frag");
	glfwSetWindowUserPointer(window, &lightingShader);
	
	//Carga de modelos
	Model Piso((char*)"Models/Minecraft/Tablero2.obj");
	//Overworld
	Model steve((char*)"Models/Minecraft/steve.obj");
	Model alex((char*)"Models/Minecraft/alex.obj");
	Model golem((char*)"Models/Minecraft/golem.obj");
	Model caballo((char*)"Models/Minecraft/caballo.obj");
	Model perro((char*)"Models/Minecraft/perro.obj");
	Model pollo((char*)"Models/Minecraft/pollo.obj");

	//Nether
	Model warden((char*)"Models/Minecraft/warden.obj");
	Model dragon((char*)"Models/Minecraft/dragon.obj");
	Model piglin((char*)"Models/Minecraft/piglin.obj");
	Model blaze((char*)"Models/Minecraft/blaze.obj");
	Model enderman((char*)"Models/Minecraft/enderman.obj");
	Model esqueleto((char*)"Models/Minecraft/esqueleto.obj");

	// --- Llamando a la funcion para inicializar el tablero
	InitializeBoard(
		&pollo, &golem, &caballo, &perro, &alex, &steve, // Modelos Blancos (Peón a Rey)
		&esqueleto, &piglin, &blaze, &enderman, &dragon, &warden // Modelos Negros (Peón a Rey)
	);

	// First, set the container's VAO (and VBO)
	GLuint VBO, VAO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	// Position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);
	// normal attribute
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	// Set texture units
	lightingShader.Use();
	glUniform1i(glGetUniformLocation(lightingShader.Program, "Material.difuse"), 0);
	glUniform1i(glGetUniformLocation(lightingShader.Program, "Material.specular"), 1);

	glm::mat4 projection = glm::perspective(camera.GetZoom(), (GLfloat)SCREEN_WIDTH / (GLfloat)SCREEN_HEIGHT, 0.1f, 100.0f);

	// --- Añadido: Registrar callback del botón del mouse ---
	void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods); // Declaración adelantada
	glfwSetMouseButtonCallback(window, MouseButtonCallback);

	// --- Fin de Añadido ---

	// Game loop
	while (!glfwWindowShouldClose(window))
	{

		// Calculate deltatime of current frame
		GLfloat currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// Check if any events have been activiated (key pressed, mouse moved etc.) and call corresponding response functions
		glfwPollEvents();
		DoMovement();

		// Clear the colorbuffer
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	   
		// OpenGL options
		glEnable(GL_DEPTH_TEST);

		
		
		//Load Model
	

		// Use cooresponding shader when setting uniforms/drawing objects
		lightingShader.Use();

        glUniform1i(glGetUniformLocation(lightingShader.Program, "diffuse"), 0);
		//glUniform1i(glGetUniformLocation(lightingShader.Program, "specular"),1);

		GLint viewPosLoc = glGetUniformLocation(lightingShader.Program, "viewPos");
		glUniform3f(viewPosLoc, camera.GetPosition().x, camera.GetPosition().y, camera.GetPosition().z);


		// Directional light  //**direccion 
		glUniform3f(glGetUniformLocation(lightingShader.Program, "dirLight.direction"), -0.2f, -1.0f, -0.3f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "dirLight.ambient"),0.8f,0.8f,0.8f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "dirLight.diffuse"), 0.5f, 0.5f, 0.5f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "dirLight.specular"),0.0f, 0.0f, 0.0f);


		// Point light 1
	    glm::vec3 lightColor;
		lightColor.x= abs(sin(glfwGetTime() *Light1.x));
		lightColor.y= abs(sin(glfwGetTime() *Light1.y));
		lightColor.z= sin(glfwGetTime() *Light1.z);

		
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[0].position"), pointLightPositions[0].x, pointLightPositions[0].y, pointLightPositions[0].z);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[0].ambient"), lightColor.x,lightColor.y, lightColor.z);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[0].diffuse"), lightColor.x,lightColor.y,lightColor.z);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[0].specular"), 0.0f, 0.0f, 0.0f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[0].constant"), 1.0f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[0].linear"), 0.045f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[0].quadratic"),0.075f);



		// Point light 2
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[1].position"), pointLightPositions[1].x, pointLightPositions[1].y, pointLightPositions[1].z);
		
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[1].ambient"), 0.05f, 0.05f, 0.05f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[1].diffuse"), 0.0f, 0.05f, 0.0f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[1].specular"), 0.0f, 0.0f, 0.0f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[1].constant"), 1.0f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[1].linear"), 0.0f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[1].quadratic"), 0.0f);

		// Point light 3
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[2].position"), pointLightPositions[2].x, pointLightPositions[2].y, pointLightPositions[2].z);

		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[2].ambient"), 0.0f, 0.0f, 0.0f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[2].diffuse"), 0.0f, 0.0f, 0.0f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[2].specular"), 0.0f, 0.0f, 0.0f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[2].constant"), 1.0f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[2].linear"), 0.0f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[2].quadratic"), 0.0f);

		// Point light 4
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[3].position"), pointLightPositions[3].x, pointLightPositions[3].y, pointLightPositions[3].z);

		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[3].ambient"), 0.10f, 0.0f, 0.10f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[3].diffuse"), 0.10f, 0.0f, 0.10f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[3].specular"), 0.0f, 0.0f, 0.0f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[3].constant"), 1.0f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[3].linear"), 0.0f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[3].quadratic"), 0.0f);

		// SpotLight  //**se mueve por la camara 
		glUniform3f(glGetUniformLocation(lightingShader.Program, "spotLight.position"), camera.GetPosition().x, camera.GetPosition().y, camera.GetPosition().z);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "spotLight.direction"), camera.GetFront().x, camera.GetFront().y, camera.GetFront().z);
		
		glUniform3f(glGetUniformLocation(lightingShader.Program, "spotLight.ambient"), 0.0f, 0.0f, 0.0f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "spotLight.diffuse"), 0.0f, 0.0f, 0.0f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "spotLight.specular"),0.0f, 0.0f, 0.0f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "spotLight.constant"), 1.0f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "spotLight.linear"), 0.0f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "spotLight.quadratic"), 0.0f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "spotLight.cutOff"), glm::cos(glm::radians(12.0f)));
		glUniform1f(glGetUniformLocation(lightingShader.Program, "spotLight.outerCutOff"), glm::cos(glm::radians(12.0f)));

		// Set material properties
		glUniform1f(glGetUniformLocation(lightingShader.Program, "material.shininess"), 16.0f);

		// Create camera transformations
		glm::mat4 view;
		view = camera.GetViewMatrix();

		// Get the uniform locations
		GLint modelLoc = glGetUniformLocation(lightingShader.Program, "model");
		GLint viewLoc = glGetUniformLocation(lightingShader.Program, "view");
		GLint projLoc = glGetUniformLocation(lightingShader.Program, "projection");

		// Pass the matrices to the shader
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));


		// --- REEMPLAZADO: Dibujar el tablero y las piezas desde la estructura 'board' ---
		// Dibujar el tablero (Piso)
		glm::mat4 model = glm::mat4(1.0f);
		// Si tu tablero no está en el origen, aplica la misma transformación que antes
	   // model = glm::translate(model, glm::vec3(offsetX, offsetY, offsetZ));
		model = glm::scale(model, glm::vec3(82.0f, 1.0f, 84.0f)); // Ajusta la escala si es necesario
		model = glm::translate(model, glm::vec3(0.0f,-3.75f,0.25f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
		Piso.Draw(lightingShader);

		
		// Dibujar las piezas
		for (int r = 0; r < 8; ++r) {
			for (int c = 0; c < 8; ++c) {
				ChessPiece& piece = board[r][c];
				if (piece.type != EMPTY && piece.model != nullptr) {
					glm::mat4 model = glm::mat4(1.0f);
					glm::vec3 worldPos = GetWorldCoordinates(r, c);
					worldPos += piece.positionOffset;

					model = glm::translate(model, worldPos);
					if (piece.rotationY != 0.0f) {
						model = glm::rotate(model, piece.rotationY, glm::vec3(0.0f, 1.0f, 0.0f));
					}
					model = glm::scale(model, piece.scale);

					// Aplicar resaltado si la pieza está seleccionada
					if (piece.isSelected) {
						glm::vec3 highlightColor = glm::vec3(1.0f, 1.0f, 0.0f); // Amarillo para resaltar
						glUniform3f(glGetUniformLocation(lightingShader.Program, "highlightColor"), highlightColor.x, highlightColor.y, highlightColor.z);
					}
					else {
						glUniform3f(glGetUniformLocation(lightingShader.Program, "highlightColor"), 0.0f, 0.0f, 0.0f);
					}

					glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
					piece.model->Draw(lightingShader);
				}
			}
		}


		// Also draw the lamp object, again binding the appropriate shader
		lampShader.Use();
		// Get location objects for the matrices on the lamp shader (these could be different on a different shader)
		modelLoc = glGetUniformLocation(lampShader.Program, "model");
		viewLoc = glGetUniformLocation(lampShader.Program, "view");
		projLoc = glGetUniformLocation(lampShader.Program, "projection");

		// Set matrices
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
		model = glm::mat4(1);
		model = glm::translate(model, lightPos);
		model = glm::scale(model, glm::vec3(0.2f)); // Make it a smaller cube
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
		// Draw the light object (using light's vertex attributes)
		for (GLuint i = 0; i < 4; i++)
		{
			model = glm::mat4(1);
			model = glm::translate(model, pointLightPositions[i]);
			model = glm::scale(model, glm::vec3(0.2f)); // Make it a smaller cube
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
			glBindVertexArray(VAO);
			glDrawArrays(GL_TRIANGLES, 0, 36);
		}
		glBindVertexArray(0);



		// Swap the screen buffers
		glfwSwapBuffers(window);
	}


	// Terminate GLFW, clearing any resources allocated by GLFW.
	glfwTerminate();



	return 0;
	

}

// Moves/alters the camera positions based on user input
void DoMovement()
{

	// Camera controls
	if (keys[GLFW_KEY_W] || keys[GLFW_KEY_UP])
	{
		camera.ProcessKeyboard(FORWARD, deltaTime);

	}

	if (keys[GLFW_KEY_S] || keys[GLFW_KEY_DOWN])
	{
		camera.ProcessKeyboard(BACKWARD, deltaTime);


	}

	if (keys[GLFW_KEY_A] || keys[GLFW_KEY_LEFT])
	{
		camera.ProcessKeyboard(LEFT, deltaTime);


	}

	if (keys[GLFW_KEY_D] || keys[GLFW_KEY_RIGHT])
	{
		camera.ProcessKeyboard(RIGHT, deltaTime);


	}

	if (keys[GLFW_KEY_T])
	{
		pointLightPositions[0].x += 0.01f;
	}
	if (keys[GLFW_KEY_G])
	{
		pointLightPositions[0].x -= 0.01f;
	}

	if (keys[GLFW_KEY_Y])
	{
		pointLightPositions[0].y += 0.01f;
	}

	if (keys[GLFW_KEY_H])
	{
		pointLightPositions[0].y -= 0.01f;
	}
	if (keys[GLFW_KEY_U])
	{
		pointLightPositions[0].z -= 0.1f;
	}
	if (keys[GLFW_KEY_J])
	{
		pointLightPositions[0].z += 0.01f;
	}
	
}

// Is called whenever a key is pressed/released via GLFW
void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
	if (GLFW_KEY_ESCAPE == key && GLFW_PRESS == action)
	{
		glfwSetWindowShouldClose(window, GL_TRUE);
	}

	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
		{
			keys[key] = true;
		}
		else if (action == GLFW_RELEASE)
		{
			keys[key] = false;
		}
	}

	if (keys[GLFW_KEY_SPACE])
	{
		active = !active;
		if (active)
		{
			Light1 = glm::vec3(1.0f, 1.0f, 0.0f);
		}
		else
		{
			Light1 = glm::vec3(0);//Cuado es solo un valor en los 3 vectores pueden dejar solo una componente
		}
	}
}

void MouseCallback(GLFWwindow *window, double xPos, double yPos)
{
	if (firstMouse)
	{
		lastX = xPos;
		lastY = yPos;
		firstMouse = false;
	}

	GLfloat xOffset = xPos - lastX;
	GLfloat yOffset = lastY - yPos;  // Reversed since y-coordinates go from bottom to left

	lastX = xPos;
	lastY = yPos;

	camera.ProcessMouseMovement(xOffset, yOffset);
}


// --- Añadido: Función auxiliar para verificar caminos ---
// Verifica si el camino entre (startRow, startCol) y (endRow, endCol) está vacío.
// ¡NO verifica la validez del movimiento en sí (horizontal, vertical, diagonal)!
// Asume que el movimiento es en línea recta (horizontal, vertical o diagonal perfecta).
// No chequea la casilla final (endRow, endCol), solo las intermedias.
bool IsPathClear(int startRow, int startCol, int endRow, int endCol) {
	// Determinar la dirección del movimiento (paso en x, paso en y)
	int stepY = (endRow > startRow) ? 1 : ((endRow < startRow) ? -1 : 0);
	int stepX = (endCol > startCol) ? 1 : ((endCol < startCol) ? -1 : 0);

	// Casilla actual empieza una después del inicio
	int currentRow = startRow + stepY;
	int currentCol = startCol + stepX;

	// Recorrer el camino hasta llegar a la casilla final
	while (currentRow != endRow || currentCol != endCol) {
		// Si alguna casilla intermedia no está vacía, el camino está bloqueado
		if (currentRow < 0 || currentRow >= 8 || currentCol < 0 || currentCol >= 8) {
			// Seguridad: Si por algún error el cálculo sale del tablero
			std::cerr << "Error en IsPathClear: fuera de límites (" << currentRow << "," << currentCol << ")" << std::endl;
			return false;
		}
		if (board[currentRow][currentCol].type != EMPTY) {
			return false; // Camino bloqueado
		}
		// Avanzar a la siguiente casilla en el camino
		currentRow += stepY;
		currentCol += stepX;
	}

	// Si el bucle termina, significa que todas las casillas intermedias estaban vacías
	return true; // Camino despejado
}
// --- Fin de Añadido ---

// --- ACTUALIZADA: Validación de Movimientos de Ajedrez (Reglas Básicas) ---
bool IsValidMove(ChessPiece* piece, int targetRow, int targetCol) {
	// 1. Chequeos Iniciales Básicos
	if (!piece || piece->type == EMPTY) {
		std::cerr << "Error IsValidMove: Pieza inválida o vacía." << std::endl;
		return false;
	}
	if (targetRow < 0 || targetRow >= 8 || targetCol < 0 || targetCol >= 8) {
		// std::cout << "Movimiento inválido: Fuera del tablero." << std::endl; // Mucho spam
		return false; // Fuera del tablero
	}

	ChessPiece& targetSquare = board[targetRow][targetCol];
	int startRow = piece->row;
	int startCol = piece->col;

	// No puedes mover a la misma casilla
	if (startRow == targetRow && startCol == targetCol) {
		return false;
	}

	// No puedes capturar una pieza de tu propio color
	if (targetSquare.type != EMPTY && targetSquare.color == piece->color) {
		// std::cout << "Movimiento inválido: No puedes capturar tu propia pieza." << std::endl;
		return false;
	}

	// 2. Lógica Específica por Tipo de Pieza
	switch (piece->type) {
	case PAWN: {
		int forward = (piece->color == WHITE) ? 1 : -1; // Dirección de avance
		// Mover 1 casilla adelante
		if (targetCol == startCol && targetRow == startRow + forward && targetSquare.type == EMPTY) {
			return true;
		}
		// Mover 2 casillas adelante (solo desde posición inicial)
		bool isStartingRow = (piece->color == WHITE && startRow == 1) || (piece->color == BLACK && startRow == 6);
		if (isStartingRow && targetCol == startCol && targetRow == startRow + 2 * forward && targetSquare.type == EMPTY) {
			// Verificar que la casilla intermedia también esté vacía
			if (board[startRow + forward][startCol].type == EMPTY) {
				return true;
			}
		}
		// Captura diagonal
		if (std::abs(targetCol - startCol) == 1 && targetRow == startRow + forward && targetSquare.type != EMPTY && targetSquare.color != piece->color) {
			return true;
		}
		// Faltan: En Passant, Promoción
		return false; // Si no es ninguno de los anteriores, es inválido para el peón
	}

	case ROOK: {
		// Debe ser movimiento horizontal o vertical
		if (startRow != targetRow && startCol != targetCol) {
			return false; // No es movimiento de torre
		}
		// Verificar que el camino esté despejado
		return IsPathClear(startRow, startCol, targetRow, targetCol);
	}

	case KNIGHT: {
		int dRow = std::abs(targetRow - startRow);
		int dCol = std::abs(targetCol - startCol);
		// Movimiento en 'L' (2 en una dirección, 1 en la perpendicular)
		return (dRow == 2 && dCol == 1) || (dRow == 1 && dCol == 2);
		// El caballo salta, no necesita IsPathClear
	}

	case BISHOP: {
		// Debe ser movimiento diagonal
		if (std::abs(targetRow - startRow) != std::abs(targetCol - startCol)) {
			return false; // No es movimiento de alfil
		}
		// Verificar que el camino esté despejado
		return IsPathClear(startRow, startCol, targetRow, targetCol);
	}

	case QUEEN: {
		// Debe ser movimiento horizontal, vertical o diagonal
		bool isStraight = (startRow == targetRow || startCol == targetCol);
		bool isDiagonal = (std::abs(targetRow - startRow) == std::abs(targetCol - startCol));
		if (!isStraight && !isDiagonal) {
			return false; // No es movimiento de reina
		}
		// Verificar que el camino esté despejado
		return IsPathClear(startRow, startCol, targetRow, targetCol);
	}

	case KING: {
		int dRow = std::abs(targetRow - startRow);
		int dCol = std::abs(targetCol - startCol);
		// Mover solo 1 casilla en cualquier dirección
		// Faltan: Enroque, Chequeo de Jaque
		return dRow <= 1 && dCol <= 1;
	}

	case EMPTY:
	default:
		std::cerr << "Error IsValidMove: Tipo de pieza desconocido o EMPTY." << std::endl;
		return false;
	}
}
// --- Fin de ACTUALIZADA ---


// Función para intentar convertir coordenadas del mundo a tablero (¡SIMPLIFICADA!)
// Asume que el eje Y es la altura y que el tablero está en el plano XZ
bool WorldToBoardCoordinates(const glm::vec3& worldPos, int& row, int& col) {
	// Calcula columna basada en X
	float boardX = worldPos.x - BOARD_OFFSET_X;
	col = static_cast<int>(floor(boardX / TILE_SIZE));

	// Calcula fila basada en Z
	float boardZ = worldPos.z - BOARD_OFFSET_Z;
	row = static_cast<int>(floor(boardZ / TILE_SIZE));

	// Verifica si está dentro de los límites del tablero
	if (row >= 0 && row < 8 && col >= 0 && col < 8) {
		return true;
	}
	else {
		row = -1; // Inválido
		col = -1; // Inválido
		return false;
	}
}

// --- Añadido: Funciones de Raycasting ---

// Calcula la dirección del rayo en coordenadas del MUNDO desde la posición del mouse
glm::vec3 CalculateMouseRay(GLFWwindow* window, double xpos, double ypos, const Camera& cam, const glm::mat4& projectionMatrix) {
	// 1. Coordenadas Normalizadas del Dispositivo (NDC)
	// Convierte las coordenadas de pantalla (0 a Width, 0 a Height) a (-1 a 1, -1 a 1)
	// Es importante invertir Y porque las coordenadas de pantalla suelen empezar arriba
	// y OpenGL las espera abajo.
	int screenWidth, screenHeight;
	glfwGetFramebufferSize(window, &screenWidth, &screenHeight); // Usa el tamaño del framebuffer
	float ndcX = (2.0f * static_cast<float>(xpos)) / screenWidth - 1.0f;
	float ndcY = 1.0f - (2.0f * static_cast<float>(ypos)) / screenHeight; // Invertir Y
	float ndcZ = -1.0f; // Queremos la dirección "hacia adelante" en la pantalla (cerca)

	// 2. Coordenadas de Clip (Homogéneas)
	// Simplemente añadimos la componente 'w'. Para la dirección, Z=-1 y W=1 si es posición,
	// pero para la dirección final en el mundo, W será 0. Por ahora, usamos W=1.
	glm::vec4 ray_clip = glm::vec4(ndcX, ndcY, -1.0f, 1.0f);

	// 3. Coordenadas de Cámara (Eye Space)
	// Des-proyectamos usando la inversa de la matriz de proyección
	glm::mat4 invProjection = glm::inverse(projectionMatrix);
	glm::vec4 ray_eye = invProjection * ray_clip;
	// Queremos una dirección, no un punto. Z=-1 indica "hacia adelante" en espacio de cámara.
	// W=0 indica que es un vector de dirección.
	ray_eye = glm::vec4(ray_eye.x, ray_eye.y, -1.0f, 0.0f);

	// 4. Coordenadas del Mundo (World Space)
	// Des-transformamos usando la inversa de la matriz de vista
	glm::mat4 invView = glm::inverse(cam.GetViewMatrix());
	glm::vec3 ray_world = glm::vec3(invView * ray_eye);

	// 5. Normalizar el vector de dirección
	ray_world = glm::normalize(ray_world);

	return ray_world;
}

// Calcula la distancia 't' a lo largo del rayo donde intersecta un plano.
// Devuelve -1.0 si no hay intersección o si está detrás del origen del rayo.
// planeNormal: Vector normal al plano (debe ser unitario)
// planePoint: Cualquier punto conocido que esté sobre el plano
float RayPlaneIntersection(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, const glm::vec3& planePoint, const glm::vec3& planeNormal) {

	// Denominador de la fórmula de intersección
	float denominator = glm::dot(planeNormal, rayDirection);

	// Comprobar si el rayo es paralelo al plano (o casi)
	if (std::abs(denominator) < 0.0001f) {
		return -1.0f; // No hay intersección (o infinitas si el origen está en el plano)
	}

	// Numerador de la fórmula
	glm::vec3 vectorToPlane = planePoint - rayOrigin;
	float numerator = glm::dot(planeNormal, vectorToPlane);

	// Calcular la distancia 't'
	float t = numerator / denominator;

	// Devolver 't' solo si la intersección está "delante" del rayo
	// (Podrías querer permitir t=0 si el origen está justo en el plano)
	// if (t >= 0.0f) {
	//     return t;
	// } else {
	//     return -1.0f; // Intersección detrás del origen del rayo
	// }
	return t; // Devolvemos t incluso si es negativo por ahora, lo filtraremos después
}


// --- ACTUALIZADA: Obtener el punto de intersección en el plano del tablero usando Raycasting ---
// --- ACTUALIZADA v2: Obtener el punto de intersección en el plano del tablero usando Raycasting ---
// Calcula la matriz de proyección internamente usando cam.GetZoom()
glm::vec3 GetBoardIntersectionPoint(GLFWwindow* window, double xpos, double ypos, const Camera& cam) {

	// Obtener dimensiones actuales del framebuffer para el aspect ratio
	int screenWidth, screenHeight;
	glfwGetFramebufferSize(window, &screenWidth, &screenHeight);
	// Evitar división por cero si la ventana está minimizada
	if (screenHeight == 0) screenHeight = 1;
	float aspectRatio = static_cast<float>(screenWidth) / static_cast<float>(screenHeight);

	// --- CORRECCIÓN AQUÍ ---
	// Calcular la matriz de proyección usando los datos de la cámara y la ventana
	// Asume que GetZoom() devuelve el FoV en grados. Near=0.1, Far=100.0 son valores comunes.
	glm::mat4 projectionMatrix = glm::perspective(glm::radians(cam.GetZoom()), aspectRatio, 0.1f, 100.0f);
	// --- FIN CORRECCIÓN ---

	// Calcular la dirección del rayo en coordenadas del mundo
	// (CalculateMouseRay necesita la matriz de proyección que acabamos de calcular)
	glm::vec3 rayDirection = CalculateMouseRay(window, xpos, ypos, cam, projectionMatrix);

	// Origen del rayo es la posición de la cámara
	glm::vec3 rayOrigin = cam.GetPosition();

	// Definir el plano del tablero
	glm::vec3 planeNormal = glm::vec3(0.0f, 1.0f, 0.0f); // Normal Y hacia arriba
	glm::vec3 planePoint = glm::vec3(0.0f, PIECE_Y_OFFSET, 0.0f); // Punto en el plano Y=PIECE_Y_OFFSET

	// Calcular la intersección
	float t = RayPlaneIntersection(rayOrigin, rayDirection, planePoint, planeNormal);

	// Verificar si hubo intersección válida (delante de la cámara)
	if (t >= 0.0f) {
		// Calcular el punto exacto de intersección
		glm::vec3 intersectionPoint = rayOrigin + rayDirection * t;
		return intersectionPoint;
	}
	else {
		// No hubo intersección válida
		return glm::vec3(-999.0f); // Devolver un punto inválido conocido
	}
}

// Callback para botones del mouse
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods, Shader& lightingShader) {
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);
		glm::vec3 intersectionPoint = GetBoardIntersectionPoint(window, xpos, ypos, camera);

		if (intersectionPoint.y > -900.0f) {
			int targetRow, targetCol;
			if (WorldToBoardCoordinates(intersectionPoint, targetRow, targetCol)) {
				ChessPiece& clickedPiece = board[targetRow][targetCol];
				Shader& lightingShader = *static_cast<Shader*>(glfwGetWindowUserPointer(window));

				if (selectedPiece == nullptr) {
					// Nada seleccionado: Intentar seleccionar una pieza
					if (clickedPiece.type != EMPTY && clickedPiece.color == currentPlayer) {
						// Resaltar la nueva pieza seleccionada
						clickedPiece.isSelected = true;
						selectedPiece = &clickedPiece;
						selectedRow = targetRow;
						selectedCol = targetCol;

						// Activar resaltado en el shader (amarillo)
						GLuint highlightLoc = glGetUniformLocation(lightingShader.Program, "highlightColor");
						glUniform3f(highlightLoc, 1.0f, 1.0f, 0.0f);

						std::cout << "Pieza seleccionada: " << selectedPiece->type << " en " << targetRow << "," << targetCol << std::endl;
					}
				}
				else {
					// Ya hay una pieza seleccionada: Intentar moverla
					if (IsValidMove(selectedPiece, targetRow, targetCol)) {
						// Quitar resaltado de la pieza actual
						selectedPiece->isSelected = false;
						GLuint highlightLoc = glGetUniformLocation(lightingShader.Program, "highlightColor");
						glUniform3f(highlightLoc, 0.0f, 0.0f, 0.0f);

						// Mover la pieza lógicamente
						ChessPiece& targetSquare = board[targetRow][targetCol];

						// Captura (si hay pieza enemiga)
						if (targetSquare.type != EMPTY) {
							std::cout << "Captura realizada en " << targetRow << "," << targetCol << std::endl;
						}

						// Mover la pieza
						targetSquare = *selectedPiece;
						targetSquare.row = targetRow;
						targetSquare.col = targetCol;
						targetSquare.isSelected = false; // Asegurar que no lleve el resaltado

						// Vaciar la casilla original
						board[selectedRow][selectedCol] = ChessPiece();
						board[selectedRow][selectedCol].row = selectedRow;
						board[selectedRow][selectedCol].col = selectedCol;

						std::cout << "Pieza movida de " << selectedRow << "," << selectedCol << " a " << targetRow << "," << targetCol << std::endl;

						// Cambiar turno
						currentPlayer = (currentPlayer == WHITE) ? BLACK : WHITE;
						std::cout << "Turno de: " << (currentPlayer == WHITE ? "Blancas" : "Negras") << std::endl;

						// Deseleccionar
						selectedPiece = nullptr;
						selectedRow = -1;
						selectedCol = -1;
					}
					else {
						// Movimiento inválido
						std::cout << "Movimiento inválido a " << targetRow << "," << targetCol << std::endl;

						// Si el clic fue en otra pieza propia, cambiar selección
						if (clickedPiece.type != EMPTY && clickedPiece.color == currentPlayer) {
							// Quitar resaltado de la pieza anterior
							selectedPiece->isSelected = false;

							// Resaltar la nueva pieza seleccionada
							clickedPiece.isSelected = true;
							selectedPiece = &clickedPiece;
							selectedRow = targetRow;
							selectedCol = targetCol;

							// Actualizar resaltado en el shader
							GLuint highlightLoc = glGetUniformLocation(lightingShader.Program, "highlightColor");
							glUniform3f(highlightLoc, 1.0f, 1.0f, 0.0f);

							std::cout << "Nueva pieza seleccionada: " << selectedPiece->type << " en " << targetRow << "," << targetCol << std::endl;
						}
						else {
							// Cancelar selección
							selectedPiece->isSelected = false;
							GLuint highlightLoc = glGetUniformLocation(lightingShader.Program, "highlightColor");
							glUniform3f(highlightLoc, 0.0f, 0.0f, 0.0f);

							selectedPiece = nullptr;
							selectedRow = -1;
							selectedCol = -1;
							std::cout << "Selección cancelada." << std::endl;
						}
					}
				}
			}
			else {
				// Clic fuera del tablero
				std::cout << "Clic fuera del tablero (según conversión)." << std::endl;
				if (selectedPiece != nullptr) {
					selectedPiece->isSelected = false;
					GLuint highlightLoc = glGetUniformLocation(lightingShader.Program, "highlightColor");
					glUniform3f(highlightLoc, 0.0f, 0.0f, 0.0f);

					selectedPiece = nullptr;
					selectedRow = -1;
					selectedCol = -1;
					std::cout << "Selección cancelada." << std::endl;
				}
			}
		}
		else {
			// Clic no intersectó el tablero
			std::cout << "Clic no intersectó el plano del tablero." << std::endl;
			if (selectedPiece != nullptr) {
				selectedPiece->isSelected = false;
				GLuint highlightLoc = glGetUniformLocation(lightingShader.Program, "highlightColor");
				glUniform3f(highlightLoc, 0.0f, 0.0f, 0.0f);

				selectedPiece = nullptr;
				selectedRow = -1;
				selectedCol = -1;
				std::cout << "Selección cancelada." << std::endl;
			}
		}
	}
}

// --- Fin de Añadido ---

//Inicializacion del tablero
// --- Añadido: Función para inicializar el tablero ---
void InitializeBoard(
	// Pasa aquí los punteros a tus modelos cargados
	Model* pPeonW, Model* pTorreW, Model* pCaballoW, Model* pAlfilW, Model* pReinaW, Model* pReyW,
	Model* pPeonB, Model* pTorreB, Model* pCaballoB, Model* pAlfilB, Model* pReinaB, Model* pReyB
) {
	// Limpiar tablero (opcional, por si acaso)
	for (int r = 0; r < 8; ++r) {
		for (int c = 0; c < 8; ++c) {
			board[r][c] = ChessPiece(); // Crea pieza vacía por defecto
			board[r][c].row = r;
			board[r][c].col = c;
		}
	}

	// Colocar piezas blancas (Overworld en tu código) - ¡AJUSTA MODELOS Y ESCALAS!
	// Fila 0 (Trasera Blanca)
	board[0][0] = { ROOK,   WHITE, pTorreW,   0, 0, glm::vec3(0.0f), glm::vec3(3.5f), 0.0f }; // Golem
	board[0][1] = { KNIGHT, WHITE, pCaballoW, 0, 1, glm::vec3(0.0f), glm::vec3(4.35f), 0.0f }; // Caballo
	board[0][2] = { BISHOP, WHITE, pAlfilW,   0, 2, glm::vec3(0.0f,-1.0f, 0.0f), glm::vec3(4.15f), 0.0f }; // Perro
	board[0][3] = { QUEEN,  WHITE, pReinaW,   0, 3, glm::vec3(0.0f,-2.0f, 0.0f), glm::vec3(0.65f), -1.55f }; // Alex
	board[0][4] = { KING,   WHITE, pReyW,     0, 4, glm::vec3(0.0f,0.2f, 0.0f), glm::vec3(5.15f), 0.0f }; // Steve
	board[0][5] = { BISHOP, WHITE, pAlfilW,   0, 5, glm::vec3(0.0f,-1.0f, 0.0f), glm::vec3(4.15f), 0.0f }; // Perro
	board[0][6] = { KNIGHT, WHITE, pCaballoW, 0, 6, glm::vec3(0.0f), glm::vec3(4.35f), 0.0f }; // Caballo
	board[0][7] = { ROOK,   WHITE, pTorreW,   0, 7, glm::vec3(0.0f), glm::vec3(3.5f), 0.0f }; // Golem
	// Fila 1 (Peones Blancos)
	for (int c = 0; c < 8; ++c) {
		board[1][c] = { PAWN, WHITE, pPeonW, 1, c, glm::vec3(0.0f,-1.0f, 0.0f), glm::vec3(4.0f), 0.0f }; // Pollo
	}

	// Colocar piezas negras (Nether en tu código) - ¡AJUSTA MODELOS, ESCALAS y ROTACIÓN!
	// Fila 7 (Trasera Negra)
	board[7][0] = { ROOK,   BLACK, pTorreB,   7, 0, glm::vec3(0.0f), glm::vec3(0.7f), 1.55f }; // Piglin
	board[7][1] = { KNIGHT, BLACK, pCaballoB, 7, 1, glm::vec3(0.0f,0.35f, 0.0f), glm::vec3(4.15f, 4.15f, -4.15f), 0.0f }; // Blaze
	board[7][2] = { BISHOP, BLACK, pAlfilB,   7, 2, glm::vec3(0.0f,0.45f, 0.0f), glm::vec3(3.75f, 3.75f, -3.75f), 0.0f }; // Enderman
	board[7][3] = { QUEEN,  BLACK, pReinaB,   7, 3, glm::vec3(0.0f,-1.25f, 0.0f), glm::vec3(0.75f, 0.75f, -0.75f), 0.0f }; // Dragon
	board[7][4] = { KING,   BLACK, pReyB,     7, 4, glm::vec3(0.0f), glm::vec3(0.45f), 1.55f }; // Warden
	board[7][5] = { BISHOP, BLACK, pAlfilB,   7, 5, glm::vec3(0.0f,0.45f, 0.0f), glm::vec3(3.75f, 3.75f, -3.75f), 0.0f }; // Enderman
	board[7][6] = { KNIGHT, BLACK, pCaballoB, 7, 6, glm::vec3(0.0f,0.35f, 0.0f), glm::vec3(4.15f, 4.15f, -4.15f), 0.0f }; // Blaze
	board[7][7] = { ROOK,   BLACK, pTorreB,   7, 7, glm::vec3(0.0f), glm::vec3(0.7f), 1.55f }; // Piglin
	// Fila 6 (Peones Negros)
	for (int c = 0; c < 8; ++c) {
		board[6][c] = { PAWN, BLACK, pPeonB, 6, c, glm::vec3(0.0f,-0.25f, 0.0f), glm::vec3(4.0f, 4.0f, -4.0f), 0.0f }; // Esqueleto
	}

	// Asignar posiciones iniciales a todas las piezas (importante para la estructura)
	for (int r = 0; r < 8; ++r) {
		for (int c = 0; c < 8; ++c) {
			board[r][c].row = r;
			board[r][c].col = c;
		}
	}
}

// --- Añadido: Función para obtener coordenadas del mundo desde fila/columna ---
glm::vec3 GetWorldCoordinates(int row, int col) {
	// Centra la pieza en la casilla
	float worldX = BOARD_OFFSET_X + col * TILE_SIZE + TILE_SIZE / 2.0f;
	float worldZ = BOARD_OFFSET_Z + row * TILE_SIZE + TILE_SIZE / 2.0f;
	return glm::vec3(worldX, PIECE_Y_OFFSET, worldZ);
}

// --- Fin de Añadidos ---


