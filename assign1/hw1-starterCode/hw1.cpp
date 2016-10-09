/*
  CSCI 420 Computer Graphics, USC
  Assignment 1: Height Fields

  Student username: ivylin
*/

#include <iostream>
#include <cstring>

# include <sstream>
#include <iomanip>

#include "openGLHeader.h"
#include "glutHeader.h"

#include "imageIO.h"
#include "openGLMatrix.h"
#include "basicPipelineProgram.h"

#ifdef WIN32
  #ifdef _DEBUG
    #pragma comment(lib, "glew32d.lib")
  #else
    #pragma comment(lib, "glew32.lib")
  #endif
#endif

#ifdef WIN32
  char shaderBasePath[1024] = SHADER_BASE_PATH;
#else
  char shaderBasePath[1024] = "../openGLHelper-starterCode";
#endif

using namespace std;

int mousePos[2]; // x,y coordinate of the mouse position

int leftMouseButton = 0; // 1 if pressed, 0 if not 
int middleMouseButton = 0; // 1 if pressed, 0 if not
int rightMouseButton = 0; // 1 if pressed, 0 if not

typedef enum { ROTATE, TRANSLATE, SCALE } CONTROL_STATE;
CONTROL_STATE controlState = ROTATE;

// state of the world
float landRotate[3] = { 0.0f, 0.0f, 0.0f };
float landTranslate[3] = { 0.0f, 0.0f, 0.0f };
float landScale[3] = { 1.0f, 1.0f, 1.0f };

int windowWidth = 1280;
int windowHeight = 720;
char windowTitle[512] = "CSCI 420 homework I";

GLfloat delta = 1.0;
GLint axis = 2;
int isAutoRotate = 1;

// global variable
BasicPipelineProgram * pipelineProgram;
ImageIO * heightmapImage;
OpenGLMatrix * matrix;
GLuint vao;
GLuint vao_line;
GLuint vao_tri;
GLuint buffer;
GLuint buffer_line;
GLuint buffer_tri;
int numVertices;
float * positions;
float * colors;
float * line_positions;
float * line_colors;
float * tri_positions;
float * tri_colors;
int line_positions_size;
int line_colors_size;
int tri_positions_size;
int tri_colors_size;

typedef enum { POINT, LINE, SOLID } VIEW_MODE;
VIEW_MODE viewMode = SOLID;

// variables for animation
int timeTemp = 0;
int counter = 0;
bool animation = false;
char savedImagePath[1024] = "screenshots/";

// write a screenshot to the specified filename
void saveScreenshot(const char * filename)
{
  unsigned char * screenshotData = new unsigned char[windowWidth * windowHeight * 3];
  glReadPixels(0, 0, windowWidth, windowHeight, GL_RGB, GL_UNSIGNED_BYTE, screenshotData);

  ImageIO screenshotImg(windowWidth, windowHeight, 3, screenshotData);

  if (screenshotImg.save(filename, ImageIO::FORMAT_JPEG) == ImageIO::OK)
    cout << "File " << filename << " saved successfully." << endl;
  else cout << "Failed to save file " << filename << '.' << endl;

  delete [] screenshotData;
}

