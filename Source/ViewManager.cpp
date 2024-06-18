///////////////////////////////////////////////////////////////////////////////
// viewmanager.cpp
// ============
// manage the viewing of 3D objects within the viewport - camera, projection
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
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
	float m_scalar = 1.0f;

	// A variable that scales the speed of the camera panning by being multiplied with gDeltaTime
	float p_scalar = 1.0f;

	// time between current frame and last frame
	float gDeltaTime = 0.0f; 
	float gLastFrame = 0.0f;

	// the following variable is false when orthographic projection
	// is off and true when it is on
	bool bOrthographicProjection = false;

	// Orthographic camera movement will rotate about the world origin, and so a different camera position will be tracked
	// It will be initialized to the same default as the perspective camera
	glm::vec3 o_camera_position = glm::vec3(0.0f, 5.0f, 12.0f);
	// The distance from which the ortho camera will pivot
	float o_radius = 10.0f;
	// The degree of our camera vector component that is ortholinear to the y axis
	float o_yaw = 0.0f;
	// The degree of our camera vector component that is ortholinear to the x axis
	float o_pitch = 0.0f;
	// Adjust camera movement speed for ortho projection
	float o_scalar = 5.0f;

	
	// Alternative keyboard layouts will be changed by setting proxy values like below
	bool bcolemak_layout = false;
	int gkey_forward = GLFW_KEY_W;
	int gkey_backward = GLFW_KEY_S;
	int gkey_left = GLFW_KEY_A;
	int gkey_right = GLFW_KEY_D;
	int gkey_up = GLFW_KEY_Q;
	int gkey_down = GLFW_KEY_E;
	int gkey_layout = GLFW_KEY_C;
}

/***********************************************************
 *  ViewManager()
 *
 *  The constructor for the class
 ***********************************************************/
ViewManager::ViewManager(
	ShaderManager *pShaderManager)
{
	// initialize the member variables
	m_pShaderManager = pShaderManager;
	m_pWindow = NULL;
	g_pCamera = new Camera();
	// default camera view parameters
	g_pCamera->Position = glm::vec3(0.0f, 5.0f, 12.0f);
	g_pCamera->Front = glm::vec3(0.0f, -0.5f, -2.0f);
	g_pCamera->Up = glm::vec3(0.0f, 1.0f, 0.0f);
	g_pCamera->Zoom = 80;
	g_pCamera->MovementSpeed = 20;
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

	// tell GLFW to capture all mouse events
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// this callback is used to receive mouse moving events
	glfwSetCursorPosCallback(window, &ViewManager::Mouse_Position_Callback);
	glfwSetScrollCallback(window, &ViewManager::Mouse_Scroll_Wheel_Callback);

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
	// Scalar is a defined global which controls speed of camera movement and is manipulated in the Mouse_Scroll_Wheel_Callback
	float xOffset = (xMousePos - gLastX) * m_scalar;
	float yOffset = (gLastY - yMousePos) * m_scalar; // reversed since y-coordinates go from bottom to top

	// set the current positions into the last position variables
	gLastX = xMousePos;
	gLastY = yMousePos;

	// move the 3D camera according to the calculated offsets
	g_pCamera->ProcessMouseMovement(xOffset, yOffset);
}

void ViewManager::Mouse_Scroll_Wheel_Callback(GLFWwindow* window, double xOffset, double yOffset)
{
	// For our purposes, we will only interpret vertical scroll
	m_scalar += yOffset;
	// Set reasonable limits for the mouse movement speed
	if (m_scalar < 0.01f)
		m_scalar = 0.01f;
	if (m_scalar > 10.0f)
		m_scalar = 10.0f;

	//We need not return a value or call a function to move the mouse, as scalar affects the calculations done in Mouse_Position_Callback
	
	// It is possilbe that the guidelines for the project intend for the scroll wheel to change camera PANNING speed
	// For this reason, we will implement this feature with horizontal scroll to preserve separate functionality in both instances
	p_scalar += xOffset;
	// Apply the same reasonable constraints as with vertical scrolling
	if (p_scalar < 0.01f)
		p_scalar = 0.01f;
	if (p_scalar > 10.0f)
		p_scalar = 10.0f;
}

/***********************************************************
 *  ProcessKeyboardEvents()
 *
 *  This method is called to process any keyboard events
 *  that may be waiting in the event queue.
 ***********************************************************/
