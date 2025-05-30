//****************************************************************************
// Archivo: Ajedrez.cpp
// Proyecto: Ajedrez 3D Interactivo con Temática Minecraft
// Autores: Rodríguez Santana Miguel Angel, Valdes Trejo Rodrigo Alfredo
// Fecha de Creación: 03 de marzo de 2025
// Última Modificación: [08/05/2025]
// Profesor: Ing. Edcel Fuerte Martínez
// Asignatura: [Laboratorio de Computacion Grafica]
// Este archivo contiene la implementación principal del juego de ajedrez 3D.
// Incluye la inicialización de gráficos (OpenGL, GLFW), carga de modelos,
// lógica del juego (tablero, piezas, movimientos), manejo de cámaras,
// interacción con el usuario (ratón y teclado) y el bucle principal de renderizado.

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

// Tipos de Pieza de Ajedrez
enum PieceType { EMPTY, PAWN, ROOK, KNIGHT, BISHOP, QUEEN, KING };
// Colores de Pieza de Ajedrez
enum PieceColor { NONE, WHITE, BLACK }; // NONE se usa para casillas vacías o piezas capturadas

// Estructura para representar una Pieza de Ajedrez en el tablero
struct ChessPiece {
    PieceType type = EMPTY;         // Tipo de la pieza (Peón, Torre, etc.)
    PieceColor color = NONE;        // Color de la pieza (Blanco, Negro)
    Model* model = nullptr;         // Puntero al modelo 3D que representa la pieza
    int row = -1;                   // Fila actual de la pieza en el tablero (0-7)
    int col = -1;                   // Columna actual de la pieza en el tablero (0-7)
    glm::vec3 positionOffset = glm::vec3(0.0f); // Desplazamiento visual para ajustar el modelo
    glm::vec3 scale = glm::vec3(1.0f);          // Escala del modelo
    float rotationY = 0.0f;         // Rotación en el eje Y (en radianes) para orientar el modelo
    bool isSelected = false;        // true si la pieza está actualmente seleccionada por el jugador

    // Para animación
    bool isMoving = false;
    glm::vec3 startPos;
    glm::vec3 targetPos;
    float moveProgress = 0.0f;
    float moveSpeed = 0.2f; // Velocidad de la animación (ajustable)
};
// --- Variables Globales del Estado del Juego ---
// Representación del tablero (8x8)
ChessPiece board[8][8];

// Listas para piezas capturadas
std::vector<ChessPiece> whiteCapturedPieces;
std::vector<ChessPiece> blackCapturedPieces;

// Variables para gestionar la selección y el movimiento actual
ChessPiece* selectedPiece = nullptr; // Puntero a la pieza actualmente seleccionada
int selectedRow = -1;                // Fila de la pieza seleccionada
int selectedCol = -1;                // Columna de la pieza seleccionada
PieceColor currentPlayer = WHITE;    // Jugador cuyo turno es actualmente

// --- Constantes de Configuración del Tablero y Escena ---
const float TILE_SIZE = 5.0f;         // Tamaño de una casilla del tablero en unidades del mundo
const float BOARD_OFFSET_X = -20.25f; // Desplazamiento X para posicionar el tablero en el mundo
const float BOARD_OFFSET_Z = 1.0f;    // Desplazamiento Z para posicionar el tablero en el mundo
const float PIECE_Y_OFFSET = -2.0f;   // Altura (Y) base a la que se colocan las piezas

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
void UpdateAnimations(float deltaTime);
void InitializeBoard( 
    Model* pPeonW, Model* pTorreW, Model* pCaballoW, Model* pAlfilW, Model* pReinaW, Model* pReyW,
    Model* pPeonB, Model* pTorreB, Model* pCaballoB, Model* pAlfilB, Model* pReinaB, Model* pReyB
);//Configura las piezas en sus posiciones iniciales en el tablero.
glm::vec3 GetWorldCoordinates(int row, int col); // Convierte coordenadas de tablero (fila, col) a coordenadas del mundo (x, y, z).
bool IsPathClear(int startRow, int startCol, int endRow, int endCol); // Verifica si no hay obstáculos entre dos casillas para movimientos rectilíneos.
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

