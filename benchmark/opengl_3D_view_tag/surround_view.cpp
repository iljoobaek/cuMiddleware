// Include standard headers
#include <unistd.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
using namespace std;

#define GLM_ENABLE_EXPERIMENTAL

// Include GLEW
#include <GL/glew.h>

// Include GLFW
//#include <glfw3.h>
#include <GLFW/glfw3.h>
GLFWwindow * gGLFWWindow;

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <GL/glut.h>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

// Include OpenCV
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <opencv2/imgproc/imgproc.hpp>

#include "shader.hpp"
#include "texture.hpp"
#include "objloader.hpp"

//#include "GpuQuery.h"
//#include "GpuLog.hpp"
#include "TimeLog.hpp"

// Tagging
#include <sys/types.h> // pid_t
#include "tag_state.h" // gettid()
#include "tag_dec.h" // frame_job_decorator
#include "tag_gpu.h"

//char       * gVideoCommon = "highway.mp4";
string gVideoCommon = "highway.mp4";

//char       * gVideoSource[4];
string gVideoSource[4];

//char       * gConstVideoSource[] =
string gConstVideoSource[] =
{
		// VideoResolution == -1
	    "../data/forbes_1920_1208_0.mp4",
	    "../data/forbes_1920_1208_1.mp4",
	    "../data/forbes_1920_1208_2.mp4",
	    "../data/forbes_1920_1208_3.mp4",
};

int          gVideoResolution = -1;

typedef struct _VBWheel
{
// VertexBuffer and UV Buffer
// 0 = front-left, 1 = front-right, 2 = rear-left, 3 = rear-right
	GLuint VertexBuffer;
	GLuint UVBuffer;

	vector<glm::vec3> v_wheel;
	vector<glm::vec2> u_wheel;
	vector<glm::vec3> n_wheel;

} VBWheel;

VBWheel gWheels[4];

typedef struct _WheelTranslation
{
	float Rotation;
	vec3 RotationAxis;

// 0 = front-left, 1 = front-right, 2 = rear-left, 3 = rear-right
	vec3 Centers[4];
} WheelTranslation;

WheelTranslation gWheelTranslation;

mat4 	gProjectionMatrix;
mat4  	gViewMatrix;

GLuint gProgramID;
GLuint gVertexArrayID;
GLuint gMatrixID;
GLuint gTextureID;
GLuint gVertexBuffer;
GLuint gUVBuffer;

int    gMeshIdx = 0;

vector<vec3> gCameraArray;
vector<vec3> gLookArray;

CvCapture * gCaptures[4];
IplImage  * gFrames[4];
GLuint      gTextures[4+1];	// final Texture is 'Car Texture'

// Variables
int gCurr;
int gPrev;
int change_mesh = 0;
int mesh = 0;
int video_demo = 1;
//int px2 = 0;
int px2 = 1;

vector<vector<vec3> > v(100);
vector<vector<vec2> > u(100);
vector<vector<vec3> > n(100);
vector<string> obj_names;

std::string cam_View = "S";

//GpuLog gGpuLog;
//GpuQuery * gGpuQuery;
TimeLog timeLog;
int64_t frame_period_us = 500;
int64_t deadline_us = 500;

void initializeWheels(const char * paramName, int paramBufferID)
{
	int i;

	vector<vec3> v_wheel;
	vector<vec2> u_wheel;
	vector<vec3> n_wheel;

	gWheelTranslation.Rotation = 0.0f;
	gWheelTranslation.RotationAxis = glm::vec3(1, 0, 0);
	for (i = 0; i < 4; i++)
		gWheelTranslation.Centers[i] = glm::vec3(0.0f, 0.0f, 0.0f);

//	bool res1 = loadOBJ( paramName,
	loadOBJ( paramName,
						 gWheels[paramBufferID].v_wheel,
						 gWheels[paramBufferID].u_wheel,
						 gWheels[paramBufferID].n_wheel);

//	GLuint vertexbuffer_wheel;
	glGenBuffers(1, &(gWheels[paramBufferID].VertexBuffer));
	glBindBuffer(GL_ARRAY_BUFFER, gWheels[paramBufferID].VertexBuffer);
	glBufferData(	GL_ARRAY_BUFFER,
					gWheels[paramBufferID].v_wheel.size() * sizeof(glm::vec3),
					&(gWheels[paramBufferID].v_wheel[0]), GL_STATIC_DRAW);

//	GLuint uvbuffer_wheel_fl;
	glGenBuffers(1, &(gWheels[paramBufferID].UVBuffer));
	glBindBuffer(GL_ARRAY_BUFFER, gWheels[paramBufferID].UVBuffer);
	glBufferData(GL_ARRAY_BUFFER, gWheels[paramBufferID].u_wheel.size() * sizeof(glm::vec2),
					&(gWheels[paramBufferID].u_wheel[0]), GL_STATIC_DRAW);
}

