/* Header Inclusions */
#include <iostream>
#include <GL/glew.h>
#include <GL/freeglut.h> // includes the freeglut header file
#include <Windows.h>

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// SOIL Image Loader Inclusion
#include "SOIL2/SOIL2.h"

using namespace std;

#define WINDOW_TITLE "3D Table" // Window title Macro

/* Shader program Macro */
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version "\n" #Source
#endif


/* Variable declarations for shader, window size initialization, buffer and array objects */
GLint cubeShaderProgram, lampShaderProgram, shaderProgram, WindowWidth = 800, WindowHeight = 600;
GLuint VBO, VAO, VBOB, VAOB, CubeVAO, LightVAO, texture, texture2;
GLfloat degrees = glm::radians(-45.0f); // Convert float to radians

//Subject position and scale
glm::vec3 cubePosition(0.0f, 0.0f, 0.0f);
glm::vec3 cubeScale(2.0f);

//Cube and light color
glm::vec3 objectColor(0.6f, 0.5f, 0.75f);
glm::vec3 lightColor(0.0f, 1.0f, 0.0f);

//Light position and scale
glm::vec3 lightPosition(0.5f, 0.5f, -3.0f);
glm::vec3 lightScale(0.3f);

//Camera roatation
float cameraRotation = glm::radians(-25.0f);

GLfloat cameraSpeed = 0.0005f; // Movement speed per frame

GLchar currentKey;

GLfloat lastMouseX = 400, lastMouseY = 300; // Locks mouse cursor at the center of the screen
GLfloat mouseXOffset, mouseYOffset, yaw = 0.0f, pitch = 0.0f; // Mouse offset, yaw, and pitch variables
GLfloat sensitivity = 0.005f; // Used for mouse / camera rotation sensitivity
bool mouseDetected = true; // Initially true when mouse movemnt is detected

// Global Vector declarations
glm::vec3 cameraPosition = glm::vec3(0.0f, 0.0f, 0.0f); // Initial camera position. Placed 5 units in z
glm::vec3 CameraUpY = glm::vec3(0.0f, 1.0f, 0.0f); // Temporary y unit vector
glm::vec3 CameraForwardZ = glm::vec3(0.0f, 0.0f, -1.0f); // temporary Z unit vector
glm::vec3 front; // Temporary z unit vector for mouse


/* Function prototypes */
void UResizeWindow(int, int);
void URenderGraphics(void);
void UCreateShader(void);
void UCreateBuffers(void);
void UCreateBuffersBase(void);
void UGenerateTexture(void);
void UGenerateTextureBase(void);
void USpecialKeyboard(int key, int x, int y);

void UMouseMove(int x, int y);

/* Vertex Shader Source Code*/
const GLchar * vertexShaderSource = GLSL(330,
	layout (location = 0) in vec3 position; // Vertex data from Vertex Attrib Pointer 0
	layout (location = 2) in vec2 textureCoordinate; // Color data from Vertex Attrib Pointer 1

	out vec2 mobileTextureCoordinate; // variable to transfer color data to the fragment shader


	//Global variables for the transform matrices
	uniform mat4 model;
	uniform mat4 view;
	uniform mat4 projection;


void main(){
		gl_Position = projection * view * model * vec4(position, 1.0f); // transforms vertex data using matrix
		mobileTextureCoordinate = vec2(textureCoordinate.x, 1.0f - textureCoordinate.y); //flips texture horizontal
	}
);


/* Fragment Shader Source Code */
const GLchar * fragmentShaderSource = GLSL(330,

		in vec2 mobileTextureCoordinate;

		out vec4 gpuTexture; // Variable to pass color data to the gpu

		uniform sampler2D uTexture; // Useful when working with multiple textures

	void main(){

		gpuTexture = texture(uTexture, mobileTextureCoordinate);

	}
);
//Stop
/* Cube Vertex Shader Source Code*/
const GLchar * cubeVertexShaderSource = GLSL(330,

  layout (location = 0) in vec3 position; // VAP position 0 for vertex position data
  layout (location = 1) in vec3 normal; // VAP position 1 for normals

  out vec3 Normal; // For outgoing normals to fragment shader
  out vec3 FragmentPos; // For outgoing color / pixels to fragment shader

  //Uniform / Global variables for the  transform matrices
  uniform mat4 model;
  uniform mat4 view;
  uniform mat4 projection;



void main(){

     gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates

     FragmentPos = vec3(model * vec4(position, 1.0f)); // Gets fragment / pixel position in world space only (exclude view and projection)

     Normal = mat3(transpose(inverse(model))) * normal; // get normal vectors in world space only and exclude normal translation properties

 }
);


/* Cube Fragment Shader Source Code*/
const GLchar * cubeFragmentShaderSource = GLSL(330,

  in vec3 Normal; // For incoming normals
  in vec3 FragmentPos; // For incoming fragment position

        out vec4 cubeColor; // For outgoing cube color to the GPU

        // Uniform / Global variables for object color, light color, light position, and camera/view position
        uniform  vec3 objectColor;
        uniform  vec3 lightColor;
        uniform vec3 lightPos;
        uniform vec3 viewPosition;

 void main(){

  /*Phong lighting model calculations to generate ambient, diffuse, and specular components*/

  //Calculate Ambient lighting*/
  float ambientStrength = 0.1f; // Set ambient or global lighting strength
  vec3 ambient = ambientStrength * lightColor; // Generate ambient light color


  //Calculate Diffuse lighting*/
  vec3 norm = normalize(Normal); // Normalize vectors to 1 unit
  vec3 lightDirection = normalize(lightPos - FragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
  float impact = max(dot(norm, lightDirection), 0.0);// Calculate diffuse impact by generating dot product of normal and light
  vec3 diffuse = impact * lightColor; // Generate diffuse light color


  //Calculate Specular lighting*/
  float specularIntensity = 0.8f; // Set specular light strength
  float highlightSize = 16.0f; // Set specular highlight size
  vec3 viewDir = normalize(viewPosition - FragmentPos); // Calculate view direction
  vec3 reflectDir = reflect(-lightDirection, norm);// Calculate reflection vector
  //Calculate specular component
  float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), highlightSize);
  vec3 specular = specularIntensity * specularComponent * lightColor;

        // Calculate phong result
  vec3 phong = (ambient + diffuse + specular) * objectColor;

  cubeColor = vec4(phong, 1.0f); // Send lighting results to GPU
  }
);


