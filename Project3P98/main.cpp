/*
	COSC 3P98 - Term Project
	@author Tennyson Demchuk | 6190532 | td16qg@brocku.ca
	@author Daniel Sokic | 6164545 | ds16sz@brocku.ca
	@author Aditya Rajyaguru | 6582282 | ar18xp@brocku.ca
	@date 02.08.2021
*/

/*
	Main  - Handles GL context creation, window management, and main render loop

	Basic setup taken from https://learnopengl.com/
	Please read instructions.txt for basic setup checklist to ensure proper
	execution of the project within Visual Studio

	Must be executed in a 64 bit Windows Environment. Ensure config is set to
	'x64 Debug' or 'x64 Release' if executing in IDE.
*/

#include "world.h"
#include "models.h"
#include "shader.h"			// shader loading library - https://learnopengl.com/code_viewer_gh.php?code=includes/learnopengl/shader.h
#include "camera.h"		    // camera - MUST BE REPLACED W/ CUSTOM FLIGHTSIM CAM USING QUATERNIONS
#include <glm/glm.hpp>		// GLM - https://glm.g-truc.net/0.9.9/index.html
#include <glm/gtc/matrix_transform.hpp>
#include <glad/glad.h>		// For GLAD - ensure to include before GLFW
#include <GLFW/glfw3.h>		// For GLFW
#include <time.h>
#include <iostream>


// function prototypes
							// system and event callbacks
void terminateProgram();
void keyboard_input(GLFWwindow* window);
void start_keyboard(GLFWwindow* window);
void end_keyboard(GLFWwindow* window);
void pause_keyboard(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double x, double y);
void window_resize_callback(GLFWwindow* window, int w, int h);


// glob vars
#define DEFAULT_WIDTH 700
#define DEFAULT_HEIGHT 700
unsigned int width, height;

float deltatime = 0;
float lastframe = 0;
float FPS = 0;

int score = 0;

Camera cam((float)width/(float)height, glm::vec3(0, 30, 0));

bool start = false;
bool end = false;
bool pause = false;


// initializes GLAD and loads OpenGL function pointers
void initGLAD() {
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		printf("GLAD initialization failed.\n");
		terminate();
	}
}

// genarate window of custom dimensions
GLFWwindow* createWindow(unsigned int w, unsigned int h) {
	GLFWwindow* window = glfwCreateWindow(w, h, "COSC 3P98 Project", nullptr, nullptr);		// create window
	if (window == NULL) {
		printf("GLFW window creation failed.\n");
		terminate();
	}
	width = w; height = h;
	glfwMakeContextCurrent(window);											// set focus																				
	glfwSetFramebufferSizeCallback(window, window_resize_callback);			// bind resize callback
	glfwSetCursorPosCallback(window, mouse_callback);						// bind mouse motion callback
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	return window;
}

// generate window of default WIDTH x HEIGHT dimensions and make current
GLFWwindow* createWindow() {
	return createWindow(DEFAULT_WIDTH, DEFAULT_HEIGHT);
}