void change_view(std::string view) {
    int view_num;

    if (view.compare("W") == 0 || view.compare("E") == 0) {
        view_num = 1;
    }
    else if (view.compare("S") == 0) {
        view_num = 0;
    }

        gPrev = gCurr;
        gCurr = view_num;
}

void display_mesh(IplImage* mesh_pic0, IplImage* mesh_pic1, IplImage* mesh_pic2, IplImage* mesh_pic3) {

    cvNamedWindow("Current Mesh", CV_WINDOW_NORMAL);

    if (mesh == 0) {
        cvShowImage("Current Mesh", mesh_pic0);
        }
    else if (mesh == 1) {
        cvShowImage("Current Mesh", mesh_pic1);
    }
    else if (mesh == 2) {
        cvShowImage("Current Mesh", mesh_pic2);
    }
    else if (mesh == 3) {
        cvShowImage("Current Mesh", mesh_pic3);
    }
    cvWaitKey(1);
}

void display_cam(IplImage* front, IplImage* left, IplImage* right, IplImage* back, IplImage* top) {
    cvNamedWindow("Current Camera View", CV_WINDOW_NORMAL);

    if (cam_View.compare("N") == 0) {
        cvShowImage("Current Camera View", front);
    }
    else if (cam_View.compare("S") == 0) {
        cvShowImage("Current Camera View", back);
    }
    else if (cam_View.compare("E") == 0) {
        cvShowImage("Current Camera View", top); // temp
    }
    else if (cam_View.compare("W") == 0) {
        cvShowImage("Current Camera View", top); // temp
    }
    else if (cam_View.compare("T") == 0) {
        cvShowImage("Current Camera View", top);
    }
    cvWaitKey(1);
}



void init_obj_names() {

    // mesh 0
	obj_names.push_back("../data/obj_files_0808/rectangular_mesh_measured_front.obj");
	obj_names.push_back("../data/obj_files_0808/rectangular_mesh_measured_left.obj");
	obj_names.push_back("../data/obj_files_0808/rectangular_mesh_measured_rear.obj");
	obj_names.push_back("../data/obj_files_0808/rectangular_mesh_measured_right.obj");

    // mesh 1
    /*
    obj_names.push_back("../data/obj_files_0711/rect_narrow_mesh_front.obj");
	obj_names.push_back("../data/obj_files_0711/rect_narrow_mesh_left.obj");
	obj_names.push_back("../data/obj_files_0711/rect_narrow_mesh_rear.obj");
	obj_names.push_back("../data/obj_files_0711/rect_narrow_mesh_right.obj");
    */

    obj_names.push_back("../data/obj_files_0711/rectangular_mesh_measured_wideLR_front.obj");
    obj_names.push_back("../data/obj_files_0711/rectangular_mesh_measured_wideLR_left.obj");
    obj_names.push_back("../data/obj_files_0711/rectangular_mesh_measured_wideLR_rear.obj");
    obj_names.push_back("../data/obj_files_0711/rectangular_mesh_measured_wideLR_right.obj");

    // mesh 2
    obj_names.push_back("../data/obj_files_0711/rectangular_mesh_measured_wideFB_front_final.obj");
	obj_names.push_back("../data/obj_files_0711/rectangular_mesh_measured_wideFB_left_final.obj");
	obj_names.push_back("../data/obj_files_0711/rectangular_mesh_measured_wideFB_rear_final.obj");
	obj_names.push_back("../data/obj_files_0711/rectangular_mesh_measured_wideFB_right_final.obj");

    // mesh 3
    // obj_names.push_back("../obj_files_0711/rectangular_mesh_measured_wideLR_front.obj");
	// obj_names.push_back("../obj_files_0711/rectangular_mesh_measured_wideLR_left.obj");
	// obj_names.push_back("../obj_files_0711/rectangular_mesh_measured_wideLR_rear.obj");
	// obj_names.push_back("../obj_files_0711/rectangular_mesh_measured_wideLR_right.obj");

    // mesh 4
    obj_names.push_back("../data/obj_files_0808/FRwL_narR_front.obj");
    obj_names.push_back("../data/obj_files_0808/FRwL_narR_left.obj");
    obj_names.push_back("../data/obj_files_0808/FRwL_narR_rear.obj");
    obj_names.push_back("../data/obj_files_0808/FRwL_narR_right.obj");

    obj_names.push_back("../data/obj_files_0808/new_front.obj");
    obj_names.push_back("../data/obj_files_0808/new_left.obj");
    obj_names.push_back("../data/obj_files_0808/new_rear.obj");
    obj_names.push_back("../data/obj_files_0808/new_right.obj");

    obj_names.push_back("../data/cadillac_chassis.obj");
}