// Cámara lateral (vista desde un costado)
Camera cameraSideView(
    glm::vec3(-45.0f, 18.0f, 21.0f),
    glm::vec3(0.0f, 1.0f, 0.0f),
    0.0f,
    -35.0f
);
bool useSideCamera = false;

// Positions of the point lights
glm::vec3 pointLightPositions[] = {
    glm::vec3(-2.0f,-5.0f, -2.0f),
    glm::vec3(2.0f,-5.0f, 2.0f),
    glm::vec3(2.0f,-5.0f,  -2.0f),
    glm::vec3(-2.0f,-5.0f, 2.0f)
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

    // Carga de modelos
    Model Piso((char*)"Models/Minecraft/tablero2.obj");
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
     // Coloca las piezas en sus posiciones iniciales y les asigna sus modelos
    InitializeBoard(
        &pollo, &golem, &caballo, &perro, &alex, &steve,
        &esqueleto, &piglin, &blaze, &enderman, &dragon, &warden
    );

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
        UpdateAnimations(deltaTime);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        lightingShader.Use();
        glUniform1i(glGetUniformLocation(lightingShader.Program, "diffuse"), 0);
        // Obtener la matriz de vista de la cámara activa
        glm::mat4 view;
        if (useSideCamera) {
            view = cameraSideView.GetViewMatrix();
        }
        else {
            view = camera.GetViewMatrix();
        }
        GLint modelLoc = glGetUniformLocation(lightingShader.Program, "model");
        GLint viewLoc = glGetUniformLocation(lightingShader.Program, "view");
        GLint projLoc = glGetUniformLocation(lightingShader.Program, "projection");

        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // Dibujar el tablero
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::scale(model, glm::vec3(82.0f, 75.0f, 84.0f));
        model = glm::translate(model, glm::vec3(0.0f, -0.054f, 0.25f));
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

                    // Aplicar offset de animación si la pieza se está moviendo
                    if (piece.isMoving) {
                        worldPos += piece.positionOffset;
                    }
                    else {
                        worldPos += piece.positionOffset; // Para otros offsets estáticos
                    }

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
        glfwSwapBuffers(window);
    }
    glfwTerminate();
    return 0;
}