/* Lamp Shader Source Code*/
const GLchar * lampVertexShaderSource = GLSL(330,

  layout (location = 0) in vec3 position; // VAP position 0 for vertex position data

        //Uniform / Global variables for the  transform matrices
  uniform mat4 model;
  uniform mat4 view;
  uniform mat4 projection;

  void main()
  {
      gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates
  }
);


/* Fragment Shader Source Code*/
const GLchar * lampFragmentShaderSource = GLSL(330,

  out vec4 color; // For outgoing lamp color (smaller cube) to the GPU

  void main()
  {
    color = vec4(1.0f); // Set color to white (1.0f,1.0f,1.0f) with alpha 1.0

  }
);



/*Main Program*/
int main(int argc, char* argv[])
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowSize(WindowWidth, WindowHeight);
	glutCreateWindow(WINDOW_TITLE);

	glutReshapeFunc(UResizeWindow);


	glewExperimental = GL_TRUE;
		if (glewInit() != GLEW_OK)
		{
			std::cout << "Failed to initialize GLEW" << std::endl;
			return -1;
		}

	UCreateShader();

	UCreateBuffers();

	UGenerateTexture();

	UCreateBuffersBase();

	UGenerateTextureBase();

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Set background color

	glutDisplayFunc(URenderGraphics);
	// Use the Shader Program
	glUseProgram(shaderProgram);
	glutSpecialFunc(USpecialKeyboard);
	glutPassiveMotionFunc(UMouseMove);
	glutMainLoop();

	// Destroys Buffer objects once used
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);

	// Destroys Buffer objects once used
	glDeleteVertexArrays(1, &VAOB);
	glDeleteBuffers(1, &VBOB);

	   // Destroys Buffer objects once used
	   glDeleteVertexArrays(1, &CubeVAO);
	   glDeleteVertexArrays(1, &LightVAO);
	   glDeleteBuffers(1, &VBO);

	return 0;
}

/* Resizes the window */
void UResizeWindow (int w, int h)
{
	WindowWidth = w;
	WindowHeight = h;
	glViewport(0, 0, WindowWidth, WindowHeight);
}