void load_all_obj() {
	for (int j = 0; j < (int) obj_names.size(); j++) {
		loadOBJ((obj_names[j]).c_str(), v[j], u[j], n[j]);
//		bool res = loadOBJ((obj_names[j]).c_str(), v[j], u[j], n[j]);
	}
}

void loadVBO_V(GLuint & paramBuffer)
{
	int i;
	int tSize = 0;
	int tIdx = 4 * mesh;
	int tBufSize = v[tIdx].size() +
				   v[tIdx + 1].size() +
				   v[tIdx + 2].size() +
				   v[tIdx + 3].size() +
				   v[(int) obj_names.size() - 1].size();

	glBindBuffer(GL_ARRAY_BUFFER, paramBuffer);
	glBufferData(GL_ARRAY_BUFFER, tBufSize * sizeof(glm::vec3), 0, GL_STATIC_DRAW);

	for (i = 0; i < 4; i++)
	{
		glBufferSubData(GL_ARRAY_BUFFER,
						tSize * sizeof(glm::vec3),
						v[tIdx + i].size() * sizeof(glm::vec3),
						&v[tIdx + i][0]);

		tSize += v[tIdx + i].size();
	}

	glBufferSubData(GL_ARRAY_BUFFER,
					tSize * sizeof(glm::vec3),
					v[(int) obj_names.size() - 1].size() * sizeof(glm::vec3),
					&v[(int) obj_names.size() - 1][0]);
}

void loadVBO_U(GLuint & paramBuffer)
{
	int i;
	int tSize = 0;
	int tIdx = 4 * mesh;
	int tBufSize = u[tIdx].size() +
				   u[tIdx + 1].size() +
				   u[tIdx + 2].size() +
				   u[tIdx + 3].size() +
				   u[(int) obj_names.size() - 1].size();

	glBindBuffer(GL_ARRAY_BUFFER, paramBuffer);
	glBufferData(GL_ARRAY_BUFFER, tBufSize * sizeof(glm::vec2), 0, GL_STATIC_DRAW);

	for (i = 0; i < 4; i++)
	{
		glBufferSubData(GL_ARRAY_BUFFER,
						tSize * sizeof(glm::vec2),
						u[tIdx + i].size() * sizeof(glm::vec2),
						&u[tIdx + i][0]);

		tSize += u[tIdx + i].size();
	}

	glBufferSubData(GL_ARRAY_BUFFER,
					tSize * sizeof(glm::vec2),
					u[(int) obj_names.size() - 1].size() * sizeof(glm::vec2),
					&u[(int) obj_names.size() - 1][0]);
}

