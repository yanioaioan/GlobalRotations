#include "NGLScene.h"
#include <QGuiApplication>
#include <QMouseEvent>

#include <ngl/Camera.h>
#include <ngl/Light.h>
#include <ngl/Material.h>
#include <ngl/NGLInit.h>
#include <ngl/NGLStream.h>
#include <ngl/ShaderLib.h>
#include <ngl/VAOPrimitives.h>

#include <ngl/VAOFactory.h>
#include <ngl/SimpleIndexVAO.h>

static bool aroundX;
static bool aroundY;
static bool aroundZ;


NGLScene::NGLScene()
{
  setTitle( "Qt5 Simple NGL Demo" );
  aroundX=false;
  aroundY=false;
  aroundZ=false;
}


NGLScene::~NGLScene()
{
  std::cout << "Shutting down NGL, removing VAO's and Shaders\n";
}



void NGLScene::resizeGL( int _w, int _h )
{
  m_cam.setShape( 45.0f, static_cast<float>( _w ) / _h, 0.05f, 350.0f );
  m_win.width  = static_cast<int>( _w * devicePixelRatio() );
  m_win.height = static_cast<int>( _h * devicePixelRatio() );
}


constexpr auto ColourShader="ColourShader";
constexpr auto ColourVertex="ColourVertex";
constexpr auto ColourFragment="ColourFragment";

void NGLScene::initializeGL()
{
  // we must call that first before any other GL commands to load and link the
  // gl commands from the lib, if that is not done program will crash
  ngl::NGLInit::instance();
  glClearColor( 0.4f, 0.4f, 0.4f, 1.0f ); // Grey Background
  // enable depth testing for drawing
  glEnable( GL_DEPTH_TEST );
// enable multisampling for smoother drawing
#ifndef USINGIOS_
  glEnable( GL_MULTISAMPLE );
#endif
  // now to load the shader and set the values
  // grab an instance of shader manager
  ngl::ShaderLib* shader = ngl::ShaderLib::instance();
  // we are creating a shader called Phong to save typos
  // in the code create some constexpr
  constexpr auto shaderProgram = "Phong";
  constexpr auto vertexShader  = "PhongVertex";
  constexpr auto fragShader    = "PhongFragment";
  // create the shader program
  shader->createShaderProgram( shaderProgram );
  // now we are going to create empty shaders for Frag and Vert
  shader->attachShader( vertexShader, ngl::ShaderType::VERTEX );
  shader->attachShader( fragShader, ngl::ShaderType::FRAGMENT );
  // attach the source
  shader->loadShaderSource( vertexShader, "shaders/PhongVertex.glsl" );
  shader->loadShaderSource( fragShader, "shaders/PhongFragment.glsl" );
  // compile the shaders
  shader->compileShader( vertexShader );
  shader->compileShader( fragShader );
  // add them to the program
  shader->attachShaderToProgram( shaderProgram, vertexShader );
  shader->attachShaderToProgram( shaderProgram, fragShader );


  // now we have associated that data we can link the shader
  shader->linkProgramObject( shaderProgram );
  // and make it active ready to load values
  ( *shader )[ shaderProgram ]->use();
  // the shader will use the currently active material and light0 so set them
  ngl::Material m( ngl::STDMAT::GOLD );
  // load our material values to the shader into the structure material (see Vertex shader)
  m.loadToShader( "material" );
  // Now we will create a basic Camera from the graphics library
  // This is a static camera so it only needs to be set once
  // First create Values for the camera position
  ngl::Vec3 from( 0, 1, 5 );
  ngl::Vec3 to( 0, 0, 0 );
  ngl::Vec3 up( 0, 1, 0 );
  // now load to our new camera
  m_cam.set( from, to, up );
  // set the shape using FOV 45 Aspect Ratio based on Width and Height
  // The final two are near and far clipping planes of 0.5 and 10
  m_cam.setShape( 45.0f, 720.0f / 576.0f, 0.05f, 350.0f );
  shader->setUniform( "viewerPos", m_cam.getEye().toVec3() );
  // now create our light that is done after the camera so we can pass the
  // transpose of the projection matrix to the light to do correct eye space
  // transformations
  ngl::Mat4 iv = m_cam.getViewMatrix();
  iv.transpose();
  ngl::Light light( ngl::Vec3( -2, 5, 2 ), ngl::Colour( 1, 1, 1, 1 ), ngl::Colour( 1, 1, 1, 1 ), ngl::LightModes::POINTLIGHT );
  light.setTransform( iv );
  // load these values to the shader as well
  light.loadToShader( "light" );


  //init pos of model matrix
  currrentTrans.setRotation(0,0,0);
  currModelMAtrix=currrentTrans.getMatrix();


  // grab an instance of shader manager
      // load a frag and vert shaders

      shader->createShaderProgram("ColourShader");

    shader->attachShader(ColourVertex,ngl::ShaderType::VERTEX);
    shader->attachShader(ColourFragment,ngl::ShaderType::FRAGMENT);
    shader->loadShaderSource(ColourVertex,"shaders/ColourVertex.glsl");
    shader->loadShaderSource(ColourFragment,"shaders/ColourFragment.glsl");

    shader->compileShader(ColourVertex);
    shader->compileShader(ColourFragment);
    shader->attachShaderToProgram(ColourShader,ColourVertex);
    shader->attachShaderToProgram(ColourShader,ColourFragment);


    shader->linkProgramObject(ColourShader);

    buildVAO();



    //now the textured cube

    shader->createShaderProgram("TextureShader");

      shader->attachShader("SimpleVertex",ngl::ShaderType::VERTEX);
      shader->attachShader("SimpleFragment",ngl::ShaderType::FRAGMENT);
      shader->loadShaderSource("SimpleVertex","shaders/TextureVert.glsl");
      shader->loadShaderSource("SimpleFragment","shaders/TextureFrag.glsl");

      shader->compileShader("SimpleVertex");
      shader->compileShader("SimpleFragment");
      shader->attachShaderToProgram("TextureShader","SimpleVertex");
      shader->attachShaderToProgram("TextureShader","SimpleFragment");


      shader->linkProgramObject("TextureShader");
      createCube(0.2);
      loadTexture("textures/crate.bmp",m_textureNameArray[0]);


      m_text.reset( new ngl::Text(QFont("Arial",14)));
      m_text->setScreenSize(width(),height());

}

