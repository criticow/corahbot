#include "application.hpp"

Temporizer Application::tempo;
bool Application::vSyncEnabled = false;

Application::Application(int width, int height, const char *title)
{
  this->window = Window(width, height, title, false);
  this->window.setUserPointer();
  this->window.setIcon("data/icons/window");

  cv::utils::logging::setLogLevel(cv::utils::logging::LogLevel::LOG_LEVEL_ERROR);
}

void Application::run()
{
  std::thread appLoop(&Application::appLoop, this);

  while(this->window.isOpen())
  {
    this->window.waitEvents();
  }

  this->destroy();

  appLoop.join();
}

void Application::toggleVsync()
{
  vSyncEnabled = !vSyncEnabled;
  glfwSwapInterval(vSyncEnabled);
}

void Application::render()
{
}

void Application::update()
{
  this->tempo.update();
  this->input.update(this->window);
  this->window.updateViewport();
}

void Application::setup()
{
  this->gui.setup(this->window);
  this->gui.init();
  this->toggleVsync();
  Store::loadMarkers();
}

void Application::appLoop()
{
  this->window.loadOpenGL();
  this->setup();

  while(this->window.isOpen())
  {
    this->update();
    this->window.clear();
    this->gui.update();
    this->gui.render();
    this->render();
    this->window.swapBuffers();
  }
}

void Application::destroy()
{
  this->gui.cleanup();
  this->window.destroy();
}