GLuint ConvertIplToTexture(IplImage *image, int feed)
{
	GLuint texture;
	int startX, startY, sizeX, sizeY;

	glGenTextures(1,&texture);
	glBindTexture(GL_TEXTURE_2D,texture);
	/*glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_DECAL);
	glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
	gluBuild2DMipmaps(GL_TEXTURE_2D,3,image->width,image->height,
	GL_BGR,GL_UNSIGNED_BYTE,image->imageData);*/
	//printf("%d\n", feed);
	switch (feed) {
		case 1:
			startX = 0;
			startY = 0;
			sizeX = (image->width) / 2;
			sizeY = (image->height) / 2;
			break;
		case 2:
			startX = (image->width) / 2;
			startY = (image->height) / 2;
			sizeX = (image->width) / 2;
			sizeY = (image->height) / 2;
			break;
		case 3:
			startX = 0;
			startY = (image->height) / 2;
			sizeX = (image->width) / 2;
			sizeY = (image->height) / 2;
			break;
		case 4:
			startX = (image->width) / 2;
			startY = 0;
			sizeX = (image->width) / 2;
			sizeY = (image->height) / 2;
			break;
	}
	//printf("%d\n", __LINE__);

	//printf("sizeX = %d, sizeY = %d\n", sizeX, sizeY);
	CvRect cropRect = cvRect(startX, startY, sizeX, sizeY); // ROI in source image
	//printf("%d\n", __LINE__);

	cvSetImageROI(image, cropRect);
	IplImage *target_im = cvCreateImage(cvGetSize(image), image->depth, image->nChannels);
	//printf("%d\n", __LINE__);
	cvCopy(image, target_im, NULL); // Copies only crop region
	//printf("%d\n", __LINE__);
	cvResetImageROI(image);

	//printf("target_im_width = %d, target_im_height = %d\n", target_im->width, target_im->height);
	glTexImage2D(GL_TEXTURE_2D, 0,GL_RGB, target_im->width, target_im->height, 0, GL_BGR, GL_UNSIGNED_BYTE, target_im->imageData);
	// ... nice trilinear filtering ...
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	// ... which requires mipmaps. Generate them automatically.
	glGenerateMipmap(GL_TEXTURE_2D);

	cvReleaseImage(&target_im);

	return texture;
}