/* Renders graphics */
void URenderGraphics(void)
{

	glEnable(GL_DEPTH_TEST); // Enable z-depth

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clears the screen

	glBindVertexArray(VAO); // Activate the Vertex Array Object before rendering and transforming them

	CameraForwardZ = front; // Replaces camera forward vector with Radians normalized as a unit vector

	// Transforms the object
	glm::mat4 model;
	model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));	// Places the object in the center of the viewpont
	model = glm::rotate(model, degrees, glm::vec3(0.0f, 1.0f, 0.0f));	// Rotates shape 45 degrees on the z axis
	model = glm::scale(model, glm::vec3(2.0f, 2.0f, 2.0f)); 	// Increases the object size by scale 2

	// Transforms the camera
	glm::mat4 view;
	view = glm::lookAt(CameraForwardZ, cameraPosition, CameraUpY);

	// creates a prespective projection
	glm::mat4 projection;
	projection = glm::perspective(45.0f, (GLfloat)WindowWidth / (GLfloat)WindowHeight, 0.1f, 100.0f);

	// Retrives and passes transform matricies to the Shader program
	GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
	GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
	GLint projLoc = glGetUniformLocation(shaderProgram, "projection");

	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));


	glutPostRedisplay();

	glBindTexture(GL_TEXTURE_2D, texture);

	// Draw the triangles
	glDrawArrays(GL_TRIANGLES, 0, 36);

	glBindVertexArray(0); // Deactivate the Vertex Aray Object

	//Object # 2
	glBindVertexArray(VAOB); // Activate the Vertex Array Object before rendering and transforming them

		// Transforms the object
		glm::mat4 model2;
		model2 = glm::translate(model2, glm::vec3(0.0f, 0.0f, 0.0f));	// Places the object in the center of the viewpont
		model2 = glm::rotate(model2, degrees, glm::vec3(0.0f, 1.0f, 0.0f));	// Rotates shape 45 degrees on the z axis
		model2 = glm::scale(model2, glm::vec3(2.0f, 2.0f, 2.0f)); 	// Increases the object size by scale 2

		// Transforms the camera
		glm::mat4 view2;
		view2 = glm::lookAt(CameraForwardZ, cameraPosition, CameraUpY);

		// creates a prespective projection
		glm::mat4 projection2;
		projection = glm::perspective(45.0f, (GLfloat)WindowWidth / (GLfloat)WindowHeight, 0.1f, 100.0f);

		// Retrives and passes transform matricies to the Shader program
		GLint modelLoc2 = glGetUniformLocation(shaderProgram, "model2");
		GLint viewLoc2 = glGetUniformLocation(shaderProgram, "view2");
		GLint projLoc2 = glGetUniformLocation(shaderProgram, "projection2");

		glUniformMatrix4fv(modelLoc2, 1, GL_FALSE, glm::value_ptr(model2));
		glUniformMatrix4fv(viewLoc2, 1, GL_FALSE, glm::value_ptr(view2));
		glUniformMatrix4fv(projLoc2, 1, GL_FALSE, glm::value_ptr(projection2));

		glutPostRedisplay();

		glBindTexture(GL_TEXTURE_2D, texture2);

		// Draw the triangles
		glDrawArrays(GL_TRIANGLES, 0, 400);

		glutSwapBuffers(); // Flips the back buffer with the font buffer every frame. Similar to GL flush

		 GLint objectColorLoc, lightColorLoc, lightPositionLoc, viewPositionLoc;

		 glUseProgram(cubeShaderProgram);
		 glBindVertexArray(CubeVAO); //

		    //Transform the cube
		 model = glm::translate(model, cubePosition);
		 model = glm::scale(model, cubeScale);

		 //Transform the camera
		 view = glm::translate(view, cameraPosition);
		 view = glm::rotate(view, cameraRotation, glm::vec3(0.0f, 1.0f, 0.0f));

		 //Set the camera projection to perspective
		 projection = glm::perspective(45.0f, (GLfloat)WindowWidth / (GLfloat)WindowHeight, 0.1f, 100.0f);

		 // Reference matrix uniforms from the Cube Shader program
		 modelLoc = glGetUniformLocation(cubeShaderProgram, "model");
		 viewLoc = glGetUniformLocation(cubeShaderProgram, "view");
		 projLoc = glGetUniformLocation(cubeShaderProgram, "projection");

		 // Pass matrix data to the Cube Shader program's matrix uniforms
		 glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
		 glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
		 glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

		 // Reference matrix uniforms from the Cube Shader program for the cub color, light color, light position, and camera position
		 objectColorLoc = glGetUniformLocation(cubeShaderProgram, "objectColor");
		 lightColorLoc = glGetUniformLocation(cubeShaderProgram, "lightColor");
		    lightPositionLoc = glGetUniformLocation(cubeShaderProgram, "lightPos");
		    viewPositionLoc = glGetUniformLocation(cubeShaderProgram, "viewPosition");

		    // Pass color, light, and camera data to the Cube Shader program's corresponding uniforms
		    glUniform3f(objectColorLoc, objectColor.r, objectColor.g, objectColor.b);
		    glUniform3f(lightColorLoc, lightColor.r, lightColor.g, lightColor.b);
		    glUniform3f(lightPositionLoc, lightPosition.x, lightPosition.y, lightPosition.z);
		    glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y,cameraPosition.z);

		// glDrawArrays(GL_TRIANGLES, 0, 36); // Draw the primitives / cube

		 glBindVertexArray(0); // Deactivate the Cube Vertex Array Object


		 /****** Use the Lamp Shader and activate the Lamp Vertex Array Object for rendering and transforming******/
		 glUseProgram(lampShaderProgram);
		 glBindVertexArray(LightVAO);

		  //Transform the smaller cube used as a visual que for the light source
		 model = glm::translate(model, lightPosition);
		 model = glm::scale(model, lightScale);

		 // Reference matrix uniforms from the Lamp Shader program
		 modelLoc = glGetUniformLocation(lampShaderProgram, "model");
		 viewLoc = glGetUniformLocation(lampShaderProgram, "view");
		 projLoc = glGetUniformLocation(lampShaderProgram, "projection");

		 // Pass matrix data to the Lamp Shader program's matrix uniforms
		 glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
		 glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
		 glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

		 //glDrawArrays(GL_TRIANGLES, 0, 36);// Draw the primitives / small cube(lamp)

		 glBindVertexArray(0); // Deactivate the Lamp Vertex Array Object

		 //glutPostRedisplay(); // marks the current window to be redisplayed
		// glutSwapBuffers(); // Flips the back buffer with the front buffer every frame. Similar to glFlush


}

/* Creates the Shader program */
void UCreateShader()
{
	// Vertex shader
	GLint vertexShader = glCreateShader(GL_VERTEX_SHADER); // creates the vertex shader
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL); // Attaches the vertex shader to the source code
	glCompileShader(vertexShader); // compliles the vertex shader

	// Fragment Shader
	GLint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER); // Create the vertex shader
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL); // Attaches the fragment shader source code
	glCompileShader(fragmentShader); // Compiles the fragment shader

	// Shader program
	shaderProgram = glCreateProgram(); // creates the shader program and returns an id
	glAttachShader(shaderProgram, vertexShader); // Attach vertex shader to the shader program
	glAttachShader(shaderProgram, fragmentShader);; // Attah fragment shader to the shader program
	glLinkProgram(shaderProgram); // Link vertex and fragment shader program

	// Delete the vertex and fragment shaders once linked
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

    // Lamp Vertex shader
    GLint lampVertexShader = glCreateShader(GL_VERTEX_SHADER); // Creates the Vertex shader
    glShaderSource(lampVertexShader, 1, &lampVertexShaderSource, NULL); // Attaches the Vertex shader to the source code
    glCompileShader(lampVertexShader);  // Compiles the Vertex shader

    // Lamp Fragment shader
    GLint lampFragmentShader = glCreateShader(GL_FRAGMENT_SHADER); // Creates the Fragment shader
    glShaderSource(lampFragmentShader, 1, &lampFragmentShaderSource, NULL);// Attaches the Fragment shader to the source code
    glCompileShader(lampFragmentShader); // Compiles the Fragment shader

    // Lamp Shader program
    lampShaderProgram = glCreateProgram(); // Creates the Shader program and returns an id
    glAttachShader(lampShaderProgram, lampVertexShader); // Attach Vertex shader to the Shader program
    glAttachShader(lampShaderProgram, lampFragmentShader);; // Attach Fragment shader to the Shader program
    glLinkProgram(lampShaderProgram); //Link Vertex and Fragment shaders to Shader program

    // Delete the lamp shaders once linked
    glDeleteShader(lampVertexShader);
    glDeleteShader(lampFragmentShader);

}