void bindProgram()
{
    GLuint program = pipelineProgram->GetProgramHandle();
    
    // bind the VBO buffer
    glBindBuffer(GL_ARRAY_BUFFER, buffer); // so that glVertexAttribPointer refers to the correct buffer
    GLuint loc = glGetAttribLocation(program, "position");
    glEnableVertexAttribArray(loc);
    const void * offset = (const void*) 0;
    glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, offset);    
    GLuint loc2 = glGetAttribLocation(program, "color");
    glEnableVertexAttribArray(loc2);
    int sizeofPositions = numVertices * 3 * sizeof(float);
    const void * offset2 = (const void*) sizeofPositions;  
    glVertexAttribPointer(loc2, 4, GL_FLOAT, GL_FALSE, 0, offset2);

    // bind the VBO buffer_line
    glBindBuffer(GL_ARRAY_BUFFER, buffer_line);  
    // get location index of the “position” shader variable
    loc = glGetAttribLocation(program, "position");
    glEnableVertexAttribArray(loc); // enable the “position” attribute   
    offset = (const void*) 0;
    // set the layout of the “position” attribute data
    glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, offset);   
    // get the location index of the “color” shader variable
    loc2 = glGetAttribLocation(program, "color");
    glEnableVertexAttribArray(loc2); // enable the “color” attribute
    int sizeofLinePositions = line_positions_size * sizeof(float);   
    offset2 = (const void*) sizeofLinePositions;
    // set the layout of the “color” attribute data
    glVertexAttribPointer(loc2, 4, GL_FLOAT, GL_FALSE, 0, offset2);

    // bind the VBO buffer_tri
    glBindBuffer(GL_ARRAY_BUFFER, buffer_tri);  
    // get location index of the “position” shader variable
    loc = glGetAttribLocation(program, "position");
    glEnableVertexAttribArray(loc); // enable the “position” attribute   
    offset = (const void*) 0;
    // set the layout of the “position” attribute data
    glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, offset);   
    // get the location index of the “color” shader variable
    loc2 = glGetAttribLocation(program, "color");
    glEnableVertexAttribArray(loc2); // enable the “color” attribute
    int sizeofTriPositions = tri_positions_size * sizeof(float);   
    offset2 = (const void*) sizeofTriPositions;
    // set the layout of the “color” attribute data
    glVertexAttribPointer(loc2, 4, GL_FLOAT, GL_FALSE, 0, offset2);

    // write projection and modelview matrix to shader
    //
    
    // before rendering, bind (activate) the pipeline program:
    pipelineProgram->Bind();  
}

void render()
{
    GLint first = 0;
    GLsizei count = numVertices;
    GLsizei count_line = line_positions_size / 3;
    GLsizei count_tri = tri_positions_size / 3;

    switch (viewMode)
    {
      case POINT:
        glDrawArrays( GL_POINTS, first, count);
      break;

      case LINE:
        glDrawArrays( GL_LINES, first, count_line);
      break;

      case SOLID:
        glDrawArrays( GL_TRIANGLE_STRIP, first, count_tri);
      break;
    }
}


void displayFunc()
{
    // render some stuff...
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    matrix->SetMatrixMode(OpenGLMatrix::ModelView);

    matrix->LoadIdentity();
    matrix->LookAt(0, 0, 3, 0, 0, 0, 0, 1, 0); // default camera
    matrix->Translate(landTranslate[0], landTranslate[1], landTranslate[2]);
    matrix->Rotate(landRotate[0], 1.0, 0.0, 0.0);
    matrix->Rotate(landRotate[1], 0.0, 1.0, 0.0);
    matrix->Rotate(landRotate[2], 0.0, 0.0, 1.0);
    matrix->Scale(landScale[0], landScale[1], landScale[2]);
    
    // get a handle to the program
    GLuint program = pipelineProgram->GetProgramHandle();
    // get a handle to the modelViewMatrix shader variable
    GLint h_modelViewMatrix = glGetUniformLocation(program, "modelViewMatrix");
    float m[16]; // column-major
    matrix->GetMatrix(m);  
    // upload m to the GPU
    GLboolean isRowMajor = GL_FALSE;
    glUniformMatrix4fv(h_modelViewMatrix, 1, isRowMajor, m);
    
    GLfloat aspect = (GLfloat) windowWidth / (GLfloat) windowHeight;
    matrix->SetMatrixMode(OpenGLMatrix::Projection);
    matrix->LoadIdentity();
    matrix->Perspective(45.0, 1.0 * aspect, 0.01, 10.0);

    // get a handle to the projectionMatrix shader variable
    GLint h_projectionMatrix = glGetUniformLocation(program, "projectionMatrix");
    float p[16];
    matrix->GetMatrix(p);
    isRowMajor = GL_FALSE;
    glUniformMatrix4fv(h_projectionMatrix, 1, isRowMajor, p);
    
    bindProgram();

    switch (viewMode)
    {
      case POINT:
        glBindVertexArray(vao);
      break;

      case LINE:
        glBindVertexArray(vao_line);
      break;

      case SOLID:
        glBindVertexArray(vao_tri);
      break;
    }
    
    render();
    
    glBindVertexArray(0); // unbind the VAO
    
    glutSwapBuffers();
}