GLuint ConvertIplToTexturePX2(IplImage *image)
{
	GLuint texture;
//	int startX, startY, sizeX, sizeY;

    // tagging - get pid, tid
    pid_t pid = getpid();
    pid_t tid = gettid();

    // Tagging begin ///////////////////////////////////////////////////////////////////
    const char *task_1_name = "glGenTextures";
    tag_job_begin(pid, tid, task_1_name, frame_period_us, deadline_us, 14L, false, true, 1UL);

	glGenTextures(1,&texture);
	glBindTexture(GL_TEXTURE_2D,texture);
    // Tag_end /////////////////////////////////////////////////////////////////////////
    tag_job_end(pid, tid, task_1_name);

	cvFlip(image, NULL, -1);


    // Tagging begin ///////////////////////////////////////////////////////////////////
    const char *task_2_name = "glTexImage2D";
    tag_job_begin(pid, tid, task_2_name, frame_period_us, deadline_us, 14L, false, true, 1UL);

	//gGpuLog.WriteLogs(timeLog.GetTimeDiff(), "TextureBuffer", 4);	// Timediff (prepare texture buffer)

	//printf("target_im_width = %d, target_im_height = %d\n", target_im->width, target_im->height);
	glTexImage2D(GL_TEXTURE_2D, 0,GL_RGB, image->width, image->height, 0, GL_BGR, GL_UNSIGNED_BYTE, image->imageData);
	// ... nice trilinear filtering ...
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    // Tag_end /////////////////////////////////////////////////////////////////////////
    tag_job_end(pid, tid, task_2_name);

    // Tagging begin ///////////////////////////////////////////////////////////////////
    const char *task_3_name = "glGenerateMipmap";
    tag_job_begin(pid, tid, task_3_name, frame_period_us, deadline_us, 14L, false, true, 1UL);

	//gGpuLog.WriteLogs(timeLog.GetTimeDiff(), "TextureConversion", 4);	// Timediff (frame to texture conversion)

	// ... which requires mipmaps. Generate them automatically.
	glGenerateMipmap(GL_TEXTURE_2D);

	//gGpuLog.WriteLogs(timeLog.GetTimeDiff(), "MipmapGen", 4);	// Timediff (mipmap generation)

    // Tag_end /////////////////////////////////////////////////////////////////////////
    tag_job_end(pid, tid, task_3_name);

	return texture;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {

	switch(key) {
		case GLFW_KEY_1:	// default view
			gPrev = gCurr;
			gCurr = 0;
			break;
		case GLFW_KEY_2:	// top view
			gPrev = gCurr;
			gCurr = 1;
			break;
		case GLFW_KEY_3:	// default view from rear left
			gPrev = gCurr;
			gCurr = 2;
			break;
		case GLFW_KEY_4:	// default view from rear right
			gPrev = gCurr;
			gCurr = 3;
			break;
		case GLFW_KEY_5:	// right door
			gPrev = gCurr;
			gCurr = 4;
			break;
		case GLFW_KEY_6:	// front to back view
			gPrev = gCurr;
			gCurr = 5;
			break;
		case GLFW_KEY_7:	// left door
			gPrev = gCurr;
			gCurr = 6;
			break;
		case GLFW_KEY_8:	// mesh 0
			change_mesh = 1;
			mesh = 0;
            // cam_View = "N";
			break;
		case GLFW_KEY_9:	// mesh 1
			change_mesh = 1;
			mesh = 1;
            // cam_View = "S";
			break;
		case GLFW_KEY_0:	// mesh 2
			change_mesh = 1;
			mesh = 2;
            // cam_View = "E";
			break;
		case GLFW_KEY_U:	// mesh 3
			change_mesh = 1;
			mesh = 3;
			break;
		case GLFW_KEY_I:	// mesh 3
			change_mesh = 1;
			mesh = 4;
			break;
	}
}

void SetUniformMVP()
{
		// Compute the MVP matrix from keyboard and mouse input
		//computeMatricesFromInputs();
		gProjectionMatrix = glm::perspective(glm::radians(60.0f), 1920.0f / 1080.0f, 0.1f, 100.0f);

		// Create quaternions from the rotation matrices produced by glm::lookAt
		glm::quat quat_start (glm::lookAt (gCameraArray.at(gPrev), gLookArray.at(gPrev), glm::vec3(0,0,1)));
		glm::quat quat_end   (glm::lookAt (gCameraArray.at(gCurr), gLookArray.at(gCurr), glm::vec3(0,0,1)));

		// Interpolate half way from original view to the new.
		float interp_factor = 0.5; // 0.0 == original, 1.0 == new

		// First interpolate the rotation
		glm::quat  quat_interp = glm::slerp (quat_start, quat_end, interp_factor);

		// Then interpolate the translation
		glm::vec3  pos_interp  = glm::mix   (gCameraArray.at(gPrev),  gCameraArray.at(gCurr),  interp_factor);

		gViewMatrix = glm::mat4_cast (quat_interp); // Setup rotation
		gViewMatrix [3]        = glm::vec4 (pos_interp, 1.0);  // Introduce translation

		// Model matrix : an identity matrix (model will be at the origin)
		glm::mat4 ModelMatrix = glm::mat4(1.0f);
		glm::mat4 MVP = gProjectionMatrix * gViewMatrix * ModelMatrix;

		// Send our transformation to the currently bound shader,
		// in the "MVP" uniform
		glUniformMatrix4fv(gMatrixID, 1, GL_FALSE, &MVP[0][0]);
}


int initializeGLFW()
{
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
	// window = glfwCreateWindow( 1920, 1080, "3D Surround View", NULL, NULL);
	gGLFWWindow = glfwCreateWindow( 960, 540, "3D Surround View", NULL, NULL);
	if( gGLFWWindow == NULL ){
		fprintf( stderr, "Failed to open GLFW window. \
			     If you have an Intel GPU, they are not 3.3 compatible. \
				 Try the 2.1 version of the tutorials.\n" );
		getchar();
		glfwTerminate();
		return -2;
	}

	glfwMakeContextCurrent(gGLFWWindow);

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(gGLFWWindow, GLFW_STICKY_KEYS, GL_TRUE);

    // Set the mouse at the center of the screen
    glfwPollEvents();
    glfwSetCursorPos(gGLFWWindow, 1920/2, 1080/2);

	return 0;
}

int initializeGLEW()
{
	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		getchar();
		glfwTerminate();
		return -1;
	}

	return 0;
}

void initializeGL()
{
	// Dark blue background
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	//glDepthFunc(GL_LESS);

	// Cull triangles which normal is not towards the camera
	glEnable(GL_CULL_FACE);

	// Create and compile our GLSL program from the shaders
	gProgramID = LoadShaders( "TransformVertexShader.vertexshader", "TextureFragmentShader.fragmentshader" );

	// Get a handle for our "MVP" uniform
	gMatrixID = glGetUniformLocation(gProgramID, "MVP");

	//GLuint Texture = loadDDS("uvmap.DDS");

	// Get a handle for our "myTextureSampler" uniform
	gTextureID  = glGetUniformLocation(gProgramID, "myTextureSampler");

	glGenVertexArrays(1, &gVertexArrayID);
	glBindVertexArray(gVertexArrayID);
}