/* Creates the buffer and Array Objects */
void UCreateBuffers()
{

	GLfloat vertices[] = {
			// Vertex Positions,   // Texture

	// Start Table top
			// Red side
			-0.5f, -0.05f, -1.0f,   0.0f, 0.0f,
			 0.5f, -0.05f, -1.0f,   1.0f, 0.0f,
			 0.5f,  0.05f, -1.0f,   1.0f, 1.0f,
			 0.5f,  0.05f, -1.0f,   1.0f, 1.0f,
			-0.5f,  0.05f, -1.0f,   0.0f, 1.0f,
			-0.5f, -0.05f, -1.0f,   0.0f, 0.0f,

			// Red Side
			-0.5f, -0.05f, 1.0f,    0.0f, 0.0f,
			 0.5f, -0.05f, 1.0f,    1.0f, 0.0f,
			 0.5f,  0.05f, 1.0f,    1.0f, 1.0f,
			 0.5f,  0.05f, 1.0f,    1.0f, 1.0f,
			-0.5f,  0.05f, 1.0f,    0.0f, 1.0f,
			-0.5f, -0.05f, 1.0f,    0.0f, 0.0f,

			// Blue Side
			-0.5f,  0.05f,  1.0f,    0.0f, 0.0f,
			-0.5f,  0.05f, -1.0f,    1.0f, 0.0f,
			-0.5f, -0.05f, -1.0f,    1.0f, 1.0f,
			-0.5f, -0.05f, -1.0f,    1.0f, 1.0f,
			-0.5f, -0.05f,  1.0f,    0.0f, 1.0f,
			-0.5f,  0.05f,  1.0f,    0.0f, 0.0f,

			// Yellow Side
			0.5f,  0.05f,  1.0f,     0.0f, 0.0f,
			0.5f,  0.05f, -1.0f,     1.0f, 0.0f,
			0.5f, -0.05f, -1.0f,     1.0f, 1.0f,
			0.5f, -0.05f, -1.0f,     1.0f, 1.0f,
			0.5f, -0.05f,  1.0f,     0.0f, 1.0f,
			0.5f,  0.05f,  1.0f,     0.0f, 0.0f,

			// Teal Bottom
			-0.5f, -0.05f, -1.0f,    0.0f, 0.0f,
			 0.5f, -0.05f, -1.0f,    1.0f, 0.0f,
			 0.5f, -0.05f,  1.0f,    1.0f, 1.0f,
			 0.5f, -0.05f,  1.0f,    1.0f, 1.0f,
			-0.5f, -0.05f,  1.0f,    0.0f, 1.0f,
			-0.5f, -0.05f, -1.0f,    0.0f, 0.0f,

			//Purple top
			-0.5f, 0.05f, -1.0f,     0.0f, 0.0f,
			 0.5f, 0.05f, -1.0f,     1.0f, 0.0f,
			 0.5f, 0.05f,  1.0f,     1.0f, 1.0f,
			 0.5f, 0.05f,  1.0f,     1.0f, 1.0f,
			-0.5f, 0.05f,  1.0f,    0.0f, 1.0f,
			-0.5f, 0.05f, -1.0f,     0.0f, 0.0f,
	// End Table top
};

	// Generate buffer ids
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	// Activate the Vertex Array Object before binding and setting any VBOs and vertex Attribute Pointers
	glBindVertexArray(VAO);

	// Activate the VBO
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); // Copy indices to EBO


	// Set Attribute pointer 0 to hold position data
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0); // Enables vertex attribute

	// Sets attribute pointer 1 to hold Texture data
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(2); // Enables vertex attribute

	glBindVertexArray(0); // Deactivates the VAO which is good practice
}

void UCreateBuffersBase()

