#include "application.hpp"

Application::Application(int width, int height, const char *title)
{
  this->window = Window(width, height, title, true);
  this->window.setUserPointer();
  this->gui = GUI(this);
  this->emulator.list();
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
  this->time.update();
  this->input.update(this->window);
  this->window.updateViewport();
}

void Application::setup()
{
  this->gui.setup(this->window);
  this->gui.addFonts();
}

void Application::appLoop()
{
  this->window.loadOpenGL();
  this->setup();

  while(this->window.isOpen())
  {
    this->update();
    this->window.clear();
    this->gui.render();
    this->render();
    this->window.swapBuffers();
  }
}

void Application::destroy()
{
  this->emulator.destroy();
  this->gui.destroy();
  this->window.destroy();
}
