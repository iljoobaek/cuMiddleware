// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <vector>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>
GLFWwindow* window;

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/norm.hpp>
using namespace glm;

// Include AntTweakBar
#include <AntTweakBar.h>

/*
#include <common/shader.hpp>
#include <common/texture.hpp>
#include <common/controls.hpp>
#include <common/objloader.hpp>
#include <common/vboindexer.hpp>
#include <common/quaternion_utils.hpp> // See quaternion_utils.cpp for RotationBetweenVectors, LookAt and RotateTowards
*/

#include <shader.hpp>
#include <texture.hpp>
#include <controls.hpp>
#include <objloader.hpp>
#include <vboindexer.hpp>
#include <quaternion_utils.hpp>

#include "GpuLog.hpp"
#include "TimeLog.hpp"
#include <unistd.h> // sleep()

vec3 gPosition1(-1.5f, 0.0f, 0.0f);
vec3 gOrientation1;

vec3 gPosition2( 1.5f, 0.0f, 0.0f);
quat gOrientation2;

bool gLookAtOther = true;

GLuint gProgramID;
GLuint gTexture;
GLuint gTextureID;
GLuint gLightID;

GLuint gMatrixID;
GLuint gViewMatrixID;
GLuint gModelMatrixID;

GLuint gVertexPosition_modelspaceID;
GLuint gVertexUVID;
GLuint gVertexNormal_modelspaceID;

GLuint gVertexBuffer;
GLuint gUVBuffer;
GLuint gNormalBuffer;
GLuint gElementBuffer;

unsigned int gIndexCount;

float gDeltaTime;

glm::mat4 gProjectionMatrix;
glm::mat4 gViewMatrix;

//GpuLog gGpuLog;
TimeLog timeLog;

void GpuBuffer()
{
		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Use our shader
		glUseProgram(gProgramID);

		gProjectionMatrix = glm::perspective(glm::radians(45.0f), 4.0f / 3.0f, 0.1f, 100.0f);
		gViewMatrix = glm::lookAt(
			glm::vec3( 0, 0, 7 ), // Camera is here
			glm::vec3( 0, 0, 0 ), // and looks here
			glm::vec3( 0, 1, 0 )  // Head is up (set to 0,-1,0 to look upside-down)
		);

		// Bind our texture in Texture Unit 0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gTexture);
		// Set our "myTextureSampler" sampler to use Texture Unit 0
		glUniform1i(gTextureID, 0);

		// 1rst attribute buffer : vertices
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, gVertexBuffer);
		glVertexAttribPointer(
			gVertexPosition_modelspaceID,  // The attribute we want to configure
			3,                            // size
			GL_FLOAT,                     // type
			GL_FALSE,                     // normalized?
			0,                            // stride
			(void*)0                      // array buffer offset
		);

		// 2nd attribute buffer : UVs
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, gUVBuffer);
		glVertexAttribPointer(
			gVertexUVID,                   // The attribute we want to configure
			2,                            // size : U+V => 2
			GL_FLOAT,                     // type
			GL_FALSE,                     // normalized?
			0,                            // stride
			(void*)0                      // array buffer offset
		);

		// 3rd attribute buffer : normals
		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, gNormalBuffer);
		glVertexAttribPointer(
			gVertexNormal_modelspaceID,    // The attribute we want to configure
			3,                            // size
			GL_FLOAT,                     // type
			GL_FALSE,                     // normalized?
			0,                            // stride
			(void*)0                      // array buffer offset
		);

		// Index buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gElementBuffer);

		glm::vec3 lightPos = glm::vec3(4,4,4);
		glUniform3f(gLightID, lightPos.x, lightPos.y, lightPos.z);
		usleep(5000);
}

