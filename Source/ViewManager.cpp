///////////////////////////////////////////////////////////////////////////////
// viewmanager.cpp
// ================
// Implements the `ViewManager` class, which manages the camera, projection, 
// and viewport settings for rendering 3D objects. Handles user input for 
// camera movement and updates the view and projection matrices.
//
// AUTHOR: Brian Battersby
// INSTITUTION: Southern New Hampshire University (SNHU)
// COURSE: CS-330 Computational Graphics and Visualization
//
// INITIAL VERSION: November 1, 2023
// LAST REVISED: December 1, 2024
//
// RESPONSIBILITIES:
// - Manage the 3D camera, including position, orientation, and zoom.
// - Handle user input for mouse and keyboard interactions.
// - Set up the main display window using GLFW.
// - Configure projection settings for both perspective and orthographic views.
// - Update the view and projection matrices for rendering.
//
// NOTE: This implementation uses GLFW for window management and input, and 
// GLM for mathematical operations like matrix transformations.
///////////////////////////////////////////////////////////////////////////////

#include "ViewManager.h"

// GLM Math Header inclusions
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>    

// declaration of the global variables and defines
namespace
{
	// Variables for window width and height
	const int WINDOW_WIDTH = 1000;
	const int WINDOW_HEIGHT = 800;
	const char* g_ViewName = "view";
	const char* g_ProjectionName = "projection";

	// camera object used for viewing and interacting with
	// the 3D scene
	Camera* g_pCamera = nullptr;

	// these variables are used for mouse movement processing
	float gLastX = WINDOW_WIDTH / 2.0f;
	float gLastY = WINDOW_HEIGHT / 2.0f;
	bool gFirstMouse = true;

	// time between current frame and last frame
	float gDeltaTime = 0.0f;
	float gLastFrame = 0.0f;

	// if orthographic projection is on, this value will be
	// true
	bool bOrthographicProjection = false;
}


static void Scroll_Callback(GLFWwindow* window, double xoffset, double yoffset) //callback function for mouse scroll events.
{
	if (!g_pCamera) // if the camera object is null, then exit this method.
		return;

	// Adjust movement speed by the scroll amount.
	float newSpeed = g_pCamera->MovementSpeed + static_cast<float>(yoffset); // "yoffset" is the scroll amount.
	if (newSpeed < 1.0f)      newSpeed = 1.0f;		//	
	if (newSpeed > 50.0f)     newSpeed = 50.0f; // To ensure speed is within a reasonable range.
	g_pCamera->MovementSpeed = newSpeed;
}
/***********************************************************
 *  ViewManager()
 *
 *  The constructor for the class
 ***********************************************************/
ViewManager::ViewManager(
	ShaderManager* pShaderManager)
{
	// initialize the member variables
	m_pShaderManager = pShaderManager;
	m_pWindow = NULL;
	g_pCamera = new Camera();
	// default camera view parameters
	g_pCamera->Position = glm::vec3(0.5f, 5.5f, 10.0f);
	g_pCamera->Front = glm::vec3(0.0f, -0.5f, -2.0f);
	g_pCamera->Up = glm::vec3(0.0f, 1.0f, 0.0f);
	g_pCamera->Zoom = 80;
	g_pCamera->MovementSpeed = 10;
}

/***********************************************************
 *  ~ViewManager()
 *
 *  The destructor for the class
 ***********************************************************/
ViewManager::~ViewManager()
{
	// free up allocated memory
	m_pShaderManager = NULL;
	m_pWindow = NULL;
	if (NULL != g_pCamera)
	{
		delete g_pCamera;
		g_pCamera = NULL;
	}
}

/***********************************************************
 *  CreateDisplayWindow()
 *
 *  This method is used to create the main display window.
 ***********************************************************/
GLFWwindow* ViewManager::CreateDisplayWindow(const char* windowTitle)
{
	GLFWwindow* window = nullptr;

	// try to create the displayed OpenGL window
	window = glfwCreateWindow(
		WINDOW_WIDTH,
		WINDOW_HEIGHT,
		windowTitle,
		NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return NULL;
	}
	glfwMakeContextCurrent(window);

	// this callback is used to receive mouse moving events
	glfwSetCursorPosCallback(window, &ViewManager::Mouse_Position_Callback); \
		// this callback is used to receive mouse scroll events.
		glfwSetScrollCallback(window, Scroll_Callback);

	// tell GLFW to capture all mouse events
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// enable blending for supporting tranparent rendering
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	m_pWindow = window;

	return(window);
}

/***********************************************************
 *  Mouse_Position_Callback()
 *
 *  This method is automatically called from GLFW whenever
 *  the mouse is moved within the active GLFW display window.
 ***********************************************************/
