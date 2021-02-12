/*
	COSC 3P98 - Term Project
	@author Tennyson Demchuk | 6190532 | td16qg@brocku.ca
	@author 
	@author 
	@date 02.08.2021
*/

/*
	Basic setup taken from https://learnopengl.com/
	Please read instructions.txt for basic setup checklist to ensure proper
	execution of the project within Visual Studio
*/

#include "shader.h"			// shader loading library - https://learnopengl.com/code_viewer_gh.php?code=includes/learnopengl/shader.h
#include "testcamera.h"		// test camera - MUST BE REPLACED W/ CUSTOM FLIGHTSIM CAM USING QUATERNIONS
#include "terrain.h"
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
void mouse_callback(GLFWwindow* window, double x, double y);
void window_resize_callback(GLFWwindow* window, int w, int h);
							// helper funcs
void randomizeTerrain();


// glob vars
#define DEFAULT_WIDTH 500
#define DEFAULT_HEIGHT 500
unsigned int width, height;

float deltatime = 0;
float lastframe = 0;

TestCamera cam(glm::vec3(0, 5, 17));
TerrainChunk tc(64);
//Terrain t;

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

	// enable gl options
	glEnable(GL_DEPTH_TEST);		// enable depth testing
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// init shader
	Shader shader("shaders/basic.vs", "shaders/basic.fs");

	// init terrain
	//tc.applyRandomHeightmap();
	tc.applySinusoidalHeightmap();
	//tc.computeFaceNormals();						// mathematically "correct" method is broken atm, use approximation 
	//tc.computeAngleWeightedSmoothNormals();
	tc.computeSmoothNormalsApproximation();

	// init test cube
	glm::mat4 cube_model = glm::mat4(1.0f);
	cube_model = glm::translate(glm::rotate(glm::scale(cube_model, glm::vec3(2, 2, 2)),glm::radians(15.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::vec3(0.0f, 2.0f, 0.0f));
	float cube_vertices[] = {
	-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
	 0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
	 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
	 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
	-0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
	-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

	-0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
	 0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
	 0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
	 0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
	-0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
	-0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,

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

	// init terrain VAO's, VBO's, EBO's
	glGenVertexArrays(1, &tc.VAO);
	glGenBuffers(1, &tc.VBO);
	glGenBuffers(1, &tc.EBO);

	glBindVertexArray(tc.VAO);				// first bind VAO, then we must bind and set vertex buffers, then configure vertex attributes
	glBindBuffer(GL_ARRAY_BUFFER, tc.VBO);
	glBufferData(GL_ARRAY_BUFFER, tc.vertices_size, tc.vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tc.EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, tc.indices_size, tc.indices, GL_STATIC_DRAW);

	/*
		Vertex Attrib Pointers
		- Enable vertex attrib array by passing in it's unique index to glEnableVertexAttribArray
		- create vertex attrib pointers with:
			glVertexAttribPointer(uint index, int size, GLenum type, bool normalized, int stride, const void* ptr)
			- index = unique index of vertex attribute to be modified
			- size	= # components per vertex attributute (ie. dimensionality of vertex data, can be 1,2,3, or 4)
			- type	= data type of each vector components, usually GL_FLOAT but can also be GL_DOUBLE, GL_INT, GL_BYTE, etc...
			- normalized = whether fixed-point data values should be normalized (GL_TRUE) or converted directly as fixed point vals when accessed
			- stride = The byte offset between consecutive generic vertex attributes. If stride is 0, the vertex attributes are interpreted as tightly packed
			- pointer = offset of the first component of the first vertex attribute in the array in the data store of the buffer currently bound to GL_ARRAY_BUFFER, initially 0.
	*/
	glEnableVertexAttribArray(0);			// position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);			// normal attribute
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glBindBuffer(GL_ARRAY_BUFFER, 0);		// unbind (legal but not necessary)
	
	// init cube VAO's, VBO's, EBO's
	unsigned int cubeVBO, cubeVAO;
	glGenVertexArrays(1, &cubeVAO);
	glGenBuffers(1, &cubeVBO);

	glBindVertexArray(cubeVAO);
	glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);	// unbind (legal but not necessary)

	// setup perspective projection
	glm::mat4 view = glm::mat4(1.0f);
	glm::vec3 viewpos(0, 5, 17);					// inverse of view matrix translation
	view = glm::translate(glm::rotate(view, glm::radians(10.0f), glm::vec3(1.0, 0, 0)), glm::vec3(0, -5, -17));		// translate the scene in the reverse direction of the way we want to move
	glm::mat4 proj = glm::perspective(glm::radians(cam.Zoom), (float)width/(float)height, 0.1f, 100.0f);
	glm::mat3 norm;									// normal matrix - https://www.lighthouse3d.com/tutorials/glsl-12-tutorial/the-normal-matrix/
	shader.use();
	shader.setMat4("proj", proj);

	// setup lighting
	glm::vec3 lightpos(0.0f, 2.0f, 0.0f);		// 2 units above the origin
	shader.setVec3("lightcolor", 1.0f, 1.0f, 1.0f);
	shader.setVec3("lightpos", lightpos);

	// render loop
	while (!glfwWindowShouldClose(window)) {	
										// time logic
		float currentFrame = (float)glfwGetTime();
		deltatime = currentFrame - lastframe;
		lastframe = currentFrame;

		keyboard_input(window);			// get keyboard input

		glClearColor(0.443f, 0.560f, 0.756f, 1.0f);	// RGBA
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		shader.use();		// use main shader
		view = cam.GetViewMatrix();
		viewpos = cam.Position;
		shader.setMat4("view", view);
		shader.setVec3("viewpos", viewpos);

		glBindVertexArray(tc.VAO);		// draw terrain
		norm = glm::mat3(glm::transpose(glm::inverse(tc.model)));
		shader.setMat3("normalMatrix", norm);
		shader.setMat4("model", tc.model);
		shader.setVec3("objcolor", tc.color.x, tc.color.y, tc.color.z);
		glDrawElements(GL_TRIANGLES, tc.indices_size, GL_UNSIGNED_INT, 0);

		glBindVertexArray(cubeVAO);		// draw cube
		norm = glm::mat3(glm::transpose(glm::inverse(cube_model)));
		shader.setMat3("normalMatrix", norm);
		shader.setMat4("model", cube_model);
		shader.setVec3("objcolor", 1.0f, 1.0f, 1.0f);
		glDrawArrays(GL_TRIANGLES, 0, 36);

		glfwSwapBuffers(window);				
		glfwPollEvents();
	}

	// perform cleanup and exit
	glDeleteVertexArrays(1, &tc.VAO);
	glDeleteBuffers(1, &tc.VBO);
	glDeleteBuffers(1, &tc.EBO);
	glDeleteVertexArrays(1, &cubeVAO);
	glDeleteBuffers(1, &cubeVBO);
	glfwTerminate();
	return 0;
}

// terminates the GLFW context and exits the program
void terminateProgram() {
	glfwTerminate();
	exit(EXIT_FAILURE);
}

// Applies a randomly generated heightmap to a terrain object and re-uploads to graphics card
// FOR DEMO-PURPOSES ONLY
void randomizeTerrain() {
	//srand(time(0));								// seed rng and randomly modify terrain
	//for (int i = 1; i < t.vertices_num_elements; i += 3) {
	//	t.vertices[i] = ((float)(rand() % 101) - 50) / 130.0f;
	//}
	//Terrain::smoothNormals(&t);

	//glBindVertexArray(t.VAO);
	//glBindBuffer(GL_ARRAY_BUFFER, t.VBO);
	//glBufferData(GL_ARRAY_BUFFER, t.vertices_size, t.vertices, GL_STATIC_DRAW);		// upload modified data
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
												// options
	else if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) randomizeTerrain();
	// controls
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) cam.ProcessKeyboard(FORWARD, deltatime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) cam.ProcessKeyboard(BACKWARD, deltatime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) cam.ProcessKeyboard(LEFT, deltatime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) cam.ProcessKeyboard(RIGHT, deltatime);
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) cam.ProcessKeyboard(UP, deltatime);
	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) cam.ProcessKeyboard(DOWN, deltatime);
}

void mouse_callback(GLFWwindow* window, double x, double y) {
	static bool first = true;
	static float lastx = width / 2.0f;
	static float lasty = height / 2.0f;
	if (first) {
		lastx = (float)x;
		lasty = (float)y;
		first = false;
	}
	float xoffset = (float)x - lastx;
	float yoffset = lasty - (float)y;		// y coord reversed
	lastx = (float)x;
	lasty = (float)y;
	cam.ProcessMouseMovement(xoffset, yoffset);
}

void window_resize_callback(GLFWwindow* window, int w, int h) {
	width = w;
	height = h;
	glViewport(0, 0, width, height);
}

// *************************************