void idleFunc()
{
  // do some stuff...
    
    //spin the quad delta degrees around the selected axis
    if(isAutoRotate) {
      landRotate[axis] += delta;
      if (landRotate[axis] > 360.0)
      landRotate[axis] -= 360.0;
    }

  // for example, here, you can save the screenshots to disk (to make the animation)
    // if during animation
    if(animation) {
      if( ( ( (float)glutGet(GLUT_ELAPSED_TIME)-timeTemp/1000 ) >= (float)1/15 ) && counter < 300 ) {
        stringstream ss;
        ss << setw(3) << setfill('0') << counter;
        string filename = ss.str();
        filename = savedImagePath + filename + ".jpg";
        saveScreenshot(filename.c_str());

        cout << "File " << filename << " saved." << endl;

        timeTemp = glutGet(GLUT_ELAPSED_TIME);  //Set new timer
        counter += 1;
      }
      else {
        animation = false;
      }
    }

  // make the screen update 
  glutPostRedisplay();
}

void reshapeFunc(int w, int h)
{
    glViewport(0, 0, w, h);

    matrix->SetMatrixMode(OpenGLMatrix::Projection);
    matrix->LoadIdentity();
    matrix->Perspective(60.0, 1.0 * w / h, 0.01, 10.0);
    matrix->SetMatrixMode(OpenGLMatrix::ModelView);
}