void initializeCameras()
{
	gCameraArray.push_back(glm::vec3(0,0,-4));			// default view
	gLookArray.push_back(glm::vec3(0,9,-10));

	gCameraArray.push_back(glm::vec3(0,-0.0001,-5));			// top view
	gLookArray.push_back(glm::vec3(0,0,-6));

	gCameraArray.push_back(glm::vec3(0,0,-5));		// default view from rear left
	gLookArray.push_back(glm::vec3(2,5,-7));

	gCameraArray.push_back(glm::vec3(0,0,-5));		// default view from rear right
	gLookArray.push_back(glm::vec3(-2,5,-7));

	gCameraArray.push_back(glm::vec3(0,0,-4));		// right door
	gLookArray.push_back(glm::vec3(-2,0,-5));

	gCameraArray.push_back(glm::vec3(0,0,-4));			// front to back view
	gLookArray.push_back(glm::vec3(0,-9,-10));

	gCameraArray.push_back(glm::vec3(0,0,-4));		// left door
	gLookArray.push_back(glm::vec3(2,0,-5));
}

int acquireFrameFromVideo()
{
	int i;

	if (video_demo)
	{
		for (i = 0; i < 4; i++)
		{
			gFrames[i] = cvQueryFrame(gCaptures[i]);

			//gGpuLog.WriteLogs(timeLog.GetTimeDiff(), "VideoToFrame", 4);	// Timediff (getting frame from video)

			if (gFrames[i] != NULL)
			{
				if (px2 == 1)
					gTextures[i] = ConvertIplToTexturePX2(gFrames[i]);
				else
					gTextures[i] = ConvertIplToTexture(gFrames[i], i+1);
			}
			else
			{
				return -1;
			}
		}
	}
	else
	{
		gTextures[0] = loadBMP_custom("front_highway_flip.bmp");
		gTextures[1] = loadBMP_custom("left_highway_flip.bmp");
		gTextures[2] = loadBMP_custom("rear_highway_flip.bmp");
		gTextures[3] = loadBMP_custom("right_highway_flip.bmp");
	}

	return 0;
}

void bindingTextures()
{
	int i;
	int offset = 0;

	// Bind our texture in Texture Unit 0
	glActiveTexture(GL_TEXTURE0);

	for (i = 0; i < 5; i++)
	{
		glBindTexture(GL_TEXTURE_2D, gTextures[i]);
		// Set our "myTextureSampler" sampler to use Texture Unit 0
		glUniform1i(gTextureID, 0);

		// 1rst attribute buffer : vertices
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, gVertexBuffer);
		glVertexAttribPointer(
			0,                  // attribute
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)(offset * sizeof(glm::vec3))            // array buffer offset
		);

		// 2nd attribute buffer : UVs
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, gUVBuffer);
		glVertexAttribPointer(
			1,                                // attribute
			2,                                // size
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)(offset * sizeof(glm::vec2))                          // array buffer offset
		);

		if ( i == 4)
			glDrawArrays(GL_TRIANGLES, 0, v[(int) obj_names.size() - 1].size() );
		else
			glDrawArrays(GL_TRIANGLES, 0, v[(4 * mesh) + i].size() );

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);

		offset += v[(4 * mesh) + i].size();
	}
}