static int incr;
//current matrix derived somehow


void NGLScene::loadMatricesToShader()
{
  ngl::ShaderLib* shader = ngl::ShaderLib::instance();

  ngl::Mat4 MV;
  ngl::Mat4 MVP;
  ngl::Mat3 normalMatrix;
  ngl::Mat4 M;


  //these rotations are applied on top of the exist orientation, either locally or globally based on pre or post multiplication
  int xRot=45;
  int yRot=45;
  int zRot=45;


  if(m_win.mouseDown)//stop applying global transform till we key press again
  {
      applyGlobalTrans.reset();
  }

  ngl::Mat4 toGlobal=applyGlobalTrans.getMatrix();

  ngl::Mat4 toLocal=applyGlobalTrans.getMatrix();

  //uncomment for /globallocal rotations accordingly
  M            = currModelMAtrix*toGlobal;
  currModelMAtrix=M;//save previous transformed matrix

//  M            = toLocal*currModelMAtrix;
  MV           = M*m_mouseGlobalTX * m_cam.getViewMatrix();
  MVP          = M*m_mouseGlobalTX * m_cam.getVPMatrix();

  normalMatrix = MV;
  normalMatrix.inverse();
  shader->setUniform( "MV", MV );

  shader->setUniform( "MVP", MVP  );
  shader->setUniform( "normalMatrix", normalMatrix );
  shader->setUniform( "M", M );
}


void NGLScene::loadTexture(const char * path, GLuint & texId)
{
  QImage image;
  bool loaded=image.load("textures/crate.bmp");
  if(loaded == true)
  {
    int width=image.width();
    int height=image.height();

    unsigned char *data = new unsigned char[ width*height*3];
    unsigned int index=0;
    QRgb colour;
    for( int y=0; y<height; ++y)
    {
      for( int x=0; x<width; ++x)
      {
        colour=image.pixel(x,y);

        data[index++]=qRed(colour);
        data[index++]=qGreen(colour);
        data[index++]=qBlue(colour);
      }
    }


  glGenTextures(1,&texId);
  glBindTexture(GL_TEXTURE_2D,texId);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);

  glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,width,height,0,GL_RGB,GL_UNSIGNED_BYTE,data);

  glGenerateMipmap(GL_TEXTURE_2D); //  Allocate the mipmaps

  }
}