void UpdateAnimations(float deltaTime) {
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            ChessPiece& piece = board[r][c];
            if (piece.isMoving) {
                float staticOffsetY = piece.positionOffset.y; // Guarda el Y actual (debería ser el estático)
                piece.moveProgress += piece.moveSpeed * deltaTime;

                if (piece.moveProgress >= 1.0f) {
                    piece.moveProgress = 1.0f; // Asegura que termine exactamente en 1.0
                    piece.isMoving = false;
                    // Al terminar, restablece SOLO los offsets X y Z de la animación a 0,
                    // manteniendo el offset Y estático que ya estaba.
                    piece.positionOffset.x = 0.0f;
                    piece.positionOffset.z = 0.0f;

                }
                else {
                    // Interpolación lineal entre startPos y targetPos
                    float easedProgress = piece.moveProgress * piece.moveProgress * (3.0f - 2.0f * piece.moveProgress); // Easing
                    glm::vec3 currentPos = piece.startPos + (piece.targetPos - piece.startPos) * easedProgress;

                    // Calcula la posición lógica de destino (donde la pieza "debería" estar sin offsets)
                    glm::vec3 targetLogicalPos = GetWorldCoordinates(piece.row, piece.col);

                    // Calcula el offset de animación SOLO para X y Z
                    piece.positionOffset.x = currentPos.x - targetLogicalPos.x;
                    piece.positionOffset.z = currentPos.z - targetLogicalPos.z;

                }
            }
        }
    }
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
/**
 * @brief Callback para eventos de clic del ratón.
 * Gestiona la selección de piezas, validación de movimientos y ejecución de movimientos/capturas.
 * @param window Puntero a la ventana GLFW.
 * @param button Botón del ratón presionado.
 * @param action Acción (GLFW_PRESS o GLFW_RELEASE).
 * @param mods Modificadores de teclado (Shift, Ctrl, etc.).
 */
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        double xpos, ypos; // Obtener posición actual del cursor
        glfwGetCursorPos(window, &xpos, &ypos);
	// Determinar qué cámara se está usando para el raycasting
        const Camera& currentActiveCamera = useSideCamera ? cameraSideView : camera;
        glm::vec3 intersectionPoint = GetBoardIntersectionPoint(window, xpos, ypos, camera);
        
        // Verificar si la intersección es válida (GetBoardIntersectionPoint devuelve -999.0f en Y si no)
        if (intersectionPoint.y > -900.0f) {
            int targetRow, targetCol;
            if (WorldToBoardCoordinates(intersectionPoint, targetRow, targetCol)) {
                std::cout << "Clicked on board at: (" << targetRow << "," << targetCol << ")" << std::endl;
                ChessPiece& clickedPiece = board[targetRow][targetCol];
                 // ... Lógica de selección y movimiento ...
                // Caso 1: Ninguna pieza está seleccionada actualmente.
                // Caso 2: Ya hay una pieza seleccionada. Evaluar movimiento.
                         // Subcaso 2.1: El movimiento es válido.
                        // Subcaso 2.2: El movimiento no es válido. ¿Se seleccionó otra pieza propia?
                if (selectedPiece == nullptr) { // Si no hay ninguna pieza seleccionada
                    // Intentar seleccionar la pieza en la casilla clickeada
                    if (clickedPiece.type != EMPTY && clickedPiece.color == currentPlayer) {
                        clickedPiece.isSelected = true;
                        selectedPiece = &clickedPiece;
                        selectedRow = targetRow;
                        selectedCol = targetCol;
                    }
                }
                else {
                    // Dentro del if (IsValidMove(...)) en MouseButtonCallback:
                    // Ya hay una pieza seleccionada, intentar moverla o cambiar selección
                    if (IsValidMove(selectedPiece, targetRow, targetCol)) {
                        selectedPiece->isSelected = false;
                        ChessPiece& targetSquare = board[targetRow][targetCol];
                        // Si hay una pieza enemiga en la casilla destino, capturarla
                        if (targetSquare.type != EMPTY) {
                            MoveCapturedPiece(targetSquare); // Mueve la pieza *antes* de sobrescribirla
                        }

                        // 1. Mover la pieza seleccionada a la casilla destino
                        ChessPiece pieceToMove = *selectedPiece; // Crea una copia temporal
                        pieceToMove.row = targetRow;
                        pieceToMove.col = targetCol;
                        pieceToMove.isSelected = false; // Ya no está seleccionada

                        // 2. Vacía la casilla original LÓGICA
                        board[selectedRow][selectedCol] = ChessPiece();
                        board[selectedRow][selectedCol].row = selectedRow;
                        board[selectedRow][selectedCol].col = selectedCol;

                        // 3. Coloca la pieza copiada en la casilla destino LÓGICA
                        board[targetRow][targetCol] = pieceToMove;

                        // 4. Configura la animación en la pieza que AHORA está en la casilla destino
                        ChessPiece& movingPiece = board[targetRow][targetCol]; // Obtén referencia a la pieza movida
                        movingPiece.isMoving = true;
                        movingPiece.startPos = GetWorldCoordinates(selectedRow, selectedCol); // Posición inicial ANTES del movimiento
                        movingPiece.targetPos = GetWorldCoordinates(targetRow, targetCol);   // Posición final
                        movingPiece.moveProgress = 0.0f;
                        // movingPiece.moveSpeed = 1.0f; // <-- AJUSTA ESTE VALOR si 0.2f es muy rápido o lento una vez funcione

                        // Deseleccionar
                        selectedPiece = nullptr;
                        selectedRow = -1;
                        selectedCol = -1;

                        // CAMBIO DE TURNO (ver siguiente punto)
                        currentPlayer = (currentPlayer == WHITE) ? BLACK : WHITE; 
                    }
                   // Limpiar el estado de selección
                    else {// El movimiento no es válido
                        // Si se hizo clic en otra pieza del mismo jugador, cambiar la selección
                        if (board[targetRow][targetCol].type != EMPTY &&
                            board[targetRow][targetCol].color == currentPlayer) {
                            selectedPiece->isSelected = false;
                            selectedPiece = &board[targetRow][targetCol];
                            selectedPiece->isSelected = true;
                            selectedRow = targetRow;
                            selectedCol = targetCol;
                        }
                        else {  // Clic en casilla enemiga (no válida para mover) o vacía (no válida para mover)
                            selectedPiece->isSelected = false; // Deseleccionar la pieza actual
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
void DoMovement() {
    // Camera controls (solo para la cámara principal)
    if (!useSideCamera) {
        if (keys[GLFW_KEY_W] || keys[GLFW_KEY_UP]) {
            camera.ProcessKeyboard(FORWARD, deltaTime);
        }
        if (keys[GLFW_KEY_S] || keys[GLFW_KEY_DOWN]) {
            camera.ProcessKeyboard(BACKWARD, deltaTime);
        }
        if (keys[GLFW_KEY_A] || keys[GLFW_KEY_LEFT]) {
            camera.ProcessKeyboard(LEFT, deltaTime);
        }
        if (keys[GLFW_KEY_D] || keys[GLFW_KEY_RIGHT]) {
            camera.ProcessKeyboard(RIGHT, deltaTime);
        }
    }
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode) { 
	if (GLFW_KEY_ESCAPE == key && GLFW_PRESS == action) {
		glfwSetWindowShouldClose(window, GL_TRUE);
	}

	if (key >= 0 && key < 1024) {
		if (action == GLFW_PRESS) {
			keys[key] = true;
			// Alternar cámaras
			if (key == GLFW_KEY_1) {
				useSideCamera = false;  // Cámara principal
			}
			if (key == GLFW_KEY_2) {
				useSideCamera = true;   // Cámara lateral
			}
		}
		else if (action == GLFW_RELEASE) {
			keys[key] = false;
		}
	}
}

/**
 * @brief Callback para movimiento del cursor del ratón.
 * Controla la orientación de la cámara principal.
 * @param window Puntero a la ventana GLFW.
 * @param xPos Nueva posición X del cursor.
 * @param yPos Nueva posición Y del cursor.
 */
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


// --- Anadido: Funcion auxiliar para verificar caminos ---
// Verifica si el camino entre (startRow, startCol) y (endRow, endCol) esta vacio.
// NO verifica la validez del movimiento en si (horizontal, vertical, diagonal)
// Asume que el movimiento es en linea recta (horizontal, vertical o diagonal perfecta).
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

	// Si el bucle termina, significa que todas las casillas intermedias estaban vacias
	return true; // Camino despejado
}


// --- ACTUALIZADA: Validacion de Movimientos de Ajedrez (Reglas B�sicas) ---
bool IsValidMove(ChessPiece* piece, int targetRow, int targetCol) {
	// 1. Chequeos Iniciales Basicos
	if (!piece || piece->type == EMPTY) {
		std::cerr << "Error IsValidMove: Pieza inv�lida o vac�a." << std::endl;
		return false;
	}
	if (targetRow < 0 || targetRow >= 8 || targetCol < 0 || targetCol >= 8) {
		// std::cout << "Movimiento invalido: Fuera del tablero." << std::endl; // Mucho spam
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
		// std::cout << "Movimiento invalido: No puedes capturar tu propia pieza." << std::endl;
		return false;
	}

	// 2. Logica Especifica por Tipo de Pieza
	switch (piece->type) {
	case PAWN: {
		int forward = (piece->color == WHITE) ? 1 : -1; // Direccion de avance
		// Mover 1 casilla adelante
		if (targetCol == startCol && targetRow == startRow + forward && targetSquare.type == EMPTY) {
			return true;
		}
		// Mover 2 casillas adelante (solo desde posicion inicial)
		bool isStartingRow = (piece->color == WHITE && startRow == 1) || (piece->color == BLACK && startRow == 6);
		if (isStartingRow && targetCol == startCol && targetRow == startRow + 2 * forward && targetSquare.type == EMPTY) {
			// Verificar que la casilla intermedia tambien esta vacia
			if (board[startRow + forward][startCol].type == EMPTY) {
				return true;
			}
		}
		// Captura diagonal
		if (std::abs(targetCol - startCol) == 1 && targetRow == startRow + forward && targetSquare.type != EMPTY && targetSquare.color != piece->color) {
			return true;
		}
		// Faltan: En Passant, Promocion
		return false; // Si no es ninguno de los anteriores, es invalido para el peon
	}

	case ROOK: {
		// Debe ser movimiento horizontal o vertical
		if (startRow != targetRow && startCol != targetCol) {
			return false; // No es movimiento de torre
		}
		// Verificar que el camino esta despejado
		return IsPathClear(startRow, startCol, targetRow, targetCol);
	}

	case KNIGHT: {
		int dRow = std::abs(targetRow - startRow);
		int dCol = std::abs(targetCol - startCol);
		// Movimiento en 'L' (2 en una direccion, 1 en la perpendicular)
		return (dRow == 2 && dCol == 1) || (dRow == 1 && dCol == 2);
		// El caballo salta, no necesita IsPathClear
	}

	case BISHOP: {
		// Debe ser movimiento diagonal
		if (std::abs(targetRow - startRow) != std::abs(targetCol - startCol)) {
			return false; // No es movimiento de alfil
		}
		// Verificar que el camino esta despejado
		return IsPathClear(startRow, startCol, targetRow, targetCol);
	}

	case QUEEN: {
		// Debe ser movimiento horizontal, vertical o diagonal
		bool isStraight = (startRow == targetRow || startCol == targetCol);
		bool isDiagonal = (std::abs(targetRow - startRow) == std::abs(targetCol - startCol));
		if (!isStraight && !isDiagonal) {
			return false; // No es movimiento de reina
		}
		// Verificar que el camino esta despejado
		return IsPathClear(startRow, startCol, targetRow, targetCol);
	}

	case KING: {
		int dRow = std::abs(targetRow - startRow);
		int dCol = std::abs(targetCol - startCol);
		// Mover solo 1 casilla en cualquier direccion
		// Faltan: Enroque, Chequeo de Jaque
		return dRow <= 1 && dCol <= 1;
	}

	case EMPTY:
	default:
		std::cerr << "Error IsValidMove: Tipo de pieza desconocido o EMPTY." << std::endl;
		return false;
	}
}


// Funcion para intentar convertir coordenadas del mundo a tablero
// Asume que el eje Y es la altura y que el tablero este en el plano XZ
bool WorldToBoardCoordinates(const glm::vec3& worldPos, int& row, int& col) {
	// Calcula columna basada en X
	float boardX = worldPos.x - BOARD_OFFSET_X;
	col = static_cast<int>(floor(boardX / TILE_SIZE));

	// Calcula fila basada en Z
	float boardZ = worldPos.z - BOARD_OFFSET_Z;
	row = static_cast<int>(floor(boardZ / TILE_SIZE));

	// Verifica si esta dentro de los limites del tablero
	if (row >= 0 && row < 8 && col >= 0 && col < 8) {
		return true;
	}
	else {
		row = -1; // Inv�lido
		col = -1; // Inv�lido
		return false;
	}
}

// --- Anadido: Funciones de Raycasting ---

// Calcula la direccion del rayo en coordenadas del MUNDO desde la posici�n del mouse
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

	// 2. Coordenadas de Clip (Homogeneas)
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
