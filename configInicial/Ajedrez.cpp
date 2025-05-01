#include <iostream>
#include <cmath>
#include <vector>

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

// Estructura de Piezas
#include <vector>
#include <map>

// Tipos de Pieza
enum PieceType { EMPTY, PAWN, ROOK, KNIGHT, BISHOP, QUEEN, KING };
// Colores de Pieza
enum PieceColor { NONE, WHITE, BLACK };

// Estructura para representar una Pieza de Ajedrez
struct ChessPiece {
	PieceType type = EMPTY;
	PieceColor color = NONE;
	Model* model = nullptr;
	int row = -1;
	int col = -1;
	glm::vec3 positionOffset = glm::vec3(0.0f);
	glm::vec3 scale = glm::vec3(1.0f);
	float rotationY = 1.0f;
	bool isSelected = false;
};

// Representación del tablero (8x8)
ChessPiece board[8][8];

// Listas para piezas capturadas
std::vector<ChessPiece> whiteCapturedPieces;
std::vector<ChessPiece> blackCapturedPieces;

// Variables para manejar la selección y movimiento
ChessPiece* selectedPiece = nullptr;
int selectedRow = -1;
int selectedCol = -1;
PieceColor currentPlayer = WHITE;

// Dimensiones y posición del tablero
const float TILE_SIZE = 5.0f;
const float BOARD_OFFSET_X = -20.25f;
const float BOARD_OFFSET_Z = 1.0f;
const float PIECE_Y_OFFSET = -2.0f;

// Posiciones para piezas capturadas
const float CAPTURED_WHITE_X = BOARD_OFFSET_X - TILE_SIZE * 2.0f;
const float CAPTURED_BLACK_X = BOARD_OFFSET_X + TILE_SIZE * 10.0f;
const float CAPTURED_Y = PIECE_Y_OFFSET;
const float CAPTURED_Z_START = BOARD_OFFSET_Z;

// Contadores para piezas capturadas
int whiteCapturedCount = 0;
int blackCapturedCount = 0;

// Function prototypes
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void MouseCallback(GLFWwindow* window, double xPos, double yPos);
void DoMovement();
void InitializeBoard(
	Model* pPeonW, Model* pTorreW, Model* pCaballoW, Model* pAlfilW, Model* pReinaW, Model* pReyW,
	Model* pPeonB, Model* pTorreB, Model* pCaballoB, Model* pAlfilB, Model* pReinaB, Model* pReyB
);
glm::vec3 GetWorldCoordinates(int row, int col);
bool IsPathClear(int startRow, int startCol, int endRow, int endCol);
bool IsValidMove(ChessPiece* piece, int targetRow, int targetCol);
bool WorldToBoardCoordinates(const glm::vec3& worldPos, int& row, int& col);
glm::vec3 CalculateMouseRay(GLFWwindow* window, double xpos, double ypos, const Camera& cam, const glm::mat4& projectionMatrix);
float RayPlaneIntersection(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, const glm::vec3& planePoint, const glm::vec3& planeNormal);
glm::vec3 GetBoardIntersectionPoint(GLFWwindow* window, double xpos, double ypos, const Camera& cam);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void MoveCapturedPiece(ChessPiece& piece);

// Window dimensions
const GLuint WIDTH = 1200, HEIGHT = 1000;
int SCREEN_WIDTH, SCREEN_HEIGHT;

// Camera
Camera  camera(glm::vec3(0.0f, 12.0f, 35.0f));
GLfloat lastX = WIDTH / 2.0;
GLfloat lastY = HEIGHT / 2.0;
bool keys[1024];
bool firstMouse = true;
glm::vec3 lightPos(0.0f, 0.0f, 0.0f);
bool active;

// Positions of the point lights
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

GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

