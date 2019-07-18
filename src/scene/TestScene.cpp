#include "TestScene.h"
#include "../ResourceManager.h"
#include <iostream>

namespace rhythmus
{

TestScene::TestScene()
  : vertex_array(0)
{
}

void TestScene::Render()
{
  /**
   * glBegin / glEnd is not supported API nowadays.
   */

  //glTranslatef(-3.0f, 0.0f, -6.0f);
  glColor3f(0.5f, 0.5f, 0.5f);
  glBindTexture(GL_TEXTURE_2D, img_->GetTextureID());
  glBindVertexArray(vertex_array);
  glDrawArrays(GL_TRIANGLES, 0, 3);
}

void TestScene::LoadScene()
{
  img_ = ResourceManager::getInstance().LoadImage("test.png");
  img_->CommitImage();

  float position[] = {
    0.0f,  0.5f, 0.0f,
    0.5f, -0.5f, 0.0f,
    -0.5f, -0.5f, 0.0f
  };

  float color[] = {
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 1.0f
  };

  float textureCoordinate[] = {
    0.5f, 1.0f,
    1.0f, 0.0f,
    0.0f, 0.0f
  };

  int errcode = 0;
  ASSERT_GL_VAL(errcode);

  /**
   * Add buffer
   */
  glGenBuffers(1, &vertex_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(position), position, GL_STATIC_DRAW);
  ASSERT_GL_VAL(errcode);

  /**
   * Add vertex pointer
   */
  GLuint vpos_location = 0, vcol_location = 0;
  glGenVertexArrays(1, &vertex_array);
  glBindVertexArray(vertex_array);
  glEnableVertexAttribArray(vpos_location);
  glVertexAttribPointer(vpos_location, 3, GL_FLOAT, GL_FALSE,
    0, (void*)0);
  ASSERT_GL_VAL(errcode);
}

void TestScene::CloseScene()
{
}

}