void ViewManager::ProcessKeyboardEvents()
{
	float delta_scaled = gDeltaTime * p_scalar;
	// close the window if the escape key has been pressed
	if (glfwGetKey(m_pWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(m_pWindow, true);
	}

	// process camera zooming in and out
	if (glfwGetKey(m_pWindow, gkey_forward) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(FORWARD, delta_scaled);
		
		// Logic for the camera movement in orthographic projection
		if (bOrthographicProjection)
		{
			o_pitch += delta_scaled * o_scalar;
			if (o_pitch > 360.0f)
				o_pitch = 0.0f;
			if (o_pitch < 0.0f)
				o_pitch = 359.9f;
		}
	}
	if (glfwGetKey(m_pWindow, gkey_backward) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(BACKWARD, delta_scaled);

		// Logic for the camera movement in orthographic projection
		if (bOrthographicProjection)
		{
			o_pitch -= delta_scaled * o_scalar;
			if (o_pitch > 360.0f)
				o_pitch = 0.0f;
			if (o_pitch < 0.0f)
				o_pitch = 359.9f;
		}
	}

	// process camera panning left and right
	if (glfwGetKey(m_pWindow, gkey_left) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(LEFT, delta_scaled);

		// Logic for the camera movement in orthographic projection
		if (bOrthographicProjection)
		{
			o_yaw -= delta_scaled * o_scalar;
			if (o_yaw > 360.0f)
				o_yaw = 0.0f;
			if (o_yaw < 0.0f)
				o_yaw = 359.9f;
		}
	}
	if (glfwGetKey(m_pWindow, gkey_right) == GLFW_PRESS)
	{
		// Logic for the camera movement in orthographic projection
		if (bOrthographicProjection)
		{
			o_yaw += delta_scaled * o_scalar;
			if (o_yaw > 360.0f)
				o_yaw = 0.0f;
			if (o_yaw < 0.0f)
				o_yaw = 359.9f;
		}
		g_pCamera->ProcessKeyboard(RIGHT, delta_scaled);
	}

	// process camera moving up and down
	if (glfwGetKey(m_pWindow, gkey_up) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(UP, delta_scaled);
	}
	if (glfwGetKey(m_pWindow, gkey_down) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(DOWN, delta_scaled);
	}

	// process selection between ortholinear and perspective projections
	if (glfwGetKey(m_pWindow, GLFW_KEY_P) == GLFW_PRESS)
	{	
		// Return camera to original position
		g_pCamera->Position = glm::vec3(0.0f, 5.0f, 12.0f);
		g_pCamera->Front = glm::vec3(0.0f, -0.5f, -2.0f);
		g_pCamera->Up = glm::vec3(0.0f, 1.0f, 0.0f);

		bOrthographicProjection = false;
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_O) == GLFW_PRESS)
	{
		// Return camera to default location
		o_camera_position = glm::vec3(0.0f, 5.0f, 12.0f);
		o_yaw = 0.0f;
		o_pitch = 0.0f;

		bOrthographicProjection = true;
	}

	// process selection of keyboard layout
	if (glfwGetKey(m_pWindow, gkey_layout) == GLFW_PRESS)
	{
		if (bcolemak_layout) {
			gkey_forward = GLFW_KEY_W;
			gkey_backward = GLFW_KEY_S;
			gkey_left = GLFW_KEY_A;
			gkey_right = GLFW_KEY_D;
			gkey_up = GLFW_KEY_Q;
			gkey_down = GLFW_KEY_E;
			gkey_layout = GLFW_KEY_C;

			bcolemak_layout = false;
		}
		else {
			gkey_forward = GLFW_KEY_F;
			gkey_backward = GLFW_KEY_S;
			gkey_left = GLFW_KEY_R;
			gkey_right = GLFW_KEY_T;
			gkey_up = GLFW_KEY_SPACE;
			gkey_down = GLFW_KEY_A;
			gkey_layout = GLFW_KEY_Q;

			bcolemak_layout = true;
		}

	}
}

/***********************************************************
 *  get_o_camera_position()
 *
 * This function is used to update the orthographic camera
 * position based on the yaw, pitch, and radius values
 ***********************************************************/
void update_o_camera_position()
{
	o_camera_position.x = cos(glm::radians(o_yaw)) * cos(glm::radians(o_pitch)) * o_radius;
	o_camera_position.y = sin(glm::radians(o_pitch)) * o_radius;
	o_camera_position.z = sin(glm::radians(o_yaw)) * cos(glm::radians(o_pitch)) * o_radius;
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

	// per-frame timing
	float currentFrame = glfwGetTime();
	gDeltaTime = currentFrame - gLastFrame;
	gLastFrame = currentFrame;

	// process any keyboard events that may be waiting in the 
	// event queue
	ProcessKeyboardEvents();

	// get the current view matrix from the camera
	view = g_pCamera->GetViewMatrix();

	// Control the size of the frustum
	float ortho_scale = 0.02f;
	// define the current projection matrix
	if (bOrthographicProjection == true) 
	{
		// If ortholinear, force the camera to look at the world origin
		update_o_camera_position();
		view = glm::lookAt(o_camera_position, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		projection = glm::ortho((GLfloat)WINDOW_WIDTH * (-1.0f) * ortho_scale, (GLfloat)WINDOW_WIDTH * ortho_scale, (GLfloat)WINDOW_HEIGHT * (-1.0f) * ortho_scale, (GLfloat)WINDOW_HEIGHT * ortho_scale, 0.1f, 100.0f);
	}else {
		projection = glm::perspective(glm::radians(g_pCamera->Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
	}

	// if the shader manager object is valid
	if (NULL != m_pShaderManager)
	{
		// set the view matrix into the shader for proper rendering
		m_pShaderManager->setMat4Value(g_ViewName, view);
		// set the view matrix into the shader for proper rendering
		m_pShaderManager->setMat4Value(g_ProjectionName, projection);
		// set the view position of the camera into the shader for proper rendering
		m_pShaderManager->setVec3Value("viewPosition", g_pCamera->Position);
	}
}