{
	GLfloat vertices2[] = {
			// Vertex Positions,   // Texture

			-0.5f, -0.9f,  -1.0f,   0.0f, 0.0f,
			-0.4f, -0.9f,  -1.0f,   1.0f, 0.0f,
			-0.4f, -0.05f, -1.0f,   1.0f, 1.0f,
			-0.4f, -0.05f, -1.0f,   1.0f, 1.0f,
			-0.5f, -0.05f, -1.0f,   0.0f, 1.0f,
			-0.5f, -0.9f,  -1.0f,   0.0f, 0.0f,

			0.5f,  -0.9f, -1.0f,   0.0f, 0.0f,
			0.4f,  -0.9f, -1.0f,   1.0f, 0.0f,
			0.4f,  -0.05f, -1.0f,   1.0f, 1.0f,
			0.4f,  -0.05f, -1.0f,   1.0f, 1.0f,
			0.5f,  -0.05f, -1.0f,   0.0f, 1.0f,
			0.5f,  -0.9f, -1.0f,   0.0f, 0.0f,

			-0.5f, -0.9f, -0.9f,   0.0f, 0.0f,
			-0.4f, -0.9f, -0.9f,   1.0f, 0.0f,
			-0.4f,  -0.05f, -0.9f,   1.0f, 1.0f,
			-0.4f,  -0.05f, -0.9f,   1.0f, 1.0f,
			-0.5f,  -0.05f, -0.9f,   0.0f, 1.0f,
			-0.5f, -0.9f, -0.9f,   0.0f, 0.0f,

			0.5f, -0.9f, -0.9f,   0.0f, 0.0f,
			0.4f, -0.9f, -0.9f,   1.0f, 1.0f,
			0.4f,  -0.05f, -0.9f,   1.0f, 1.0f,
			0.4f,  -0.05f, -0.9f,   1.0f, 1.0f,
			0.5f,  -0.05f, -0.9f,   0.0f, 1.0f,
			0.5f, -0.9f, -0.9f,   0.0f, 0.0f,

			0.5f,  -0.9f,  -0.9f,     0.0f, 0.0f,
			0.5f,  -0.9f,  -1.0f,     1.0f, 1.0f,
			0.5f,  -0.05f, -1.0f,     1.0f, 1.0f,
			0.5f,  -0.05f, -1.0f,     1.0f, 1.0f,
			0.5f,  -0.05f, -0.9f,     0.0f, 1.0f,
			0.5f,  -0.9f,  -0.9f,     0.0f, 0.0f,

			-0.5f,  -0.9f,  -0.9f,     0.0f, 0.0f,
			-0.5f,  -0.9f,  -1.0f,     1.0f, 1.0f,
			-0.5f,  -0.05f,   -1.0f,     1.0f, 1.0f,
			-0.5f,  -0.05f,   -1.0f,     1.0f, 1.0f,
			-0.5f,  -0.05f,   -0.9f,   0.0f, 1.0f,
			-0.5f,  -0.9f,  -0.9f,     0.0f, 0.0f,

			0.4f,  -0.9f,  -0.9f,     0.0f, 0.0f,
			0.4f,  -0.9f,  -1.0f,     1.0f, 1.0f,
			0.4f,  -0.05f,   -1.0f,     1.0f, 1.0f,
			0.4f,  -0.05f,   -1.0f,     1.0f, 1.0f,
			0.4f,  -0.05f,   -0.9f,     0.0f, 1.0f,
			0.4f,  -0.9f,  -0.9f,     0.0f, 0.0f,

			-0.4f,  -0.9f,  -0.9f,     0.0f, 0.0f,
			-0.4f,  -0.9f,  -1.0f,     1.0f, 1.0f,
			-0.4f,  -0.05f,   -1.0f,     1.0f, 1.0f,
			-0.4f,  -0.05f,   -1.0f,     1.0f, 1.0f,
			-0.4f,  -0.05f,   -0.9f,     0.0f, 1.0f,
			-0.4f,  -0.9f,  -0.9f,     0.0f, 0.0f,

			// side X
			0.45f,  -0.9f,  -0.9f,     0.0f, 0.0f,
			0.45f,  -0.9f,  -1.0f,     1.0f, 1.0f,
			-0.35f,  -0.05f,   -1.0f,     1.0f, 1.0f,
			-0.35f,  -0.05f,   -1.0f,     1.0f, 1.0f,
			-0.35f,  -0.05f,   -0.9f,     0.0f, 1.0f,
			0.45f,  -0.9f,  -0.9f,     0.0f, 0.0f,

			0.35f,  -0.9f,  -0.9f,     0.0f, 0.0f,
			0.35f,  -0.9f,  -1.0f,     1.0f, 1.0f,
			-0.45f,  -0.05f,   -1.0f,     1.0f, 1.0f,
			-0.45f,  -0.05f,   -1.0f,     1.0f, 1.0f,
			-0.45f,  -0.05f,   -0.9f,     0.0f, 1.0f,
			0.35f,  -0.9f,  -0.9f,     0.0f, 0.0f,

			-0.45f,  -0.9f,  -0.9f,     0.0f, 0.0f,
			-0.45f,  -0.9f,  -1.0f,     1.0f, 1.0f,
			0.35f,  -0.05f,   -1.0f,     1.0f, 1.0f,
			0.35f,  -0.05f,   -1.0f,     1.0f, 1.0f,
			0.35f,  -0.05f,   -0.9f,     0.0f, 1.0f,
			-0.45f,  -0.9f,  -0.9f,     0.0f, 0.0f,

			-0.35f,  -0.9f,  -0.9f,     0.0f, 0.0f,
			-0.35f,  -0.9f,  -1.0f,     1.0f, 1.0f,
			0.45f,  -0.05f,   -1.0f,     1.0f, 1.0f,
			0.45f,  -0.05f,   -1.0f,     1.0f, 1.0f,
			0.45f,  -0.05f,   -0.9f,     0.0f, 1.0f,
			-0.35f,  -0.9f,  -0.9f,     0.0f, 0.0f,

			-0.35f,  -0.9f,  -0.9f,     0.0f, 0.0f,
			-0.35f,  -0.9f,  -1.0f,     1.0f, 1.0f,
			0.45f,  -0.05f,   -1.0f,     1.0f, 1.0f,
			0.45f,  -0.05f,   -1.0f,     1.0f, 1.0f,
			0.45f,  -0.05f,   -0.9f,     0.0f, 1.0f,
			-0.35f,  -0.9f,  -0.9f,     0.0f, 0.0f,

			-0.5f, -0.9f, 1.0f,   0.0f, 0.0f,
			 -0.4f, -0.9f, 1.0f,   1.0f, 1.0f,
			 -0.4f,  -0.05f, 1.0f,   1.0f, 1.0f,
			 -0.4f,  -0.05f, 1.0f,   1.0f, 1.0f,
			-0.5f,  -0.05f, 1.0f,   0.0f, 1.0f,
			-0.5f, -0.9f, 1.0f,   0.0f, 0.0f,

			0.5f,  -0.9f, 1.0f,   0.0f, 0.0f,
			0.4f,  -0.9f, 1.0f,   1.0f, 1.0f,
			0.4f,  -0.05f, 1.0f,   1.0f, 1.0f,
			0.4f,  -0.05f, 1.0f,   1.0f, 1.0f,
			0.5f,  -0.05f, 1.0f,   0.0f, 1.0f,
			0.5f,  -0.9f, 1.0f,   0.0f, 0.0f,

			-0.5f, -0.9f, 0.9f,   0.0f, 0.0f,
			-0.4f, -0.9f, 0.9f,   1.0f, 1.0f,
			-0.4f,  -0.05f, 0.9f,   1.0f, 1.0f,
			-0.4f,  -0.05f, 0.9f,   1.0f, 1.0f,
			-0.5f,  -0.05f, 0.9f,   0.0f, 1.0f,
			-0.5f, -0.9f, 0.9f,   0.0f, 0.0f,

			0.5f, -0.9f, 0.9f,   0.0f, 0.0f,
			0.4f, -0.9f, 0.9f,   1.0f, 1.0f,
			0.4f,  -0.05f, 0.9f,   1.0f, 1.0f,
			0.4f,  -0.05f, 0.9f,   1.0f, 1.0f,
			0.5f,  -0.05f, 0.9f,   0.0f, 1.0f,
			0.5f, -0.9f, 0.9f,   0.0f, 0.0f,

			0.5f,  -0.9f,  0.9f,     0.0f, 0.0f,
			0.5f,  -0.9f,  1.0f,     1.0f, 1.0f,
			0.5f,  -0.05f,   1.0f,     1.0f, 1.0f,
			0.5f,  -0.05f,   1.0f,     1.0f, 1.0f,
			0.5f,  -0.05f,   0.9f,     0.0f, 1.0f,
			0.5f,  -0.9f,  0.9f,     0.0f, 0.0f,

			-0.5f,  -0.9f,  0.9f,     0.0f, 0.0f,
			-0.5f,  -0.9f,  1.0f,     1.0f, 1.0f,
			-0.5f,  -0.05f,   1.0f,     1.0f, 1.0f,
			-0.5f,  -0.05f,   1.0f,     1.0f, 1.0f,
			-0.5f,  -0.05f,   0.9f,     0.0f, 1.0f,
			-0.5f,  -0.9f,  0.9f,     0.0f, 0.0f,

			0.4f,  -0.9f,  0.9f,     0.0f, 0.0f,
			0.4f,  -0.9f,  1.0f,     1.0f, 1.0f,
			0.4f,  -0.05f,   1.0f,     1.0f, 1.0f,
			0.4f,  -0.05f,   1.0f,     1.0f, 1.0f,
			0.4f,  -0.05f,   0.9f,     0.0f, 1.0f,
			0.4f,  -0.9f,  0.9f,     0.0f, 0.0f,

			-0.4f,  -0.9f,  0.9f,     0.0f, 0.0f,
			-0.4f,  -0.9f,  1.0f,     1.0f, 1.0f,
			-0.4f,  -0.05f,   1.0f,     1.0f, 1.0f,
			-0.4f,  -0.05f,   1.0f,     1.0f, 1.0f,
			-0.4f,  -0.05f,   0.9f,     0.0f, 1.0f,
			-0.4f,  -0.9f,  0.9f,     0.0f, 0.0f,

			0.45f,  -0.9f,  0.9f,     0.0f, 0.0f,
			0.45f,  -0.9f,  1.0f,     1.0f, 1.0f,
			-0.35f,  -0.05f,   1.0f,     1.0f, 1.0f,
			-0.35f,  -0.05f,   1.0f,     1.0f, 1.0f,
			-0.35f,  -0.05f,   0.9f,     0.0f, 1.0f,
			0.45f,  -0.9f,  0.9f,     0.0f, 1.0f,

			0.35f,  -0.9f,  0.9f,     0.0f, 0.0f,
			0.35f,  -0.9f,  1.0f,     1.0f, 1.0f,
			-0.45f,  -0.05f,   1.0f,     1.0f, 1.0f,
			-0.45f,  -0.05f,   1.0f,     1.0f, 1.0f,
			-0.45f,  -0.05f,   0.9f,     0.0f, 1.0f,
			0.35f,  -0.9f,  0.9f,     0.0f, 0.0f,

			-0.45f,  -0.9f,  0.9f,     0.0f, 0.0f,
			-0.45f,  -0.9f,  1.0f,     1.0f, 1.0f,
			0.35f,  -0.05f,   1.0f,     1.0f, 1.0f,
			0.35f,  -0.05f,   1.0f,     1.0f, 1.0f,
			0.35f,  -0.05f,   0.9f,     0.0f, 1.0f,
			-0.45f,  -0.9f,  0.9f,     0.0f, 0.0f,

			-0.35f,  -0.9f,  0.9f,     0.0f, 0.0f,
			-0.35f,  -0.9f,  1.0f,     1.0f, 1.0f,
			0.45f,  -0.05f,   1.0f,     1.0f, 1.0f,
			0.45f,  -0.05f,   1.0f,     1.0f, 1.0f,
			0.45f,  -0.05f,   0.9f,     0.0f, 1.0f,
			-0.35f,  -0.9f,  0.9f,     0.0f, 0.0f,

			-0.35f,  -0.9f,  0.9f,     0.0f, 0.0f,
			-0.35f,  -0.9f,  1.0f,     1.0f, 1.0f,
			0.45f,  -0.05f,   1.0f,     1.0f, 1.0f,
			0.45f,  -0.05f,   1.0f,     1.0f, 1.0f,
			0.45f,  -0.05f,   0.9f,     0.0f, 1.0f,
			-0.35f,  -0.9f,  0.9f,     0.0f, 0.0f,

	//end Sides

	// Start bottom
			// Side Pole // blue side

			-0.5f, -1.0f, -1.0f,    0.0f, 0.0f,
			 0.5f, -1.0f, -1.0f,    1.0f, 1.0f,
			 0.5f, -1.0f, -0.9f,    1.0f, 1.0f,
			 0.5f, -1.0f, -0.9f,    1.0f, 1.0f,
			-0.5f, -1.0f, -0.9f,    0.0f, 1.0f,
			-0.5f, -1.0f, -1.0f,    0.0f, 0.0f,

			// Side pole // blue side

			-0.5f, -0.9f, -1.0f,    0.0f, 0.0f,
			 0.5f, -0.9f, -1.0f,    1.0f, 1.0f,
			 0.5f, -0.9f, -0.9f,    1.0f, 1.0f,
			 0.5f, -0.9f, -0.9f,    1.0f, 1.0f,
			-0.5f, -0.9f, -0.9f,    0.0f, 1.0f,
			-0.5f, -0.9f, -1.0f,    0.0f, 0.0f,

			// side pole // blue side
			-0.5f, -0.9f, -1.0f,   0.0f, 0.0f,
			 0.5f, -0.9f, -1.0f,   1.0f, 1.0f,
			 0.5f,  -1.0f, -1.0f,   1.0f, .0f,
			 0.5f,  -1.0f, -1.0f,   1.0f, 1.0f,
			-0.5f,  -1.0f, -1.0f,   0.0f, 1.0f,
			-0.5f, -0.9f, -1.0f,   0.0f, 0.0f,

			// side pole blue side
			-0.5f, -0.9f, -0.9f,   0.0f, 0.0f,
			 0.5f, -0.9f, -0.9f,   1.0f, 1.0f,
			 0.5f,  -1.0f, -0.9f,   1.0f, 1.0f,
			 0.5f,  -1.0f, -0.9f,   1.0f, 1.0f,
			-0.5f,  -1.0f, -0.9f,   0.0f, 1.0f,
			-0.5f, -0.9f, -0.9f,   0.0f, 0.0f,

			// side pole cap // blue
			0.5f,  -0.9f,  -0.9f,     0.0f, 0.0f,
			0.5f,  -0.9f,  -1.0f,     1.0f, 1.0f,
			0.5f,  -1.0f,   -1.0f,     1.0f, 1.0f,
			0.5f,  -1.0f,   -1.0f,     1.0f, 1.0f,
			0.5f,  -1.0f,   -0.9f,     0.0f, 1.0f,
			0.5f,  -0.9f,  -0.9f,     0.0f, 0.0f,

			// side pole cap // side pole blue
			-0.5f,  -0.9f,  -0.9f,     0.0f, 0.0f,
			-0.5f,  -0.9f,  -1.0f,     1.0f, 1.0f,
			-0.5f,  -1.0f,   -1.0f,     1.0f, 1.0f,
			-0.5f,  -1.0f,   -1.0f,     1.0f, 1.0f,
			-0.5f,  -1.0f,   -0.9f,     0.0f, 1.0f,
			-0.5f,  -0.9f,  -0.9f,     0.0f, 0.0f,

			// Side Pole // purple side
			-0.5f, -1.0f, 1.0f,    0.0f, 0.0f,
			 0.5f, -1.0f, 1.0f,    1.0f, 1.0f,
			 0.5f, -1.0f, 0.9f,    1.0f, 1.0f,
			 0.5f, -1.0f, 0.9f,    1.0f, 1.0f,
			-0.5f, -1.0f, 0.9f,    0.0f, 1.0f,
			-0.5f, -1.0f, 1.0f,    0.0f, 0.0f,

			// Side pole // purple side
			-0.5f, -0.9f, 1.0f,    0.0f, 0.0f,
			 0.5f, -0.9f, 1.0f,    1.0f, 1.0f,
			 0.5f, -0.9f, 0.9f,    1.0f, 1.0f,
			 0.5f, -0.9f, 0.9f,    1.0f, 1.0f,
			-0.5f, -0.9f, 0.9f,    0.0f, 1.0f,
			-0.5f, -0.9f, 1.0f,    0.0f, 0.0f,

			// side pole // purple side
			-0.5f, -0.9f, 1.0f,   0.0f, 0.0f,
			 0.5f, -0.9f, 1.0f,   1.0f, 1.0f,
			 0.5f,  -1.0f, 1.0f,   1.0f, 1.0f,
			 0.5f,  -1.0f, 1.0f,   1.0f, 1.0f,
			-0.5f,  -1.0f, 1.0f,   0.0f, 1.0f,
			-0.5f, -0.9f, 1.0f,   0.0f, 0.0f,

			// side pole purple side
			-0.5f, -0.9f, 0.9f,   0.0f, 0.0f,
			 0.5f, -0.9f, 0.9f,   1.0f, 1.0f,
			 0.5f,  -1.0f, 0.9f,   1.0f, 1.0f,
			 0.5f,  -1.0f, 0.9f,   1.0f, 1.0f,
			-0.5f,  -1.0f, 0.9f,   0.0f, 1.0f,
			-0.5f, -0.9f, 0.9f,   0.0f, 0.0f,

			// side pole cap // purple
			0.5f,  -0.9f,  0.9f,     0.0f, 0.0f,
			0.5f,  -0.9f,  1.0f,     1.0f, 1.0f,
			0.5f,  -1.0f,   1.0f,     1.0f, 1.0f,
			0.5f,  -1.0f,   1.0f,     1.0f, 1.0f,
			0.5f,  -1.0f,   0.9f,     0.0f, 1.0f,
			0.5f,  -0.9f,  0.9f,     0.0f, 0.0f,

			// side pole cap // side pole purple
			-0.5f,  -0.9f,  0.9f,     0.0f, 0.0f,
			-0.5f,  -0.9f,  1.0f,     1.0f, 1.0f,
			-0.5f,  -1.0f,   1.0f,     1.0f, 1.0f,
			-0.5f,  -1.0f,   1.0f,     1.0f, 1.0f,
			-0.5f,  -1.0f,   0.9f,     0.0f, 1.0f,
			-0.5f,  -0.9f,  0.9f,     0.0f, 0.0f,

			// base side
			0.5f,  -0.9f,  0.9f,     0.0f, 0.0f,
			0.5f,  -0.9f, -0.9f,     1.0f, 1.0f,
			0.5f, -1.0f, -0.9f,     1.0f,  1.0f,
			0.5f, -1.0f, -0.9f,     1.0f, 1.0f,
			0.5f, -1.0f,  0.9f,     0.0f, 1.0f,
			0.5f,  -0.9f,  0.9f,     0.0f, 0.0f,

			-0.5f,  -0.9f,  0.9f,     0.0f, 0.0f,
			-0.5f,  -0.9f, -0.9f,     1.0f, 1.0f,
			-0.5f, -1.0f, -0.9f,     1.0f,  1.0f,
			-0.5f, -1.0f, -0.9f,     1.0f, 1.0f,
			-0.5f, -1.0f,  0.9f,     0.0f, 1.0f,
			-0.5f,  -0.9f,  0.9f,     0.0f, 0.0f,

			-0.4f,  -0.9f,  0.9f,     0.0f, 0.0f,
			-0.4f,  -0.9f, -0.9f,     1.0f, 1.0f,
			-0.4f, -1.0f, -0.9f,     1.0f,  1.0f,
			-0.4f, -1.0f, -0.9f,     1.0f, 1.0f,
			-0.4f, -1.0f,  0.9f,     0.0f, 1.0f,
			-0.4f,  -0.9f,  0.9f,     0.0f, 0.0f,

			0.4f,  -0.9f,  0.9f,     0.0f, 0.0f,
			0.4f,  -0.9f, -0.9f,     1.0f, 1.0f,
			0.4f, -1.0f, -0.9f,     1.0f,  1.0f,
			0.4f, -1.0f, -0.9f,     1.0f, 1.0f,
			0.4f, -1.0f,  0.9f,     0.0f, 1.0f,
			0.4f,  -0.9f,  0.9f,     0.0f, 0.0f,

			-0.5f, -1.0f, -0.9f,    0.0f, 1.0f,
			 -0.4f, -1.0f, -0.9f,    1.0f, 1.0f,
			 -0.4f, -1.0f,  0.9f,    1.0f, 1.0f,
			 -0.4f, -1.0f,  0.9f,    1.0f, 1.0f,
			-0.5f, -1.0f,  0.9f,    0.0f, 1.0f,
			-0.5f, -1.0f, -0.9f,    0.0f, 0.0f,

			0.5f, -1.0f, -0.9f,    0.0f, 0.0f,
			0.4f, -1.0f, -0.9f,    1.0f, 1.0f,
			0.4f, -1.0f,  0.9f,    1.0f, 1.0f,
			0.4f, -1.0f,  0.9f,    1.0f, 1.0f,
			0.5f, -1.0f,  0.9f,    0.0f, 1.0f,
			0.5f, -1.0f, -0.9f,    0.0f, 0.0f,

			-0.5f, -0.9f, -0.9f,    0.0f, 0.0f,
			-0.4f, -0.9f, -0.9f,    1.0f, 1.0f,
			-0.4f, -0.9f,  0.9f,    1.0f, 1.0f,
			-0.4f, -0.9f,  0.9f,    1.0f, 1.0f,
			-0.5f, -0.9f,  0.9f,    0.0f, 1.0f,
			-0.5f, -0.9f, -0.9f,    0.0f, 0.0f,

			0.5f, -0.9f, -0.9f,    0.0f, 0.0f,
			0.4f, -0.9f, -0.9f,    1.0f, 1.0f,
			0.4f, -0.9f,  0.9f,    1.0f, 1.0f,
			0.4f, -0.9f,  0.9f,    1.0f, 1.0f,
			0.5f, -0.9f,  0.9f,    0.0f, 1.0f,
			0.5f, -0.9f, -0.9f,    0.0f, 0.0f
	// end base!!
	};

	// Generate buffer ids
	glGenVertexArrays(1, &VAOB);
	glGenBuffers(1, &VBOB);

	// Activate the Vertex Array Object before binding and setting any VBOs and vertex Attribute Pointers
	glBindVertexArray(VAOB);

	// Activate the VBO
	glBindBuffer(GL_ARRAY_BUFFER, VBOB);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices2), vertices2, GL_STATIC_DRAW); // Copy indices to EBO


	// Set Attribute pointer 0 to hold position data
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0); // Enables vertex attribute

	// Sets attribute pointer 1 to hold Texture data
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(2); // Enables vertex attribute

	glBindVertexArray(0); // Deactivates the VAO which is good practice
}