void mouseMotionDragFunc(int x, int y)
{
  // mouse has moved and one of the mouse buttons is pressed (dragging)

  // the change in mouse position since the last invocation of this function
  int mousePosDelta[2] = { x - mousePos[0], y - mousePos[1] };

  switch (controlState)
  {
    // translate the landscape
    case TRANSLATE:
      if (leftMouseButton)
      {
        // control x,y translation via the left mouse button
        landTranslate[0] += mousePosDelta[0] * 0.01f;
        landTranslate[1] -= mousePosDelta[1] * 0.01f;
      }
      if (middleMouseButton)
      {
        // control z translation via the middle mouse button
        landTranslate[2] += mousePosDelta[1] * 0.01f;
      }
      break;

    // rotate the landscape
    case ROTATE:
      if (leftMouseButton)
      {
        // control x,y rotation via the left mouse button
        landRotate[0] += mousePosDelta[1];
        landRotate[1] += mousePosDelta[0];
      }
      if (middleMouseButton)
      {
        // control z rotation via the middle mouse button
        landRotate[2] += mousePosDelta[1];
      }
      break;

    // scale the landscape
    case SCALE:
      if (leftMouseButton)
      {
        // control x,y scaling via the left mouse button
        landScale[0] *= 1.0f + mousePosDelta[0] * 0.01f;
        landScale[1] *= 1.0f - mousePosDelta[1] * 0.01f;
      }
      if (middleMouseButton)
      {
        // control z scaling via the middle mouse button
        landScale[2] *= 1.0f - mousePosDelta[1] * 0.01f;
      }
      break;
  }

  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void mouseMotionFunc(int x, int y)
{
  // mouse has moved
  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void mouseButtonFunc(int button, int state, int x, int y)
{
  // a mouse button has has been pressed or depressed

  // keep track of the mouse button state, in leftMouseButton, middleMouseButton, rightMouseButton variables
  switch (button)
  {
    case GLUT_LEFT_BUTTON:
      leftMouseButton = (state == GLUT_DOWN);
    break;

    case GLUT_MIDDLE_BUTTON:
      middleMouseButton = (state == GLUT_DOWN);
    break;

    case GLUT_RIGHT_BUTTON:
      rightMouseButton = (state == GLUT_DOWN);
    break;
  }

  // keep track of whether CTRL and SHIFT keys are pressed
  switch (glutGetModifiers())
  {
    case GLUT_ACTIVE_CTRL:
      controlState = TRANSLATE;
    break;

    case GLUT_ACTIVE_SHIFT:
      controlState = SCALE;
    break;

    // if CTRL and SHIFT are not pressed, we are in rotate mode
    default:
      controlState = ROTATE;
    break;
  }

  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void keyboardFunc(unsigned char key, int x, int y)
{
  switch (key)
  {
    case 27: // ESC key
      exit(0); // exit the program
    break;

    case ' ':
      isAutoRotate = !isAutoRotate;
    break;

    case 'x':
      // take a screenshot
      saveScreenshot("screenshot.jpg");
    break;

    case 'a':
      // start animation
      // animation();
      cout << "Animation!" << endl;
      animation = true;
      counter = 0;
      timeTemp = glutGet(GLUT_ELAPSED_TIME);
      cout << timeTemp << endl;
    break;

    case 'p':
      viewMode = POINT;
    break;

    case 'l':
      viewMode = LINE;
    break;

    case 's':
      viewMode = SOLID;
    break;
  }
}

void keyboardSpecialInputFunc(int key, int x, int y)
{
  switch(key)
  {  
    case GLUT_KEY_DOWN:
      axis = 0;
    break;

    case GLUT_KEY_RIGHT:
      axis = 1;
    break;

    case GLUT_KEY_LEFT:
      axis = 2;
    break;
  }
}

void initPipelineProgram()
{
    // initialize shader pipeline program
    pipelineProgram = new BasicPipelineProgram();
    pipelineProgram->Init("../openGLHelper-starterCode");
}

void initVBO()
{
    // buffer for positions & colors
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    
    glBufferData( GL_ARRAY_BUFFER, numVertices * 7 * sizeof(float), positions, GL_STATIC_DRAW );
    glBufferSubData( GL_ARRAY_BUFFER, 0, numVertices * 3 * sizeof(float), positions );
    glBufferSubData( GL_ARRAY_BUFFER, numVertices * 3 * sizeof(float), numVertices * 4 * sizeof(float), colors );

    // buffer for line_positions & line_colors
    glGenBuffers(1, &buffer_line);
    glBindBuffer(GL_ARRAY_BUFFER, buffer_line);

    glBufferData( GL_ARRAY_BUFFER, (line_colors_size + line_positions_size) * sizeof(float), line_positions, GL_STATIC_DRAW );
    glBufferSubData( GL_ARRAY_BUFFER, 0, line_positions_size * sizeof(float), line_positions );
    glBufferSubData( GL_ARRAY_BUFFER, line_positions_size * sizeof(float), line_colors_size * sizeof(float), line_colors );

    // buffer for tri_positions & tri_colors
    glGenBuffers(1, &buffer_tri);
    glBindBuffer(GL_ARRAY_BUFFER, buffer_tri);

    glBufferData( GL_ARRAY_BUFFER, (tri_colors_size + tri_positions_size) * sizeof(float), tri_positions, GL_STATIC_DRAW );
    glBufferSubData( GL_ARRAY_BUFFER, 0, tri_positions_size * sizeof(float), tri_positions );
    glBufferSubData( GL_ARRAY_BUFFER, tri_positions_size * sizeof(float), tri_colors_size * sizeof(float), tri_colors );
}

void initVAO()
{
    // vao for positions
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao); // bind the VAO   

    // // bind the VBO “buffer” (must be previously created)
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    
    GLuint program = pipelineProgram->GetProgramHandle();
    
    // get location index of the “position” shader variable
    GLuint loc = glGetAttribLocation(program, "position");
    glEnableVertexAttribArray(loc); // enable the “position” attribute
    
    const void * offset = (const void*) 0;
    GLsizei stride = 0;
    GLboolean normalized = GL_FALSE;
    // set the layout of the “position” attribute data
    glVertexAttribPointer(loc, 3, GL_FLOAT, normalized, stride, offset);
    
    // get the location index of the “color” shader variable
    GLuint loc2 = glGetAttribLocation(program, "color");
    glEnableVertexAttribArray(loc2); // enable the “color” attribute
    int sizeofPositions = numVertices * 3 * sizeof(float);
    const void * offset2 = (const void*) sizeofPositions;
    // set the layout of the “color” attribute data
    glVertexAttribPointer(loc2, 4, GL_FLOAT, normalized, stride, offset2);
    glBindVertexArray(0); // unbind the VAO

    
    //  vao for line_positions
    glGenVertexArrays(1, &vao_line);
    glBindVertexArray(vao_line); // bind the VAO   

    // bind the VBO buffer_line
    glBindBuffer(GL_ARRAY_BUFFER, buffer_line);
    
    // get location index of the “position” shader variable
    loc = glGetAttribLocation(program, "position");
    glEnableVertexAttribArray(loc); // enable the “position” attribute
    
    offset = (const void*) 0;
    // set the layout of the “position” attribute data
    glVertexAttribPointer(loc, 3, GL_FLOAT, normalized, stride, offset);
    
    // get the location index of the “color” shader variable
    loc2 = glGetAttribLocation(program, "color");
    glEnableVertexAttribArray(loc2); // enable the “color” attribute

    int sizeofLinePositions = line_positions_size * sizeof(float);
    
    offset2 = (const void*) sizeofLinePositions;
    // set the layout of the “color” attribute data
    glVertexAttribPointer(loc2, 4, GL_FLOAT, normalized, stride, offset2);
    glBindVertexArray(0); // unbind the VAO
    

    //  vao for tri_positions
    glGenVertexArrays(1, &vao_tri);
    glBindVertexArray(vao_tri); // bind the VAO   

    // bind the VBO buffer_tri
    glBindBuffer(GL_ARRAY_BUFFER, buffer_tri);
    
    // get location index of the “position” shader variable
    loc = glGetAttribLocation(program, "position");
    glEnableVertexAttribArray(loc); // enable the “position” attribute
    
    offset = (const void*) 0;
    // set the layout of the “position” attribute data
    glVertexAttribPointer(loc, 3, GL_FLOAT, normalized, stride, offset);
    
    // get the location index of the “color” shader variable
    loc2 = glGetAttribLocation(program, "color");
    glEnableVertexAttribArray(loc2); // enable the “color” attribute

    int sizeofTriPositions = tri_positions_size * sizeof(float);
    
    offset2 = (const void*) sizeofTriPositions;
    // set the layout of the “color” attribute data
    glVertexAttribPointer(loc2, 4, GL_FLOAT, normalized, stride, offset2);
    glBindVertexArray(0); // unbind the VAO
}

void initScene(int argc, char *argv[])
{
    // load the image from a jpeg disk file to main memory
    heightmapImage = new ImageIO();
    if (heightmapImage->loadJPEG(argv[1]) != ImageIO::OK)
    {
      cout << "Error reading image " << argv[1] << "." << endl;
      exit(EXIT_FAILURE);
    }

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    // do additional initialization here...
    // load positions and colors
    int imageWidth = heightmapImage->getWidth();
    int imageHeight = heightmapImage->getHeight();
    numVertices = imageWidth * imageHeight;
    positions = new float[numVertices*3];
    colors = new float[numVertices*4];

    line_positions_size = (6 * numVertices - 4 * imageWidth - 4 * imageHeight + 2) * 3;
    line_colors_size = line_positions_size / 3 * 4;
    line_positions = new float[line_positions_size];
    line_colors = new float[line_colors_size];

    tri_positions_size = (numVertices + (imageHeight - 2) * imageWidth) * 3;
    tri_colors_size = tri_positions_size / 3 * 4;
    tri_positions = new float[tri_positions_size];
    tri_colors = new float[tri_colors_size];
    
    int index_pos_line = 0;
    int index_col_line = 0;
    int index_pos_tri = 0;
    int index_col_tri = 0;

    for(int i=0; i<imageHeight; ++i) {
      for(int j=0; j<imageWidth; ++j) {
        // compute the height as a value between 0.0 and 1.0
        float pixel = (float)(heightmapImage->getPixel(j, i, 0));
        float pointHeight = pixel / 256.0;
        int index_pos = (i*imageWidth + j)*3;
        int index_col = (i*imageWidth + j)*4;
        
        positions[index_pos] = (float)j / 256.0 - 0.5;
        positions[index_pos+1] = (float)i / 256.0 - 0.5;
        positions[index_pos+2] = pointHeight / 2;
        
        colors[index_col] = 0; //pointHeight;
        colors[index_col+1] = pointHeight;
        colors[index_col+2] = pointHeight;
        colors[index_col+3] = 1.0f;

        // positions and colors for GL_LINES
        if(i != imageHeight - 1) {
          // positions and colors for vertical line
          float belowPointHeight = (float)(heightmapImage->getPixel(j, i+1, 0)) / 256.0;

          line_positions[index_pos_line] = (float)j / 256.0 - 0.5;
          line_positions[index_pos_line+1] = (float)i / 256.0 - 0.5;
          line_positions[index_pos_line+2] = pointHeight / 2;
          line_positions[index_pos_line+3] = (float)j / 256.0 - 0.5;
          line_positions[index_pos_line+4] = (float)(i + 1) / 256.0 - 0.5;
          line_positions[index_pos_line+5] = belowPointHeight / 2;

          line_colors[index_col_line] = 0; //pointHeight;
          line_colors[index_col_line+1] = pointHeight;
          line_colors[index_col_line+2] = pointHeight;
          line_colors[index_col_line+3] = 1.0f;
          line_colors[index_col_line+4] = 0; //belowPointHeight;
          line_colors[index_col_line+5] = belowPointHeight;
          line_colors[index_col_line+6] = belowPointHeight;
          line_colors[index_col_line+7] = 1.0f;

          index_pos_line += 6;
          index_col_line += 8;

          // positions and colors for diagonal line
          if(j != imageWidth - 1) {
            float diagPointHeight = (float)(heightmapImage->getPixel(j+1, i+1, 0)) / 256.0;
            line_positions[index_pos_line] = (float)j / 256.0 - 0.5;
            line_positions[index_pos_line+1] = (float)i / 256.0 - 0.5;
            line_positions[index_pos_line+2] = pointHeight / 2;
            line_positions[index_pos_line+3] = (float)(j + 1) / 256.0 - 0.5;
            line_positions[index_pos_line+4] = (float)(i + 1) / 256.0 - 0.5;
            line_positions[index_pos_line+5] = diagPointHeight / 2;

            line_colors[index_col_line] = 0; //pointHeight;
            line_colors[index_col_line+1] = pointHeight;
            line_colors[index_col_line+2] = pointHeight;
            line_colors[index_col_line+3] = 1.0f;
            line_colors[index_col_line+4] = 0; //diagPointHeight;
            line_colors[index_col_line+5] = diagPointHeight;
            line_colors[index_col_line+6] = diagPointHeight;
            line_colors[index_col_line+7] = 1.0f;

            index_pos_line += 6;
            index_col_line += 8;
          }
        }

        // positions and colors for horizontal line
        if(j != imageWidth - 1) {
          float rightPointHeight = (float)(heightmapImage->getPixel(j+1, i, 0)) / 256.0;

          line_positions[index_pos_line] = (float)j / 256.0 - 0.5;
          line_positions[index_pos_line+1] = (float)i / 256.0 - 0.5;
          line_positions[index_pos_line+2] = pointHeight / 2;
          line_positions[index_pos_line+3] = (float)(j + 1) / 256.0 - 0.5;
          line_positions[index_pos_line+4] = (float)i / 256.0 - 0.5;
          line_positions[index_pos_line+5] = rightPointHeight / 2;

          line_colors[index_col_line] = 0; //pointHeight;
          line_colors[index_col_line+1] = pointHeight;
          line_colors[index_col_line+2] = pointHeight;
          line_colors[index_col_line+3] = 1.0f;
          line_colors[index_col_line+4] = 0; //rightPointHeight;
          line_colors[index_col_line+5] = rightPointHeight;
          line_colors[index_col_line+6] = rightPointHeight;
          line_colors[index_col_line+7] = 1.0f;

          index_pos_line += 6;
          index_col_line += 8;
        }
      }

      // positions and colors for GL_TRIANGLE_STRIPE
      if(i % 2 == 0) {
        for(int j=0; j<imageWidth; ++j) {
          float pixel1 = (float)(heightmapImage->getPixel(j, i, 0)) / 256.0;
          float pixel2 = (float)(heightmapImage->getPixel(j, i+1, 0)) / 256.0;

          tri_positions[index_pos_tri] = (float)j / 256.0 - 0.5;
          tri_positions[index_pos_tri+1] = (float)i / 256.0 - 0.5;
          tri_positions[index_pos_tri+2] = pixel1 / 2;
          tri_positions[index_pos_tri+3] = (float)j / 256.0 - 0.5;
          tri_positions[index_pos_tri+4] = (float)(i + 1) / 256.0 - 0.5;
          tri_positions[index_pos_tri+5] = pixel2 / 2;

          tri_colors[index_col_tri] = 0; //pixel1;
          tri_colors[index_col_tri+1] = pixel1;
          tri_colors[index_col_tri+2] = pixel1;
          tri_colors[index_col_tri+3] = 1.0f;
          tri_colors[index_col_tri+4] = 0; //pixel2;
          tri_colors[index_col_tri+5] = pixel2;
          tri_colors[index_col_tri+6] = pixel2;
          tri_colors[index_col_tri+7] = 1.0f;
          
          index_pos_tri += 6;
          index_col_tri += 8;
        }

        if(i + 2 <= imageWidth - 1) {
          for(int j=imageWidth-1; j>=0; --j) {
            float pixel1 = (float)(heightmapImage->getPixel(j, i+1, 0)) / 256.0;
            float pixel2 = (float)(heightmapImage->getPixel(j, i+2, 0)) / 256.0;

            tri_positions[index_pos_tri] = (float)j / 256.0 - 0.5;
            tri_positions[index_pos_tri+1] = (float)(i + 1) / 256.0 - 0.5;
            tri_positions[index_pos_tri+2] = pixel1 / 2;
            tri_positions[index_pos_tri+3] = (float)j / 256.0 - 0.5;
            tri_positions[index_pos_tri+4] = (float)(i + 2) / 256.0 - 0.5;
            tri_positions[index_pos_tri+5] = pixel2 / 2;

            tri_colors[index_col_tri] = 0; //pixel1;
            tri_colors[index_col_tri+1] = pixel1;
            tri_colors[index_col_tri+2] = pixel1;
            tri_colors[index_col_tri+3] = 1.0f;
            tri_colors[index_col_tri+4] = 0; //pixel2;
            tri_colors[index_col_tri+5] = pixel2;
            tri_colors[index_col_tri+6] = pixel2;
            tri_colors[index_col_tri+7] = 1.0f;

            index_pos_tri += 6;
            index_col_tri += 8;
          }
        }
      }
    }

    glEnable(GL_DEPTH_TEST);
    matrix = new OpenGLMatrix();
    initVBO();
    initPipelineProgram();
    initVAO(); 
}

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    cout << "The arguments are incorrect." << endl;
    cout << "usage: ./hw1 <heightmap file>" << endl;
    exit(EXIT_FAILURE);
  }

  cout << "Initializing GLUT..." << endl;
  glutInit(&argc,argv);

  cout << "Initializing OpenGL..." << endl;

  #ifdef __APPLE__
    glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
  #else
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
  #endif

  glutInitWindowSize(windowWidth, windowHeight);
  glutInitWindowPosition(0, 0);  
  glutCreateWindow(windowTitle);

  cout << "OpenGL Version: " << glGetString(GL_VERSION) << endl;
  cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << endl;
  cout << "Shading Language Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

  // tells glut to use a particular display function to redraw 
  glutDisplayFunc(displayFunc);
  // perform animation inside idleFunc
  glutIdleFunc(idleFunc);
  // callback for mouse drags
  glutMotionFunc(mouseMotionDragFunc);
  // callback for idle mouse movement
  glutPassiveMotionFunc(mouseMotionFunc);
  // callback for mouse button changes
  glutMouseFunc(mouseButtonFunc);
  // callback for resizing the window
  glutReshapeFunc(reshapeFunc);
  // callback for pressing the keys on the keyboard
  glutKeyboardFunc(keyboardFunc);
  //
  glutSpecialFunc(keyboardSpecialInputFunc);

  // init glew
  #ifdef __APPLE__
    // nothing is needed on Apple
  #else
    // Windows, Linux
    GLint result = glewInit();
    if (result != GLEW_OK)
    {
      cout << "error: " << glewGetErrorString(result) << endl;
      exit(EXIT_FAILURE);
    }
  #endif

  // do initialization
  initScene(argc, argv);

  // sink forever into the glut loop
  glutMainLoop();
}