int main()
{
	glfwInit();
	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Ajedrez 3D", nullptr, nullptr);

	if (nullptr == window)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return EXIT_FAILURE;
	}

	glfwMakeContextCurrent(window);
	glfwGetFramebufferSize(window, &SCREEN_WIDTH, &SCREEN_HEIGHT);
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetCursorPosCallback(window, MouseCallback);
	glfwSetMouseButtonCallback(window, MouseButtonCallback);

	glewExperimental = GL_TRUE;
	if (GLEW_OK != glewInit())
	{
		std::cout << "Failed to initialize GLEW" << std::endl;
		return EXIT_FAILURE;
	}

	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

	Shader lightingShader("Shader/lighting.vs", "Shader/lighting.frag");
	Shader lampShader("Shader/lamp.vs", "Shader/lamp.frag");

	// Carga de modelos
	Model Piso((char*)"Models/Minecraft/Tablero2.obj");
	// Overworld
	Model steve((char*)"Models/Minecraft/steve.obj");
	Model alex((char*)"Models/Minecraft/alex.obj");
	Model golem((char*)"Models/Minecraft/golem.obj");
	Model caballo((char*)"Models/Minecraft/caballo.obj");
	Model perro((char*)"Models/Minecraft/perro.obj");
	Model pollo((char*)"Models/Minecraft/pollo.obj");
	// Nether
	Model warden((char*)"Models/Minecraft/warden.obj");
	Model dragon((char*)"Models/Minecraft/dragon.obj");
	Model piglin((char*)"Models/Minecraft/piglin.obj");
	Model blaze((char*)"Models/Minecraft/blaze.obj");
	Model enderman((char*)"Models/Minecraft/enderman.obj");
	Model esqueleto((char*)"Models/Minecraft/esqueleto.obj");

	InitializeBoard(
		&pollo, &golem, &caballo, &perro, &alex, &steve,
		&esqueleto, &piglin, &blaze, &enderman, &dragon, &warden
	);

	GLuint VBO, VAO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	lightingShader.Use();
	glUniform1i(glGetUniformLocation(lightingShader.Program, "Material.difuse"), 0);
	glUniform1i(glGetUniformLocation(lightingShader.Program, "Material.specular"), 1);

	glm::mat4 projection = glm::perspective(camera.GetZoom(), (GLfloat)SCREEN_WIDTH / (GLfloat)SCREEN_HEIGHT, 0.1f, 100.0f);

	while (!glfwWindowShouldClose(window))
	{
		GLfloat currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		glfwPollEvents();
		DoMovement();

		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);

		lightingShader.Use();
		glUniform1i(glGetUniformLocation(lightingShader.Program, "diffuse"), 0);

		GLint viewPosLoc = glGetUniformLocation(lightingShader.Program, "viewPos");
		glUniform3f(viewPosLoc, camera.GetPosition().x, camera.GetPosition().y, camera.GetPosition().z);

		// Directional light
		glUniform3f(glGetUniformLocation(lightingShader.Program, "dirLight.direction"), -0.2f, -1.0f, -0.3f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "dirLight.ambient"), 0.8f, 0.8f, 0.8f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "dirLight.diffuse"), 0.5f, 0.5f, 0.5f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "dirLight.specular"), 0.0f, 0.0f, 0.0f);

		// Point light 1
		glm::vec3 lightColor;
		lightColor.x = abs(sin(glfwGetTime() * Light1.x));
		lightColor.y = abs(sin(glfwGetTime() * Light1.y));
		lightColor.z = sin(glfwGetTime() * Light1.z);

		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[0].position"), pointLightPositions[0].x, pointLightPositions[0].y, pointLightPositions[0].z);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[0].ambient"), lightColor.x, lightColor.y, lightColor.z);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[0].diffuse"), lightColor.x, lightColor.y, lightColor.z);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[0].specular"), 0.0f, 0.0f, 0.0f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[0].constant"), 1.0f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[0].linear"), 0.045f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[0].quadratic"), 0.075f);

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

		// SpotLight
		glUniform3f(glGetUniformLocation(lightingShader.Program, "spotLight.position"), camera.GetPosition().x, camera.GetPosition().y, camera.GetPosition().z);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "spotLight.direction"), camera.GetFront().x, camera.GetFront().y, camera.GetFront().z);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "spotLight.ambient"), 0.0f, 0.0f, 0.0f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "spotLight.diffuse"), 0.0f, 0.0f, 0.0f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "spotLight.specular"), 0.0f, 0.0f, 0.0f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "spotLight.constant"), 1.0f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "spotLight.linear"), 0.0f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "spotLight.quadratic"), 0.0f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "spotLight.cutOff"), glm::cos(glm::radians(12.0f)));
		glUniform1f(glGetUniformLocation(lightingShader.Program, "spotLight.outerCutOff"), glm::cos(glm::radians(12.0f)));

		// Set material properties
		glUniform1f(glGetUniformLocation(lightingShader.Program, "material.shininess"), 16.0f);

		glm::mat4 view = camera.GetViewMatrix();
		GLint modelLoc = glGetUniformLocation(lightingShader.Program, "model");
		GLint viewLoc = glGetUniformLocation(lightingShader.Program, "view");
		GLint projLoc = glGetUniformLocation(lightingShader.Program, "projection");

		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

		// Dibujar el tablero
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::scale(model, glm::vec3(82.0f, 1.0f, 84.0f));
		model = glm::translate(model, glm::vec3(0.0f, -3.75f, 0.25f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
		Piso.Draw(lightingShader);

		// Dibujar las piezas en el tablero
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

					if (piece.isSelected) {
						glm::vec3 highlightColor = glm::vec3(1.0f, 1.0f, 0.0f);
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

		// Dibujar piezas capturadas blancas
		for (size_t i = 0; i < whiteCapturedPieces.size(); i++) {
			ChessPiece& piece = whiteCapturedPieces[i];
			if (piece.model != nullptr) {
				glm::mat4 model = glm::mat4(1.0f);
				float worldX = CAPTURED_WHITE_X;
				float worldZ = CAPTURED_Z_START + i * TILE_SIZE * 0.5f;
				model = glm::translate(model, glm::vec3(worldX, CAPTURED_Y, worldZ));
				if (piece.rotationY != 0.0f) {
					model = glm::rotate(model, piece.rotationY, glm::vec3(0.0f, 1.0f, 0.0f));
				}
				model = glm::scale(model, piece.scale);
				glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
				piece.model->Draw(lightingShader);
			}
		}

		// Dibujar piezas capturadas negras
		for (size_t i = 0; i < blackCapturedPieces.size(); i++) {
			ChessPiece& piece = blackCapturedPieces[i];
			if (piece.model != nullptr) {
				glm::mat4 model = glm::mat4(1.0f);
				float worldX = CAPTURED_BLACK_X;
				float worldZ = CAPTURED_Z_START + i * TILE_SIZE * 0.5f;
				model = glm::translate(model, glm::vec3(worldX, CAPTURED_Y, worldZ));
				if (piece.rotationY != 0.0f) {
					model = glm::rotate(model, piece.rotationY, glm::vec3(0.0f, 1.0f, 0.0f));
				}
				model = glm::scale(model, piece.scale);
				glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
				piece.model->Draw(lightingShader);
			}
		}

		// Dibujar las luces
		lampShader.Use();
		modelLoc = glGetUniformLocation(lampShader.Program, "model");
		viewLoc = glGetUniformLocation(lampShader.Program, "view");
		projLoc = glGetUniformLocation(lampShader.Program, "projection");

		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

		for (GLuint i = 0; i < 4; i++) {
			model = glm::mat4(1);
			model = glm::translate(model, pointLightPositions[i]);
			model = glm::scale(model, glm::vec3(0.2f));
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
			glBindVertexArray(VAO);
			glDrawArrays(GL_TRIANGLES, 0, 36);
		}
		glBindVertexArray(0);

		glfwSwapBuffers(window);
	}

	glfwTerminate();
	return 0;
}

void MoveCapturedPiece(ChessPiece& piece) {
	if (piece.color == WHITE) {
		whiteCapturedPieces.push_back(piece);
		whiteCapturedCount++;
	}
	else {
		blackCapturedPieces.push_back(piece);
		blackCapturedCount++;
	}
	// Marcar la pieza como capturada
	piece.type = EMPTY;
	piece.color = NONE;
	piece.model = nullptr;
}

void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);

		glm::vec3 intersectionPoint = GetBoardIntersectionPoint(window, xpos, ypos, camera);

		if (intersectionPoint.y > -900.0f) {
			int targetRow, targetCol;
			if (WorldToBoardCoordinates(intersectionPoint, targetRow, targetCol)) {
				ChessPiece& clickedPiece = board[targetRow][targetCol];

				if (selectedPiece == nullptr) {
					if (clickedPiece.type != EMPTY && clickedPiece.color == currentPlayer) {
						clickedPiece.isSelected = true;
						selectedPiece = &clickedPiece;
						selectedRow = targetRow;
						selectedCol = targetCol;
					}
				}
				else {
					if (IsValidMove(selectedPiece, targetRow, targetCol)) {
						selectedPiece->isSelected = false;
						ChessPiece& targetSquare = board[targetRow][targetCol];

						// Captura de pieza
						if (targetSquare.type != EMPTY) {
							MoveCapturedPiece(targetSquare);
						}

						// Mover la pieza
						targetSquare = *selectedPiece;
						targetSquare.row = targetRow;
						targetSquare.col = targetCol;

						// Vaciar la casilla original
						board[selectedRow][selectedCol] = ChessPiece();
						board[selectedRow][selectedCol].row = selectedRow;
						board[selectedRow][selectedCol].col = selectedCol;

						// Cambiar turno
						currentPlayer = (currentPlayer == WHITE) ? BLACK : WHITE;

						// Deseleccionar
						selectedPiece = nullptr;
						selectedRow = -1;
						selectedCol = -1;
					}
					else {
						if (board[targetRow][targetCol].type != EMPTY &&
							board[targetRow][targetCol].color == currentPlayer) {
							selectedPiece->isSelected = false;
							selectedPiece = &board[targetRow][targetCol];
							selectedPiece->isSelected = true;
							selectedRow = targetRow;
							selectedCol = targetCol;
						}
						else {
							selectedPiece->isSelected = false;
							selectedPiece = nullptr;
							selectedRow = -1;
							selectedCol = -1;
						}
					}
				}
			}
		}
	}
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
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode)
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