/* Implements the UMouse Move Function*/
void UGenerateTexture()
{
		glGenTextures(2, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);

		int width, height;

		unsigned char* image = SOIL_load_image("TableTop.jpg", &width, &height, 0, SOIL_LOAD_RGB); // Loads texture file

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		glGenerateMipmap(GL_TEXTURE_2D);
		SOIL_free_image_data(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture
}

void UGenerateTextureBase()
{
		glGenTextures(1, &texture2);
		glBindTexture(GL_TEXTURE_2D, texture2);

		int width2, height2;

		unsigned char* image2 = SOIL_load_image("Gray.jpg", &width2, &height2, 0, SOIL_LOAD_RGB); // Loads texture file

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width2, height2, 0, GL_RGB, GL_UNSIGNED_BYTE, image2);
		glGenerateMipmap(GL_TEXTURE_2D);
		SOIL_free_image_data(image2);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture
}
/* Implements the UMouse Move Function*/
void UMouseMove(int x, int y)
{
	// Immediately replaces center locked coordinates with new mouse coordinates
	if(mouseDetected)
	{
		lastMouseX = x;
		lastMouseY = y;
		mouseDetected = false;
	}

	if(currentKey == 'a')
	{
		// Gets the direction the mouse was moved in x and y
		mouseXOffset = x - lastMouseX;
		mouseYOffset = lastMouseY - y; // Inverted y

		//Updates with new mouse coordinates
		lastMouseX = x;
		lastMouseY = y;

		// Applies sensitivity to mouse direction
		mouseXOffset *= sensitivity;
		mouseYOffset *= sensitivity;

		// Accumulates the yaw and pitch variables
		yaw += mouseXOffset;
		pitch += mouseYOffset;

		// Orbits around the center
		front.x = 10.f * cos(yaw);
		front.y = 10.0f * sin(pitch);
		front.z = sin(yaw) * cos(pitch) * 10.0f;
	}
}

void USpecialKeyboard(int key, GLint x, GLint y)
{
	int mod = glutGetModifiers();
	switch(mod)
	{
		case GLUT_ACTIVE_ALT:
			if(currentKey == key)
			{
				currentKey = 0;
			}
			else
			{
			currentKey = 'a';
			}
			break;

		default:
			if (currentKey == 'a')
			{
			cout << "Press 'alt' to stop orbiting the table" << endl;
			}
			else
			{
			cout << "Press 'alt' to orbit the table" << endl;
			}
	}
}
