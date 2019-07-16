#include "TestScene.h"
#include "../ResourceManager.h"
#include <iostream>

namespace rhythmus
{

void TestScene::Render()
{
  glLoadIdentity();
  glTranslatef(-3.0f, 0.0f, -6.0f);
  glBindTexture(GL_TEXTURE_2D, img_->GetTextureID());
  glBegin(GL_QUADS);
  glTexCoord2f(0.0, 0.0);
  glVertex3f(0.0, 0.0, 0.0);
  glTexCoord2f(1.0, 0.0);
  glVertex3f(1.0, 0.0, 0.0);
  glTexCoord2f(1.0, 1.0);
  glVertex3f(1.0, 1.0, 0.0);
  glTexCoord2f(0.0, 1.0);
  glVertex3f(0.0, 1.0, 0.0);
  glEnd();
}

void TestScene::LoadScene()
{
  img_ = ResourceManager::getInstance().LoadImage("test.png");
  img_->CommitImage();
}

void TestScene::CloseScene()
{
}

}