void MouseCallback(GLFWwindow* window, double xPos, double yPos)
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


// --- A�adido: Funci�n auxiliar para verificar caminos ---
// Verifica si el camino entre (startRow, startCol) y (endRow, endCol) est� vac�o.
// �NO verifica la validez del movimiento en s� (horizontal, vertical, diagonal)!
// Asume que el movimiento es en l�nea recta (horizontal, vertical o diagonal perfecta).
// No chequea la casilla final (endRow, endCol), solo las intermedias.
bool IsPathClear(int startRow, int startCol, int endRow, int endCol) {
	// Determinar la direcci�n del movimiento (paso en x, paso en y)
	int stepY = (endRow > startRow) ? 1 : ((endRow < startRow) ? -1 : 0);
	int stepX = (endCol > startCol) ? 1 : ((endCol < startCol) ? -1 : 0);

	// Casilla actual empieza una despu�s del inicio
	int currentRow = startRow + stepY;
	int currentCol = startCol + stepX;

	// Recorrer el camino hasta llegar a la casilla final
	while (currentRow != endRow || currentCol != endCol) {
		// Si alguna casilla intermedia no est� vac�a, el camino est� bloqueado
		if (currentRow < 0 || currentRow >= 8 || currentCol < 0 || currentCol >= 8) {
			// Seguridad: Si por alg�n error el c�lculo sale del tablero
			std::cerr << "Error en IsPathClear: fuera de l�mites (" << currentRow << "," << currentCol << ")" << std::endl;
			return false;
		}
		if (board[currentRow][currentCol].type != EMPTY) {
			return false; // Camino bloqueado
		}
		// Avanzar a la siguiente casilla en el camino
		currentRow += stepY;
		currentCol += stepX;
	}

	// Si el bucle termina, significa que todas las casillas intermedias estaban vac�as
	return true; // Camino despejado
}
// --- Fin de A�adido ---