void ViewManager::Mouse_Position_Callback(GLFWwindow* window, double xMousePos, double yMousePos)
{
	// when the first mouse move event is received, this needs to be recorded so that
	// all subsequent mouse moves can correctly calculate the X position offset and Y
	// position offset for proper operation
	if (gFirstMouse)
	{
		gLastX = xMousePos;
		gLastY = yMousePos;
		gFirstMouse = false;
	}

	// calculate the X offset and Y offset values for moving the 3D camera accordingly
	float xOffset = xMousePos - gLastX;
	float yOffset = gLastY - yMousePos; // reversed since y-coordinates go from bottom to top

	// set the current positions into the last position variables
	gLastX = xMousePos;
	gLastY = yMousePos;

	// move the 3D camera according to the calculated offsets
	g_pCamera->ProcessMouseMovement(xOffset, yOffset);
}

/***********************************************************
 *  ProcessKeyboardEvents()
 *
 *  This method is called to process any keyboard events
 *  that may be waiting in the event queue.
 ***********************************************************/
void ViewManager::ProcessKeyboardEvents()
{
	// close the window if the escape key has been pressed
	if (glfwGetKey(m_pWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(m_pWindow, true);
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_P) == GLFW_PRESS)
	{
		bOrthographicProjection = false;
		// When returning to perspective, you might want to allow free‐look:
		// We leave camera position/orientation as‐is, so the user can orbit normally.
	}

	if (glfwGetKey(m_pWindow, GLFW_KEY_O) == GLFW_PRESS)
	{
		bOrthographicProjection = true;
		// Snap camera so it looks directly at the object (no tilt):
		g_pCamera->Position = glm::vec3(0.0f, 0.0f, 10.0f);
		g_pCamera->Front = glm::normalize(glm::vec3(0.0f, 0.0f, -1.0f));
		g_pCamera->Up = glm::vec3(0.0f, 1.0f, 0.0f);
		g_pCamera->Yaw = -90.0f;   // Facing –Z
		g_pCamera->Pitch = 0.0f;    // Level with the "horizon"
	}
	// if the camera object is null, then exit this method
	if (NULL == g_pCamera)
	{
		return;
	}

	// process camera zooming in and out
	if (glfwGetKey(m_pWindow, GLFW_KEY_W) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(FORWARD, gDeltaTime);
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_S) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(BACKWARD, gDeltaTime);
	}

	// process camera panning left and right
	if (glfwGetKey(m_pWindow, GLFW_KEY_A) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(LEFT, gDeltaTime);
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_D) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(RIGHT, gDeltaTime);
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_Q) == GLFW_PRESS)
	{
		// Adjust camera position upward using camera's Up vector
		g_pCamera->Position += g_pCamera->Up * g_pCamera->MovementSpeed * gDeltaTime;
	}
	// Adjust camera position  downward (along world Up direction)
	if (glfwGetKey(m_pWindow, GLFW_KEY_E) == GLFW_PRESS)
	{
		g_pCamera->Position -= g_pCamera->Up * g_pCamera->MovementSpeed * gDeltaTime;
	}
}

/***********************************************************
 *  PrepareSceneView()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void ViewManager::PrepareSceneView()
{
	glm::mat4 view;
	glm::mat4 projection;

	// 1) Timing
	float currentFrame = glfwGetTime();
	gDeltaTime = currentFrame - gLastFrame;
	gLastFrame = currentFrame;

	// 2) Process keyboard events
	ProcessKeyboardEvents();

	// 3) Compute the view matrix from the camera
	view = g_pCamera->GetViewMatrix();

	// 4) Choose projection mode
	if (bOrthographicProjection)
	{

		float orthoHalfHeight = 10.0f;
		float aspect = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;
		float orthoHalfWidth = orthoHalfHeight * aspect;

		float nearPlane = 0.1f;
		float farPlane = 100.0f;
		projection = glm::ortho(
			-orthoHalfWidth,  // left
			orthoHalfWidth,  // right
			-orthoHalfHeight, // bottom
			orthoHalfHeight, // top
			nearPlane,
			farPlane
		);
	}
	else
	{
		projection = glm::perspective(
			glm::radians(g_pCamera->Zoom),
			(GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT,
			0.1f,
			100.0f
		);
	}

	// 5) Send both view and projection into the shader
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ViewName, view);
		m_pShaderManager->setMat4Value(g_ProjectionName, projection);
		// Keep passing along camera position for any lighting calculations
		m_pShaderManager->setVec3Value("viewPosition", g_pCamera->Position);
	}
}