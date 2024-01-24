#include "application.hpp"

#ifdef _WIN32
  // For Windows, use WinMain for release build to suppress the console window
  #if defined(NDEBUG)
    int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
    {
      Application app(600, 600, "Corahbot");
      app.run();
      return 0;
    }
  #else
    int main()
    {
      // For other build configurations (Debug, etc.), use the original main
      Application app(600, 600, "Corahbot");
      app.run();
      return 0;
    }
  #endif
#endif