// --- ACTUALIZADA: Validaci�n de Movimientos de Ajedrez (Reglas B�sicas) ---
bool IsValidMove(ChessPiece* piece, int targetRow, int targetCol) {
	// 1. Chequeos Iniciales B�sicos
	if (!piece || piece->type == EMPTY) {
		std::cerr << "Error IsValidMove: Pieza inv�lida o vac�a." << std::endl;
		return false;
	}
	if (targetRow < 0 || targetRow >= 8 || targetCol < 0 || targetCol >= 8) {
		// std::cout << "Movimiento inv�lido: Fuera del tablero." << std::endl; // Mucho spam
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
		// std::cout << "Movimiento inv�lido: No puedes capturar tu propia pieza." << std::endl;
		return false;
	}

	// 2. L�gica Espec�fica por Tipo de Pieza
	switch (piece->type) {
	case PAWN: {
		int forward = (piece->color == WHITE) ? 1 : -1; // Direcci�n de avance
		// Mover 1 casilla adelante
		if (targetCol == startCol && targetRow == startRow + forward && targetSquare.type == EMPTY) {
			return true;
		}
		// Mover 2 casillas adelante (solo desde posici�n inicial)
		bool isStartingRow = (piece->color == WHITE && startRow == 1) || (piece->color == BLACK && startRow == 6);
		if (isStartingRow && targetCol == startCol && targetRow == startRow + 2 * forward && targetSquare.type == EMPTY) {
			// Verificar que la casilla intermedia tambi�n est� vac�a
			if (board[startRow + forward][startCol].type == EMPTY) {
				return true;
			}
		}
		// Captura diagonal
		if (std::abs(targetCol - startCol) == 1 && targetRow == startRow + forward && targetSquare.type != EMPTY && targetSquare.color != piece->color) {
			return true;
		}
		// Faltan: En Passant, Promoci�n
		return false; // Si no es ninguno de los anteriores, es inv�lido para el pe�n
	}

	case ROOK: {
		// Debe ser movimiento horizontal o vertical
		if (startRow != targetRow && startCol != targetCol) {
			return false; // No es movimiento de torre
		}
		// Verificar que el camino est� despejado
		return IsPathClear(startRow, startCol, targetRow, targetCol);
	}

	case KNIGHT: {
		int dRow = std::abs(targetRow - startRow);
		int dCol = std::abs(targetCol - startCol);
		// Movimiento en 'L' (2 en una direcci�n, 1 en la perpendicular)
		return (dRow == 2 && dCol == 1) || (dRow == 1 && dCol == 2);
		// El caballo salta, no necesita IsPathClear
	}

	case BISHOP: {
		// Debe ser movimiento diagonal
		if (std::abs(targetRow - startRow) != std::abs(targetCol - startCol)) {
			return false; // No es movimiento de alfil
		}
		// Verificar que el camino est� despejado
		return IsPathClear(startRow, startCol, targetRow, targetCol);
	}

	case QUEEN: {
		// Debe ser movimiento horizontal, vertical o diagonal
		bool isStraight = (startRow == targetRow || startCol == targetCol);
		bool isDiagonal = (std::abs(targetRow - startRow) == std::abs(targetCol - startCol));
		if (!isStraight && !isDiagonal) {
			return false; // No es movimiento de reina
		}
		// Verificar que el camino est� despejado
		return IsPathClear(startRow, startCol, targetRow, targetCol);
	}

	case KING: {
		int dRow = std::abs(targetRow - startRow);
		int dCol = std::abs(targetCol - startCol);
		// Mover solo 1 casilla en cualquier direcci�n
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


// Funci�n para intentar convertir coordenadas del mundo a tablero (�SIMPLIFICADA!)
// Asume que el eje Y es la altura y que el tablero est� en el plano XZ
bool WorldToBoardCoordinates(const glm::vec3& worldPos, int& row, int& col) {
	// Calcula columna basada en X
	float boardX = worldPos.x - BOARD_OFFSET_X;
	col = static_cast<int>(floor(boardX / TILE_SIZE));

	// Calcula fila basada en Z
	float boardZ = worldPos.z - BOARD_OFFSET_Z;
	row = static_cast<int>(floor(boardZ / TILE_SIZE));

	// Verifica si est� dentro de los l�mites del tablero
	if (row >= 0 && row < 8 && col >= 0 && col < 8) {
		return true;
	}
	else {
		row = -1; // Inv�lido
		col = -1; // Inv�lido
		return false;
	}
}

// --- A�adido: Funciones de Raycasting ---

// Calcula la direcci�n del rayo en coordenadas del MUNDO desde la posici�n del mouse
glm::vec3 CalculateMouseRay(GLFWwindow* window, double xpos, double ypos, const Camera& cam, const glm::mat4& projectionMatrix) {
	// 1. Coordenadas Normalizadas del Dispositivo (NDC)
	// Convierte las coordenadas de pantalla (0 a Width, 0 a Height) a (-1 a 1, -1 a 1)
	// Es importante invertir Y porque las coordenadas de pantalla suelen empezar arriba
	// y OpenGL las espera abajo.
	int screenWidth, screenHeight;
	glfwGetFramebufferSize(window, &screenWidth, &screenHeight); // Usa el tama�o del framebuffer
	float ndcX = (2.0f * static_cast<float>(xpos)) / screenWidth - 1.0f;
	float ndcY = 1.0f - (2.0f * static_cast<float>(ypos)) / screenHeight; // Invertir Y
	float ndcZ = -1.0f; // Queremos la direcci�n "hacia adelante" en la pantalla (cerca)

	// 2. Coordenadas de Clip (Homog�neas)
	// Simplemente a�adimos la componente 'w'. Para la direcci�n, Z=-1 y W=1 si es posici�n,
	// pero para la direcci�n final en el mundo, W ser� 0. Por ahora, usamos W=1.
	glm::vec4 ray_clip = glm::vec4(ndcX, ndcY, -1.0f, 1.0f);

	// 3. Coordenadas de C�mara (Eye Space)
	// Des-proyectamos usando la inversa de la matriz de proyecci�n
	glm::mat4 invProjection = glm::inverse(projectionMatrix);
	glm::vec4 ray_eye = invProjection * ray_clip;
	// Queremos una direcci�n, no un punto. Z=-1 indica "hacia adelante" en espacio de c�mara.
	// W=0 indica que es un vector de direcci�n.
	ray_eye = glm::vec4(ray_eye.x, ray_eye.y, -1.0f, 0.0f);

	// 4. Coordenadas del Mundo (World Space)
	// Des-transformamos usando la inversa de la matriz de vista
	glm::mat4 invView = glm::inverse(cam.GetViewMatrix());
	glm::vec3 ray_world = glm::vec3(invView * ray_eye);

	// 5. Normalizar el vector de direcci�n
	ray_world = glm::normalize(ray_world);

	return ray_world;
}

// Calcula la distancia 't' a lo largo del rayo donde intersecta un plano.
// Devuelve -1.0 si no hay intersecci�n o si est� detr�s del origen del rayo.
// planeNormal: Vector normal al plano (debe ser unitario)
// planePoint: Cualquier punto conocido que est� sobre el plano
float RayPlaneIntersection(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, const glm::vec3& planePoint, const glm::vec3& planeNormal) {

	// Denominador de la f�rmula de intersecci�n
	float denominator = glm::dot(planeNormal, rayDirection);

	// Comprobar si el rayo es paralelo al plano (o casi)
	if (std::abs(denominator) < 0.0001f) {
		return -1.0f; // No hay intersecci�n (o infinitas si el origen est� en el plano)
	}

	// Numerador de la f�rmula
	glm::vec3 vectorToPlane = planePoint - rayOrigin;
	float numerator = glm::dot(planeNormal, vectorToPlane);

	// Calcular la distancia 't'
	float t = numerator / denominator;

	// Devolver 't' solo si la intersecci�n est� "delante" del rayo
	// (Podr�as querer permitir t=0 si el origen est� justo en el plano)
	// if (t >= 0.0f) {
	//     return t;
	// } else {
	//     return -1.0f; // Intersecci�n detr�s del origen del rayo
	// }
	return t; // Devolvemos t incluso si es negativo por ahora, lo filtraremos despu�s
}


// --- ACTUALIZADA: Obtener el punto de intersecci�n en el plano del tablero usando Raycasting ---
// --- ACTUALIZADA v2: Obtener el punto de intersecci�n en el plano del tablero usando Raycasting ---
// Calcula la matriz de proyecci�n internamente usando cam.GetZoom()
glm::vec3 GetBoardIntersectionPoint(GLFWwindow* window, double xpos, double ypos, const Camera& cam) {

	// Obtener dimensiones actuales del framebuffer para el aspect ratio
	int screenWidth, screenHeight;
	glfwGetFramebufferSize(window, &screenWidth, &screenHeight);
	// Evitar divisi�n por cero si la ventana est� minimizada
	if (screenHeight == 0) screenHeight = 1;
	float aspectRatio = static_cast<float>(screenWidth) / static_cast<float>(screenHeight);

	// --- CORRECCI�N AQU� ---
	// Calcular la matriz de proyecci�n usando los datos de la c�mara y la ventana
	// Asume que GetZoom() devuelve el FoV en grados. Near=0.1, Far=100.0 son valores comunes.
	glm::mat4 projectionMatrix = glm::perspective(glm::radians(cam.GetZoom()), aspectRatio, 0.1f, 100.0f);
	// --- FIN CORRECCI�N ---

	// Calcular la direcci�n del rayo en coordenadas del mundo
	// (CalculateMouseRay necesita la matriz de proyecci�n que acabamos de calcular)
	glm::vec3 rayDirection = CalculateMouseRay(window, xpos, ypos, cam, projectionMatrix);

	// Origen del rayo es la posici�n de la c�mara
	glm::vec3 rayOrigin = cam.GetPosition();

	// Definir el plano del tablero
	glm::vec3 planeNormal = glm::vec3(0.0f, 1.0f, 0.0f); // Normal Y hacia arriba
	glm::vec3 planePoint = glm::vec3(0.0f, PIECE_Y_OFFSET, 0.0f); // Punto en el plano Y=PIECE_Y_OFFSET

	// Calcular la intersecci�n
	float t = RayPlaneIntersection(rayOrigin, rayDirection, planePoint, planeNormal);

	// Verificar si hubo intersecci�n v�lida (delante de la c�mara)
	if (t >= 0.0f) {
		// Calcular el punto exacto de intersecci�n
		glm::vec3 intersectionPoint = rayOrigin + rayDirection * t;
		return intersectionPoint;
	}
	else {
		// No hubo intersecci�n v�lida
		return glm::vec3(-999.0f); // Devolver un punto inv�lido conocido
	}
}



//Inicializacion del tablero
// --- A�adido: Funci�n para inicializar el tablero ---
void InitializeBoard(
	// Pasa aqu� los punteros a tus modelos cargados
	Model* pPeonW, Model* pTorreW, Model* pCaballoW, Model* pAlfilW, Model* pReinaW, Model* pReyW,
	Model* pPeonB, Model* pTorreB, Model* pCaballoB, Model* pAlfilB, Model* pReinaB, Model* pReyB
) {
	// Limpiar tablero (opcional, por si acaso)
	for (int r = 0; r < 8; ++r) {
		for (int c = 0; c < 8; ++c) {
			board[r][c] = ChessPiece(); // Crea pieza vac�a por defecto
			board[r][c].row = r;
			board[r][c].col = c;
		}
	}

	// Colocar piezas blancas (Overworld en tu c�digo) - �AJUSTA MODELOS Y ESCALAS!
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

	// Colocar piezas negras (Nether en tu c�digo) - �AJUSTA MODELOS, ESCALAS y ROTACI�N!
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

// --- Anadido: Funcion para obtener coordenadas del mundo desde fila/columna ---
glm::vec3 GetWorldCoordinates(int row, int col) {
	// Centra la pieza en la casilla
	float worldX = BOARD_OFFSET_X + col * TILE_SIZE + TILE_SIZE / 2.0f;
	float worldZ = BOARD_OFFSET_Z + row * TILE_SIZE + TILE_SIZE / 2.0f;
	return glm::vec3(worldX, PIECE_Y_OFFSET, worldZ);
}