//note the creation of the cube is pure opengl (not wrapped in NGL)
void NGLScene::createCube( GLfloat _scale )
{

   // vertex coords array
  GLfloat vertices[] = {
                        -1,1,-1,1,1,-1,1,-1,-1, -1,1,-1,-1,-1,-1,1,-1,-1, //back
                        -1,1,1,1,1,1,1,-1,1, -1,-1,1, 1,-1,1,-1,1,1,  //front
                        -1,1,-1, 1,1,-1, 1,1,1, -1,1,1, 1,1,1, -1,1,-1, // top
                        -1,-1,-1, 1,-1,-1, 1,-1,1, -1,-1,1, 1,-1,1, -1,-1,-1, // bottom
                        -1,1,-1,-1,1,1,-1,-1,-1, -1,-1,-1,-1,-1,1,-1,1,1, // left
                        1,1,-1,1,1,1,1,-1,-1, 1,-1,-1,1,-1,1,1,1,1, // left

                        };
  GLfloat texture[] = {
                        0,0,0,1,1,1 ,0,0,1,0,1,1, //back
                        0,1,1,0,1,1 ,0,0,1,0,0,1, // front
                        0,0,1,0,1,1 ,0,1,1,1,0,0, //top
                        0,0,1,0,1,1 ,0,1,1,1,0,0, //bottom
                        1,0,1,1,0,0 ,0,0,0,1,1,1, // left
                        1,0,1,1,0,0 ,0,0,0,1,1,1, // right

                        };


  std::cout<<sizeof(vertices)/sizeof(GLfloat)<<"\n";
  // first we scale our vertices to _scale
  for(uint i=0; i<sizeof(vertices)/sizeof(GLfloat); ++i)
  {
    vertices[i]*=_scale;
  }

  // first we create a vertex array Object
  glGenVertexArrays(1, &m_vaoID);

  // now bind this to be the currently active one
  glBindVertexArray(m_vaoID);
  // now we create two VBO's one for each of the objects these are only used here
  // as they will be associated with the vertex array object
  GLuint vboID[2];
  glGenBuffers(2, &vboID[0]);
  // now we will bind an array buffer to the first one and load the data for the verts
  glBindBuffer(GL_ARRAY_BUFFER, vboID[0]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices)*sizeof(GLfloat), vertices, GL_STATIC_DRAW);
  // now we bind the vertex attribute pointer for this object in this case the
  // vertex data
  glVertexAttribPointer(0,3,GL_FLOAT,false,0,0);
  glEnableVertexAttribArray(0);
  // now we repeat for the UV data using the second VBO
  glBindBuffer(GL_ARRAY_BUFFER, vboID[1]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(texture)*sizeof(GLfloat), texture, GL_STATIC_DRAW);
  glVertexAttribPointer(1,2,GL_FLOAT,false,0,0);
  glEnableVertexAttribArray(1);

}



void NGLScene::buildVAO()
{
    // create a vao as a series of GL_TRIANGLES
      m_vao.reset( ngl::VAOFactory::createVAO("simpleIndexVAO",GL_LINES));
      m_vao->bind();


      const static GLubyte indices[]=  {
                                          0,1
                                       };

       GLfloat vertices[] = {0,0,0,
                             0,0,5
                            };

       GLfloat colours[]={
                            1,0,0
                          };
       // in this case we are going to set our data as the vertices above

       m_vao->setData(ngl::SimpleIndexVAO::VertexData( 6*sizeof(GLfloat),vertices[0],sizeof(indices),&indices[0],GL_UNSIGNED_BYTE,GL_STATIC_DRAW));
       // now we set the attribute pointer to be 0 (as this matches vertIn in our shader)
       m_vao->setVertexAttributePointer(0,3,GL_FLOAT,0,0);

       m_vao->setData(ngl::SimpleIndexVAO::VertexData(3*sizeof(GLfloat),colours[0],sizeof(indices),&indices[0],GL_UNSIGNED_BYTE,GL_STATIC_DRAW));
       // now we set the attribute pointer to be 1 (as this matches inColour in our shader)
       m_vao->setVertexAttributePointer(1,3,GL_FLOAT,0,0);
       m_vao->setNumIndices(sizeof(indices));
     // now unbind
      m_vao->unbind();


}