void translateWheels()
{
	unsigned int i,j;

	if (gWheelTranslation.Rotation < -3.88f)
		gWheelTranslation.Rotation = 0.0f;
	gWheelTranslation.Rotation -= 1.296f;


	for (i = 0; i < 4; i++)
	{
		gWheelTranslation.Centers[i] = glm::vec3(0.0f, 0.0f, 0.0f);

		for (j = 0; j < gWheels[i].v_wheel.size(); j++)
			gWheelTranslation.Centers[i] += gWheels[i].v_wheel[j];

		gWheelTranslation.Centers[i] = gWheelTranslation.Centers[i] / (float) gWheels[i].v_wheel.size() ;
		glm::mat4 ModelMatrix = glm::mat4(1.0f);
		ModelMatrix = glm::translate( ModelMatrix, gWheelTranslation.Centers[i]);
		ModelMatrix = glm::rotate( ModelMatrix, gWheelTranslation.Rotation, gWheelTranslation.RotationAxis );
		ModelMatrix = glm::translate( ModelMatrix, (-1.0f) * gWheelTranslation.Centers[i]);

		glm::mat4 MVPMatrix = gProjectionMatrix * gViewMatrix * ModelMatrix;
		glUniformMatrix4fv(gMatrixID, 1, GL_FALSE, &MVPMatrix[0][0]);

		glBindTexture(GL_TEXTURE_2D, gTextures[4]);
		// Set our "myTextureSampler" sampler to use Texture Unit 0
		glUniform1i(gTextureID, 0);

		// 1rst attribute buffer : vertices

		glEnableVertexAttribArray(0);

//		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer_wheel_fl);

		glBindBuffer(GL_ARRAY_BUFFER, gWheels[i].VertexBuffer);
		glVertexAttribPointer(
			0,                  // attribute
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0             // array buffer offset
		);

		// 2nd attribute buffer : UVs
		glEnableVertexAttribArray(1);
//		glBindBuffer(GL_ARRAY_BUFFER, uvbuffer_wheel_fl);
		glBindBuffer(GL_ARRAY_BUFFER, gWheels[i].UVBuffer);
		glVertexAttribPointer(
			1,                                // attribute
			2,                                // size
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                        // array buffer offset
		);

		// Draw the triangle !
		glDrawArrays(GL_TRIANGLES, 0, gWheels[i].v_wheel.size() );

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
	}
}

int InitializeGpuLog()
{
//	gGpuQuery = new GpuQuery();
	//gGpuLog.Initialize();
/*
	if (gGpuQuery->InitializeQuery() == 0)
	{
		gGpuQuery->PrintErrorString("Initialization Failed");
		return -1;
	}

	gGpuQuery->GetDevices();
	gGpuQuery->GetDeviceNames();
	gGpuQuery->GetAccountingModes();

	gGpuQuery->EnableAccountingModes(NVML_FEATURE_ENABLED);
	gGpuQuery->GetAccountingModes();
	gGpuQuery->ClearAccountingPids();
*/
	return 0;
}

void WriteGpuLog()
{
//	gGpuQuery->GetAccountingPids();
//	gGpuQuery->GetAccountingStats();

//	gGpuLog.WriteLogs(gGpuQuery->GetMaxMemoryBytes(0), 1);
//	gGpuLog.WriteLogs(gGpuQuery->GetMemUtilization(0), 2);
//	gGpuLog.WriteLogs(gGpuQuery->GetGpuUtilization(0), 3);

//	gGpuQuery->ClearAccountingPids();
}


