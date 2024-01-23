#include "application.hpp"

Temporizer Application::tempo;

Application::Application(int width, int height, const char *title)
{
  this->window = Window(width, height, title, true);
  this->window.setUserPointer();

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
  Store::loadMarkers();
}

void Application::appLoop()
{
  this->window.loadOpenGL();
  // glfwSwapInterval(1);
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