void DrawEulerRotation()
{
        for(int i=0; i<10; i++)
		{ // Euler

			// As an example, rotate arount the vertical axis at 180 degree/sec
			gOrientation1.y += 3.14159f/2.0f * gDeltaTime;

			// Build the model matrix
			glm::mat4 RotationMatrix = eulerAngleYXZ(gOrientation1.y, gOrientation1.x, gOrientation1.z);
			glm::mat4 TranslationMatrix = translate(mat4(), gPosition1); // A bit to the left
			glm::mat4 ScalingMatrix = scale(mat4(), vec3(1.0f, 1.0f, 1.0f));
			glm::mat4 ModelMatrix = TranslationMatrix * RotationMatrix * ScalingMatrix;

			glm::mat4 MVP = gProjectionMatrix * gViewMatrix * ModelMatrix;

			// Send our transformation to the currently bound shader,
			// in the "MVP" uniform
			glUniformMatrix4fv(gMatrixID, 1, GL_FALSE, &MVP[0][0]);
			glUniformMatrix4fv(gModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
			glUniformMatrix4fv(gViewMatrixID, 1, GL_FALSE, &gViewMatrix[0][0]);



			// Draw the triangles !
			glDrawElements(
				GL_TRIANGLES,      // mode
				gIndexCount, //indices.size(),    // count
				GL_UNSIGNED_SHORT,   // type
				(void*)0           // element array buffer offset
			);
		}
}

void DrawQuaternionRotation()
{
        for(int i=0; i<10; i++)
		{ // Quaternion

			// It the box is checked...
			if (gLookAtOther){
				vec3 desiredDir = gPosition1-gPosition2;
				vec3 desiredUp = vec3(0.0f, 1.0f, 0.0f); // +Y

				// Compute the desired orientation
				quat targetOrientation = normalize(LookAt(desiredDir, desiredUp));

				// And interpolate
				gOrientation2 = RotateTowards(gOrientation2, targetOrientation, 1.0f*gDeltaTime);
			}

			glm::mat4 RotationMatrix = mat4_cast(gOrientation2);
			glm::mat4 TranslationMatrix = translate(mat4(), gPosition2); // A bit to the right
			glm::mat4 ScalingMatrix = scale(mat4(), vec3(1.0f, 1.0f, 1.0f));
			glm::mat4 ModelMatrix = TranslationMatrix * RotationMatrix * ScalingMatrix;

			glm::mat4 MVP = gProjectionMatrix * gViewMatrix * ModelMatrix;

			// Send our transformation to the currently bound shader,
			// in the "MVP" uniform
			glUniformMatrix4fv(gMatrixID, 1, GL_FALSE, &MVP[0][0]);
			glUniformMatrix4fv(gModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
			glUniformMatrix4fv(gViewMatrixID, 1, GL_FALSE, &gViewMatrix[0][0]);


			// Draw the triangles !
			glDrawElements(
				GL_TRIANGLES,      // mode
				gIndexCount, //indices.size(),    // count
				GL_UNSIGNED_SHORT,   // type
				(void*)0           // element array buffer offset
			);
		}

}

void DrawUI()
{
    for(int i=0; i<10; i++)
    {
		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);

		// Draw GUI
		TwDraw();
	}
}

void SwapFrameBuffer()
{
		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();
		usleep(5000);
}

int main( void )
{

	//gGpuLog.Initialize();

	// Initialise GLFW
	if( !glfwInit() )
	{
		fprintf( stderr, "Failed to initialize GLFW\n" );
		getchar();
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Open a window and create its OpenGL context
	window = glfwCreateWindow( 1024, 768, "Tutorial 17 - Rotations", NULL, NULL);
	if( window == NULL ){
		fprintf( stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n" );
		getchar();
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		getchar();
		glfwTerminate();
		return -1;
	}

	// Initialize the GUI
	//gGpuLog.WriteLogs(timeLog.GetTimeDiff(), "Preparation", 4);	// Diff(Program Start, Preparation)

	TwInit(TW_OPENGL_CORE, NULL);
	TwWindowSize(1024, 768);

	TwBar * EulerGUI = TwNewBar("Euler settings");
	TwBar * QuaternionGUI = TwNewBar("Quaternion settings");
	TwSetParam(EulerGUI, NULL, "refresh", TW_PARAM_CSTRING, 1, "0.1");
	TwSetParam(QuaternionGUI, NULL, "position", TW_PARAM_CSTRING, 1, "808 16");

	TwAddVarRW(EulerGUI, "Euler X", TW_TYPE_FLOAT, &gOrientation1.x, "step=0.01");
	TwAddVarRW(EulerGUI, "Euler Y", TW_TYPE_FLOAT, &gOrientation1.y, "step=0.01");
	TwAddVarRW(EulerGUI, "Euler Z", TW_TYPE_FLOAT, &gOrientation1.z, "step=0.01");
	TwAddVarRW(EulerGUI, "Pos X"  , TW_TYPE_FLOAT, &gPosition1.x, "step=0.1");
	TwAddVarRW(EulerGUI, "Pos Y"  , TW_TYPE_FLOAT, &gPosition1.y, "step=0.1");
	TwAddVarRW(EulerGUI, "Pos Z"  , TW_TYPE_FLOAT, &gPosition1.z, "step=0.1");

	TwAddVarRW(QuaternionGUI, "Quaternion", TW_TYPE_QUAT4F, &gOrientation2, "showval=true open=true ");
	TwAddVarRW(QuaternionGUI, "Use LookAt", TW_TYPE_BOOL8 , &gLookAtOther, "help='Look at the other monkey ?'");

	// Set GLFW event callbacks. I removed glfwSetWindowSizeCallback for conciseness
	glfwSetMouseButtonCallback(window, (GLFWmousebuttonfun)TwEventMouseButtonGLFW); // - Directly redirect GLFW mouse button events to AntTweakBar
	glfwSetCursorPosCallback(window, (GLFWcursorposfun)TwEventMousePosGLFW);          // - Directly redirect GLFW mouse position events to AntTweakBar
	glfwSetScrollCallback(window, (GLFWscrollfun)TwEventMouseWheelGLFW);    // - Directly redirect GLFW mouse wheel events to AntTweakBar
	glfwSetKeyCallback(window, (GLFWkeyfun)TwEventKeyGLFW);                         // - Directly redirect GLFW key events to AntTweakBar
	glfwSetCharCallback(window, (GLFWcharfun)TwEventCharGLFW);                      // - Directly redirect GLFW char events to AntTweakBar


	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    // Hide the mouse and enable unlimited mouvement
    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Set the mouse at the center of the screen
    glfwPollEvents();
    glfwSetCursorPos(window, 1024/2, 768/2);

	//gGpuLog.WriteLogs(timeLog.GetTimeDiff(), "Initialization", 4);	// Diff(Preparation, Setup Program)

	// Dark blue background
	glClearColor(0.0f, 0.0f, 0.4f, 0.0f);

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS);

	// Cull triangles which normal is not towards the camera
	glEnable(GL_CULL_FACE);

	// Create and compile our GLSL program from the shaders
//	GLuint programID = LoadShaders( "StandardShading.vertexshader", "StandardShading.fragmentshader" );
	gProgramID = LoadShaders( "StandardShading.vertexshader", "StandardShading.fragmentshader" );

	// Get a handle for our "MVP" uniform
	gMatrixID = glGetUniformLocation(gProgramID, "MVP");
	gViewMatrixID = glGetUniformLocation(gProgramID, "V");
	gModelMatrixID = glGetUniformLocation(gProgramID, "M");

	// Get a handle for our buffers
	gVertexPosition_modelspaceID = glGetAttribLocation(gProgramID, "vertexPosition_modelspace");
	gVertexUVID = glGetAttribLocation(gProgramID, "vertexUV");
	gVertexNormal_modelspaceID = glGetAttribLocation(gProgramID, "vertexNormal_modelspace");

	// Load the texture
//	GLuint Texture = loadDDS("uvmap.DDS");
	gTexture = loadDDS("uvmap.DDS");
	// Get a handle for our "myTextureSampler" uniform
//	GLuint TextureID  = glGetUniformLocation(programID, "myTextureSampler");
	gTextureID  = glGetUniformLocation(gProgramID, "myTextureSampler");

	// Read our .obj file
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> uvs;
	std::vector<glm::vec3> normals;
	bool res = loadOBJ("suzanne.obj", vertices, uvs, normals);

	std::vector<unsigned short> indices;
	std::vector<glm::vec3> indexed_vertices;
	std::vector<glm::vec2> indexed_uvs;
	std::vector<glm::vec3> indexed_normals;
	indexVBO(vertices, uvs, normals, indices, indexed_vertices, indexed_uvs, indexed_normals);

	gIndexCount = indices.size();

	// Load it into a VBO

//	GLuint gVertexBuffer;
	glGenBuffers(1, &gVertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, gVertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, indexed_vertices.size() * sizeof(glm::vec3), &indexed_vertices[0], GL_STATIC_DRAW);

//	GLuint uvbuffer;
	glGenBuffers(1, &gUVBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, gUVBuffer);
	glBufferData(GL_ARRAY_BUFFER, indexed_uvs.size() * sizeof(glm::vec2), &indexed_uvs[0], GL_STATIC_DRAW);

//	GLuint gNormalBuffer;
	glGenBuffers(1, &gNormalBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, gNormalBuffer);
	glBufferData(GL_ARRAY_BUFFER, indexed_normals.size() * sizeof(glm::vec3), &indexed_normals[0], GL_STATIC_DRAW);

	// Generate a buffer for the indices as well
//	GLuint elementbuffer;
	glGenBuffers(1, &gElementBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gElementBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned short), &indices[0] , GL_STATIC_DRAW);

	// Get a handle for our "LightPosition" uniform
	glUseProgram(gProgramID);
	gLightID = glGetUniformLocation(gProgramID, "LightPosition_worldspace");

	//gGpuLog.WriteLogs(timeLog.GetTimeDiff(), "Buffers", 4);	// Diff(Setup Program, Buffer Initialization)

	// For speed computation
	double lastTime = glfwGetTime();
	double lastFrameTime = lastTime;
	int nbFrames = 0;

    timeLog.Initialize(glfwGetTime());

	do{
        double currentTime = glfwGetTime();
		gDeltaTime = (float)(currentTime - lastFrameTime);
		lastFrameTime = currentTime;
		nbFrames++;
        if ( currentTime - lastTime >= 1.0 ){ // If last prinf() was more than 1sec ago
			// printf and reset
            printf("%f ms/frame\n", 1000.0/double(nbFrames));
			//printf("%f frames/sec\n", double(nbFrames));
			nbFrames = 0;
			lastTime += 1.0;
		}

		timeLog.CheckFrameStartTime();
		timeLog.CheckFrame10();
		timeLog.CheckNBFrame(glfwGetTime());

		//gGpuLog.WriteLogs(timeLog.GetTimeDiff(), "Start", 4);	// Diff(Buffer Initialization, Start Frame)

		GpuBuffer();

		//gGpuLog.WriteLogs(timeLog.GetTimeDiff(), "GLBuffer", 4);	// Diff(Start Frame, GL Buffer Setup)

		DrawEulerRotation();

		//gGpuLog.WriteLogs(timeLog.GetTimeDiff(), "DrawEuler", 4);	// Diff(GL Buffer Setup, Draw Half)

		DrawQuaternionRotation();

		//gGpuLog.WriteLogs(timeLog.GetTimeDiff(), "DrawQuaternion", 4);	// Diff(Draw Half, Draw Full)

		DrawUI();

		//gGpuLog.WriteLogs(timeLog.GetTimeDiff(), "DrawUI", 4);	// Diff(Draw Full, Draw UI)

		SwapFrameBuffer();

		//gGpuLog.WriteLogs(timeLog.GetTimeDiff(), "SwapFrameBuffer", 4);	// Diff(Draw Full, Swap and End of Frame)
        timeLog.CheckFrameEndTime();
	} // Check if the ESC key was pressed or the window was closed
	while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
		   glfwWindowShouldClose(window) == 0 );

    timeLog.PrintResult();

	// Cleanup VBO and shader
	glDeleteBuffers(1, &gVertexBuffer);
	glDeleteBuffers(1, &gUVBuffer);
	glDeleteBuffers(1, &gNormalBuffer);
	glDeleteBuffers(1, &gElementBuffer);
	glDeleteProgram(gProgramID);
	glDeleteTextures(1, &gTexture);

	// Close GUI and OpenGL window, and terminate GLFW
	TwTerminate();
	glfwTerminate();

	return 0;
}