int main(int argc, char** argv)
{
	if (InitializeGpuLog() < 0)
		return -1;

	if (initializeGLFW() < 0)
		return -1;

	if (initializeGLEW() < 0)
		return -1;

	initializeGL();

	init_obj_names();
	load_all_obj();

	if (argc > 1)
		gVideoResolution = atoi(argv[1]);

	int i;

	for (i = 0; i < 4; i++)
		gVideoSource[i] = gConstVideoSource[4 * (gVideoResolution + 1) + i];

    if (video_demo)
	{
		if (!px2)
		{
			for (i = 0; i < 4; i++)
				gCaptures[i] = cvCreateFileCapture(gVideoCommon.c_str());
		}
		else if(px2)
		{
			for (i = 0; i < 4; i++)
				gCaptures[i] = cvCreateFileCapture(gVideoSource[i].c_str());

		}
	}

	gTextures[4] = loadBMP_custom("../data/cadillac.bmp");

	// Load it into a VBO

	glGenBuffers(1, &gVertexBuffer);
	loadVBO_V(gVertexBuffer);
	glGenBuffers(1, &gUVBuffer);
	loadVBO_U(gUVBuffer);

	initializeWheels("../data/front_left_wheel.obj", 0);
	initializeWheels("../data/front_right_wheel.obj", 1);
	initializeWheels("../data/rear_left_wheel.obj", 2);
	initializeWheels("../data/rear_right_wheel.obj", 3);

	initializeCameras();

	timeLog.Initialize(glfwGetTime());

    // tagging - get pid, tid
    pid_t pid = getpid();
    pid_t tid = gettid();

    // main loop
    do{

		timeLog.CheckFrameStartTime();
		timeLog.CheckFrame10();
		timeLog.CheckNBFrame(glfwGetTime());


		//WriteGpuLog();
		//gGpuLog.WriteLogs(timeLog.GetTimeDiff(), "Start", 4);	// Diff(Frame End, Frame Start)

		if (change_mesh) {
			change_mesh = 0;
			loadVBO_V(gVertexBuffer);
			loadVBO_U(gUVBuffer);
		}

		// Load the texture
		if (acquireFrameFromVideo() < 0) break;

		//gGpuLog.WriteLogs(timeLog.GetTimeDiff(), "OpenCVEnd", 4);	// Diff(Frame Start, OpenCV call to make texture)



        // Tagging begin ///////////////////////////////////////////////////////////////////
        const char *task_2_name = "opengl_draw";
        tag_job_begin(pid, tid, task_2_name, frame_period_us, deadline_us, 14L, false, true, 1UL);

		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Use our shader
		glUseProgram(gProgramID);

		glfwSetKeyCallback(gGLFWWindow, key_callback);

		SetUniformMVP();

		bindingTextures();

		//gGpuLog.WriteLogs(timeLog.GetTimeDiff(), "BindWheels", 4);	// Diff(OpenCV call to make texture, bind texture)

		translateWheels();

		//WriteGpuLog();
		//gGpuLog.WriteLogs(timeLog.GetTimeDiff(), "DrawWheels", 4);	// Diff(bind texture, draw wheels)

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        //glOrtho(0, 1920, 1080, 0, -1, 10);
        gluOrtho2D(0, 1920, 0, 1080);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);

        // Tag_end /////////////////////////////////////////////////////////////////////////
        tag_job_end(pid, tid, task_2_name);


        // Tagging begin ///////////////////////////////////////////////////////////////////
        const char *task_3_name = "opengl_glBegin";
        tag_job_begin(pid, tid, task_3_name, frame_period_us, deadline_us, 14L, false, true, 1UL);

		//WriteGpuLog();
		//gGpuLog.WriteLogs(timeLog.GetTimeDiff(), "glBegin", 4);	// Diff(draw wheels, glBegin)

        glClear(GL_DEPTH_BUFFER_BIT);

        glColor3f(1.0, 0.0, 0.0);
        glBegin(GL_QUADS);
        glVertex2f(0.0,0.0);
        glVertex2f(200.0,0.0);
        glVertex2f(200.0,200.0);
        glVertex2f(0.0,200.0);
        glEnd();

		//WriteGpuLog();
		//gGpuLog.WriteLogs(timeLog.GetTimeDiff(), "glEnd", 4);  	// Diff(glBegin, glEnd)

        // Tag_end /////////////////////////////////////////////////////////////////////////
        tag_job_end(pid, tid, task_3_name);


        // Tagging begin ///////////////////////////////////////////////////////////////////
        const char *task_4_name = "opengl_swapBuffer";
        tag_job_begin(pid, tid, task_4_name, frame_period_us, deadline_us, 14L, false, true, 1UL);

        glMatrixMode(GL_PROJECTION);
        glMatrixMode(GL_MODELVIEW);

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);

		// Swap buffers
		glfwSwapBuffers(gGLFWWindow);

		//gGpuLog.WriteLogs(timeLog.GetTimeDiff(), "SwapBuffer", 4);	// Diff(glEnd, glSwapBuffers)

		glfwPollEvents();
		glDeleteTextures(1, &(gTextures[0]));
		glDeleteTextures(1, &(gTextures[1]));
		glDeleteTextures(1, &(gTextures[2]));
		glDeleteTextures(1, &(gTextures[3]));

		//WriteGpuLog();
		//gGpuLog.WriteLogs(timeLog.GetTimeDiff(), "End", 4);	// Diff(glSwapBuffers, Frame End)

        // Tag_end /////////////////////////////////////////////////////////////////////////
        tag_job_end(pid, tid, task_4_name);

		timeLog.CheckFrameEndTime();

	} // Check if the ESC key was pressed or the window was closed
	while( glfwGetKey(gGLFWWindow, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
		   glfwWindowShouldClose(gGLFWWindow) == 0 );

//	gGpuQuery->ShutdownNVML();

//	delete gGpuQuery;

	timeLog.PrintResult();

	// Cleanup VBO and shader
	glDeleteBuffers(1, &gVertexBuffer);
	glDeleteBuffers(1, &gUVBuffer);
	glDeleteProgram(gProgramID);

	//glDeleteTextures(1, &Texture5);
	glDeleteVertexArrays(1, &gVertexArrayID);
	gCameraArray.clear();
	gLookArray.clear();

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}