void NGLScene::paintGL()
{
  glViewport( 0, 0, m_win.width, m_win.height );
  // clear the screen and depth buffer
  glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

  // grab an instance of the shader manager
  ngl::ShaderLib* shader = ngl::ShaderLib::instance();
  ( *shader )[ "Phong" ]->use();

  // Rotation based on the mouse position for our global transform
  ngl::Mat4 rotX;
  ngl::Mat4 rotY;
  // create the rotation matrices
  rotX.rotateX( m_win.spinXFace );
  rotY.rotateY( m_win.spinYFace );
  // multiply the rotations
  m_mouseGlobalTX = rotY * rotX;
  // add the translations
  m_mouseGlobalTX.m_m[ 3 ][ 0 ] = m_modelPos.m_x;
  m_mouseGlobalTX.m_m[ 3 ][ 1 ] = m_modelPos.m_y;
  m_mouseGlobalTX.m_m[ 3 ][ 2 ] = m_modelPos.m_z;

  // get the VBO instance and draw the built in teapot
  ngl::VAOPrimitives* prim = ngl::VAOPrimitives::instance();
  // draw


//  loadMatricesToShader();
//  prim->draw( "teapot" );


  (*shader)["ColourShader"]->use();
   ngl::Mat4 MVP;
   ngl::Transformation t;
   t.setRotation(0,0,0);
   t.setPosition(0,0,0);
   MVP=t.getMatrix()*m_mouseGlobalTX*m_cam.getVPMatrix();

   shader->setRegisteredUniform("setcolor",ngl::Vec3(0,0,1));
   shader->setShaderParamFromMat4("MVP",MVP);

   m_vao->bind();
   m_vao->draw();
   m_vao->unbind();

   t.setRotation(90,0,0);
   MVP=t.getMatrix()*m_mouseGlobalTX*m_cam.getVPMatrix();
   shader->setRegisteredUniform("setcolor",ngl::Vec3(0,1,0));
   shader->setShaderParamFromMat4("MVP",MVP);

   m_vao->bind();
   m_vao->draw();
   m_vao->unbind();


   t.setRotation(0,90,0);
   MVP=t.getMatrix()*m_mouseGlobalTX*m_cam.getVPMatrix();
   shader->setRegisteredUniform("setcolor",ngl::Vec3(1,0,0));
   shader->setShaderParamFromMat4("MVP",MVP);

   m_vao->bind();
   m_vao->draw();
   m_vao->unbind();


   //now draw the textured cube
   shader->use("TextureShader");
   loadMatricesToShader();

    // now we bind back our vertex array object and draw
    glBindVertexArray(m_vaoID);		// select first VAO

    // need to bind the active texture before drawing
    glBindTexture(GL_TEXTURE_2D,m_textureNameArray[0]);
    glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
    glDrawArrays(GL_TRIANGLES, 0,36 );	// draw object


    QString text;
    if (aroundX)
    {
        m_text->setColour(1,0,0);
        text=QString("drawing Back & Forth around Global X");
    }
    else if (aroundY)
    {
        m_text->setColour(0,1,0);
        text=QString("drawing Back & Forth around Global Y");
    }
    else if (aroundZ)
    {
        m_text->setColour(0,0,1);
        text=QString("drawing Back & Forth around Global Z");
    }
    m_text->renderText(10,20,text);


}

//----------------------------------------------------------------------------------------------------------------------
static int Yrot;
static int Zrot;


void NGLScene::keyPressEvent( QKeyEvent* _event )
{
  // that method is called every time the main window recives a key event.
  // we then switch on the key value and set the camera in the GLWindow
  switch ( _event->key() )
  {
    // escape key to quit
    case Qt::Key_Escape:
      QGuiApplication::exit( EXIT_SUCCESS );
      break;
//  case Qt::Key_Left:
//      Yrot++;
//      applyGlobalTrans.setRotation(0,Yrot,Zrot);
//      break;
//  case Qt::Key_Up:
//      Zrot++;
//      applyGlobalTrans.setRotation(0,Yrot,Zrot);
//    break;
// turn on wirframe rendering
#ifndef USINGIOS_
    case Qt::Key_W:
      glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
      break;
    // turn off wire frame
    case Qt::Key_S:
      glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
      break;
#endif
    // show full screen
    case Qt::Key_F:
      showFullScreen();
      break;
    // show windowed
    case Qt::Key_N:
      showNormal();
      break;
    case Qt::Key_Space :
      m_win.spinXFace=0;
      m_win.spinYFace=0;
      m_modelPos.set(ngl::Vec3::zero());
    break;

  case Qt::Key_Left :
        {
         applyGlobalTrans.setRotation(0,0,-Rot);
         aroundX=false;aroundY=false;aroundZ=true;
            break;
        }
  case Qt::Key_Right :
  {
      applyGlobalTrans.setRotation(0,0,Rot);
      aroundX=false;aroundY=false;aroundZ=true;
         break;
     }
  break;

  case Qt::Key_Up :
    {applyGlobalTrans.setRotation(-Rot,0,0);
        aroundX=true;aroundY=false;aroundZ=false;
         break;
     }
  case Qt::Key_Down :
  {applyGlobalTrans.setRotation(Rot,0,0);
      aroundX=true;aroundY=false;aroundZ=false;
       break;
   }

  case Qt::Key_K :
        {applyGlobalTrans.setRotation(0,-Rot,0);
      aroundX=false;aroundY=true;aroundZ=false;
       break;
   }
  case Qt::Key_L :
  {applyGlobalTrans.setRotation(0,Rot,0);
      aroundX=false;aroundY=true;aroundZ=false;
       break;
   }


    default:
      break;
  }
  update();
}