// main func
int main(int argc, char* argv[]) {		

	// perform setup
	glfwInit();									// init GLFW and set options
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = createWindow();		// Create OpenGL window
	initGLAD();
	glfwSetWindowPos(window, 700, 100);

	// enable gl options
	glEnable(GL_DEPTH_TEST);		// enable depth testing
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	World w(cam);

	// FPS calculation via simple moving average - https://stackoverflow.com/a/87732
	constexpr int SAMPLES = 50;
	int frameIndex = 0;
	float fpsSum = 0;
	float fpsSamples[SAMPLES];
	for (int i = 0; i < SAMPLES; i++) fpsSamples[i] = 0;
	float currentFrame;

	printf("CONTROLS:\nLEFT SHIFT:Thrust Forward\nP:Pause \nU:Unpause\nW:Pitch Up\nS:Pitch Down\nA:Yaw Left\nD:Yaw Right\nQ:Roll Left\nE:Roll Right\n");
	printf("PRESS THE LEFT SHIFT KEY TO START!\n");
	// render loop
	while (!glfwWindowShouldClose(window)) {	
										// time logic
		currentFrame = (float)glfwGetTime();
		deltatime = currentFrame - lastframe;
		lastframe = currentFrame;
										// compute FPS
		fpsSum -= fpsSamples[frameIndex];
		fpsSum += deltatime;
		fpsSamples[frameIndex] = deltatime;
		FPS = (float)SAMPLES / fpsSum;			// compute # frame samples / total time (in seconds)
		frameIndex = (frameIndex + 1) % SAMPLES;

		glClearColor(0.443f, 0.560f, 0.756f, 1.0f);	// RGBA
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// update and draw world
		w.update(deltatime);

		if (!pause) {
			keyboard_input(window);			// get keyboard input

			// update camera by applying gravity
			if (start) {
				cam.applyGravity(deltatime);
				std::cout << "\x1b[A";
				std::cout << "Score: " << score << "\n";
			}
		}
		if(pause) {
			glfwSetCursorPosCallback(window, nullptr);
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			pause_keyboard(window);
		}

		glfwSwapBuffers(window);				
		glfwPollEvents();

		if (cam.camPos.y > 30) {
			cam.camPos.y = 30;
		}

		if (cam.camPos.y < -10) {
			end = true;
			printf("TOO LOW, YOU LOSE! PRESS ESCAPE TO EXIT!\n");
			while (end) {
				end_keyboard(window);
				glfwPollEvents();
			}
		}

	}
	// perform cleanup and exit
	glfwTerminate();
	return 0;
}

// terminates the GLFW context and exits the program
void terminateProgram() {
	glfwTerminate();
	exit(EXIT_FAILURE);
}


/*
   *------------------*
	CALLBACK FUNCTIONS
   *------------------*
*/// ***********************************

void keyboard_input(GLFWwindow* window) {		// not technically a "callback", rather is called every frame
												// system and render mode input
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);
	else if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);	// wireframe
	else if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);	// full

	// controls
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) cam.processKeyControls(PITCHDOWN, deltatime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) cam.processKeyControls(PITCHUP, deltatime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) cam.processKeyControls(YAWLEFT, deltatime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) cam.processKeyControls(YAWRIGHT, deltatime);
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) cam.processKeyControls(ROLLLEFT, deltatime);
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) cam.processKeyControls(ROLLRIGHT, deltatime);
	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_RELEASE) cam.processKeyControls(ENDTHRUST, deltatime);
	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
		if (!start) {
			start = true;
			printf("GAME HAS STARTED!\n");
			std::cout << "Score: " << score << "\n";
		}
		cam.processKeyControls(STARTTHRUST, deltatime);
	}

	if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
		printf("FPS: %.1f.\n", FPS);
		printf("GAME PAUSED! PRESS U to unpause\n");
		std::cout << "Score: " << score << "\n";
		pause = true;
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}
}

void end_keyboard(GLFWwindow* window) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		end = false;
		glfwSetWindowShouldClose(window, true);
	}
}

void pause_keyboard(GLFWwindow* window) {
	if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		pause = false;
		glfwSetCursorPosCallback(window, mouse_callback);
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		printf("GAME UNPAUSED!\n");
		std::cout << "Score: " << score << "\n";
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	}
}

// runs when the mouse is moved - manuipulates camera control
void mouse_callback(GLFWwindow* window, double x, double y) {
	if (start && !pause) {
		static bool firstmove = true;
		static float lastx = width / 2.0f;
		static float lasty = height / 2.0f;
		if (firstmove) {
			lastx = (float)x;
			lasty = (float)y;
			firstmove = false;
		}
		float xoffset = (float)x - lastx;
		float yoffset = lasty - (float)y;		// y coord reversed
		lastx = (float)x;
		lasty = (float)y;
		cam.processMouseControls(xoffset, yoffset);
	}
}

void window_resize_callback(GLFWwindow* window, int w, int h) {
	width = w;
	height = h;
	glViewport(0, 0, width, height);
}

// *************************************