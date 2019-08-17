#include "Scene.h"

namespace rhythmus
{

// -------------------------- SceneLoader

SceneLoader::SceneLoader(Scene *s)
  : scene_(s)
{
}

SceneLoader::~SceneLoader()
{
}

void SceneLoader::GetThemeOptions(std::vector<ThemeOption>& theme_options)
{
  theme_options.swap(theme_options_);

}

void SceneLoader::GetSpriteObjects(std::vector<SpriteAuto>& sprites)
{
  sprites.swap(sprites_);
}

void SceneLoader::GetImages(std::vector<ImageAuto>& images)
{
  images.swap(images_);
}

const ThemeParameter& SceneLoader::GetThemeParameter()
{
  return theme_param_;
}


// -------------------------------- Scene

void Scene::LoadScene(SceneLoader *scene_loader)
{
  scene_loader->GetImages(images_);
  scene_loader->GetSpriteObjects(sprites_);
  scene_loader->GetThemeOptions(theme_options_);
  theme_param_ = scene_loader->GetThemeParameter();
}

void Scene::LoadOptions()
{
  // TODO
}

void Scene::SaveOptions()
{
  // TODO
}

void Scene::GetThemeOptionSelectList(
  const ThemeOption &option, std::vector<std::string>& selectable)
{
}

}