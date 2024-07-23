#include "emulator.hpp"
#include "bot.hpp"

std::string console = "C:/Nox/bin/NoxConsole.exe";

const int EMULATOR_WIDTH = 384;
const int EMULATOR_HEIGHT = 642;

BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam) {
  if (IsWindowVisible(hWnd)) {
    char title[256];
    GetWindowText(hWnd, title, sizeof(title));

    DWORD processId;
    GetWindowThreadProcessId(hWnd, &processId);

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if (hProcess != NULL) {
      char processName[MAX_PATH];
      GetModuleBaseNameA(hProcess, NULL, processName, sizeof(processName));

      if(strcmp(processName, "Nox.exe") == 0 && strcmp(title, "Nox") != 0)
      {
        std::vector<std::string> *list = reinterpret_cast<std::vector<std::string>*>(lParam);
        list->insert(list->begin(), title);
      }

      CloseHandle(hProcess);
    }
  }

  return TRUE;
}

void Emulator::arrange(int monitor, int cols)
{
  std::vector<std::string> instances = Emulator::list();
  std::sort(instances.begin(), instances.end());

  int x = 0;
  int y = 0;

  if(monitor == 2)
  {
    x = -210;
    y = -1080;
  }
  
  int counter = 0;
  int index = 0;

  for(auto &instance : instances)
  {
    if(counter == cols && index < 8)
    {
      counter = 0;
      y += 380;
    }

    int xPos = x + (EMULATOR_WIDTH * counter);
    setPosition(instance, {xPos, y});
    counter++;
    index++;
  }
}

glm::ivec2 Emulator::getPosition(const std::string &windowTitle)
{
  glm::ivec2 position(0);
  HWND hwnd = FindWindow(nullptr, windowTitle.c_str());
  if(!hwnd)
    return position;
  
  RECT windowRect;
  GetWindowRect(hwnd, &windowRect);

  position.x = windowRect.left;
  position.y = windowRect.top;

  return position;
}

void Emulator::setPosition(const std::string &windowTitle, glm::ivec2 position)
{
  HWND hwnd = FindWindow(nullptr, windowTitle.c_str());
  if(hwnd)
  {
    SetWindowPos(hwnd, HWND_TOP, position.x, position.y, EMULATOR_WIDTH, EMULATOR_HEIGHT, SWP_NOZORDER); // Brings window to top
    SetForegroundWindow(hwnd);
  }
}

// std::vector<std::string> Emulator::listInstances()
// {
//   std::vector<std::string> instances;
//   EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&instances));
//   return instances;
// }

std::vector<std::string> Emulator::list()
{
  std::string res = util::parseCMD(console, {"list"});
  std::vector<std::string> list;

  for(auto &line : util::split(res, '\n'))
  {
    // If it does not end with -1,-1 it is running
    if(!line.ends_with("-1,-1"))
    {
      std::vector<std::string> values = util::split(line, ',');
      list.push_back(values[2]);
    }
  }

  return list;
}

void Emulator::launch(int index)
{
  std::string res = util::parseCMD(console, {"launch", std::format("-index:{}", index)});

  if(!res.empty())
  {
    LOGGER_DEBUG(res);
  }
}

void Emulator::quit(const std::string &windowTitle)
{
  std::string res = util::parseCMD(console, {"quit", "-name:" + windowTitle});

  if(!res.empty())
  {
    LOGGER_DEBUG(res);
  }
}

void Emulator::reboot(const std::string &windowTitle)
{
  std::string res = util::parseCMD(console, {"reboot", "-name:" + windowTitle});

  if(!res.empty())
  {
    LOGGER_DEBUG(res);
  }
}

void Emulator::click(const std::string &windowTitle, glm::ivec2 point)
{
  HWND hWnd = FindWindow(nullptr, windowTitle.c_str());

  LPARAM lParam = MAKELPARAM(point.x, point.y);

  // Send left mouse button down message
  SendMessage(hWnd, WM_LBUTTONDOWN, MK_LBUTTON, lParam);

  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  // Send left mouse button up message
  SendMessage(hWnd, WM_LBUTTONUP, 0, lParam);

  // LOGGER_DEBUG("Clicked: {}, {}", point.x, point.y);
}

void Emulator::click(const std::string &windowTitle, glm::ivec4 point)
{
  int x = Random::choose(point.x, point.x + point.z);
  int y = Random::choose(point.y, point.y + point.w);
  click(windowTitle, glm::ivec2(x, y));
}

void Emulator::click(const std::string & windowTitle, Marker & marker)
{
  int x = Random::choose(marker.x, marker.x + marker.width);
  int y = Random::choose(marker.y, marker.y + marker.height);
  click(windowTitle, glm::ivec2(x, y));
}

void Emulator::drag(const std::string &windowTitle, glm::ivec2 start, glm::ivec2 end)
{
  HWND hWnd = FindWindow(nullptr, windowTitle.c_str());

  LPARAM startParam = MAKELPARAM(start.x, start.y);

  // Send left mouse button down message
  SendMessage(hWnd, WM_LBUTTONDOWN, MK_LBUTTON, startParam);

  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  LPARAM endParam;

  int numSteps = 10;

  for(size_t i = 1; i <= numSteps; i++)
  {
    int newX = start.x + i * (end.x - start.x) / numSteps;
    int newY = start.y + i * (end.y - start.y) / numSteps;

    endParam = MAKELPARAM(newX, newY);
    SendMessage(hWnd, WM_MOUSEMOVE, MK_LBUTTON, endParam);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  // Send left mouse button up message
  SendMessage(hWnd, WM_LBUTTONUP, 0, endParam);
}

std::pair<bool, glm::ivec4> Emulator::find(cv::Mat haystack, cv::Mat needle, float threshold = 0.8)
{
  std::pair matchResult = std::make_pair(false, glm::ivec4(0));

  cv::Mat result;
  cv::matchTemplate(haystack, needle, result, cv::TM_CCOEFF_NORMED);

  // Locate the best match
  cv::Point minLoc, maxLoc;
  cv::minMaxLoc(result, nullptr, nullptr, &minLoc, &maxLoc);

  // float res = result.at<float>(maxLoc);
  // std::cout << res << std::endl;

  if (result.at<float>(maxLoc) > threshold) {
    matchResult.first = true;
    matchResult.second = glm::ivec4(maxLoc.x, maxLoc.y, needle.cols, needle.rows);
  }

  return matchResult;
}

std::pair<bool, glm::ivec4> Emulator::find(const std::string &windowTitle, Marker marker , const std::string &haystackPath)
{
  cv::Mat image = printscreen(windowTitle, marker.x, marker.y, marker.width, marker.height);

  cv::Mat needle;
  cv::cvtColor(image, needle, cv::COLOR_BGR2GRAY);

  cv::Mat haystack = cv::imread("data/images/" + haystackPath + ".png", cv::IMREAD_GRAYSCALE);

  return find(haystack, needle);
}

std::pair<bool, glm::ivec4> Emulator::find(const std::string &windowTitle, const std::string &needlePath, float threshold = 0.8)
{
  cv::Mat needle = cv::imread(needlePath, cv::IMREAD_GRAYSCALE);
  cv::Mat haystack;
  cv::cvtColor(printscreen(windowTitle), haystack, cv::COLOR_BGR2GRAY);

  return find(haystack, needle);
}

void Emulator::drawRectangles(cv::Mat &canvas, const std::vector<glm::ivec4> &points)
{
  for(auto &point : points)
  {
    cv::Rect roiRect(point.x, point.y, point.z, point.w);
    cv::rectangle(canvas, roiRect, cv::Scalar(0, 255, 0), 2); // Green rectangle

    int centerX = point.x + point.z / 2;
    int centerY = point.y + point.w / 2;

    cv::circle(canvas, cv::Point(centerX, centerY), 5, cv::Scalar(255, 0, 0), -1); // Blue circle
  }
}

bool Emulator::compareImages(cv::Mat &image1, cv::Mat &image2)
{
  if(image1.empty() || image2.empty())
  {
    return false;
  }

  cv::Mat diff;
  cv::absdiff(image1, image2, diff);
  cv::Scalar sum = cv::sum(diff);

#ifdef NDEBUG // For dist
  // Set a threshold (adjust as needed)
  int threshold = 100;

  // If the sum of differences is below the threshold for all channels, consider the images as the same
  return sum[0] < threshold && sum[1] < threshold && sum[2] < threshold && sum[3] < threshold;
#else // For debug
  // If the sum of differences is zero, the images are the same
  return sum[0] == 0 && sum[1] == 0 && sum[2] == 0 && sum[3] == 0;
#endif
}

bool Emulator::compareImages(const std::string &windowTitle, Marker marker)
{
  cv::Mat image1 = cv::imread("data/images/" + marker.location + "/" + marker.name + ".png");
  cv::Mat image2 = printscreen(windowTitle, marker.x, marker.y, marker.width, marker.height);

  return compareImages(image1, image2);
}

void Emulator::runapp(const std::string &windowTitle, const std::string &packageName)
{
  std::string res = util::parseCMD(console, {"runapp", "-name:" + windowTitle, "-packagename:" + packageName});

  if(!res.empty())
  {
    LOGGER_DEBUG(res);
  }
}

void Emulator::killappall(const std::string & windowTitle)
{
  std::string res = util::parseCMD(console, {"killappall", "-name:" + windowTitle});

  if(!res.empty())
  {
    LOGGER_DEBUG(res);
  }
}

void Emulator::killapp(const std::string &windowTitle, const std::string &packageName)
{
  std::string res = util::parseCMD(console, {"killapp", "-name:" + windowTitle, "-packagename:" + packageName});

  if(!res.empty())
  {
    LOGGER_DEBUG(res);
  }
}

cv::Mat Emulator::printscreen(const std::string & windowTitle)
{
  // Find the Nox App Player window by its title
  HWND hwnd = FindWindow(nullptr, windowTitle.c_str());

  // Empty cv::Mat if window is closed
  if(!hwnd)
  {
    return cv::Mat();
  }

  // Get the device context of the window
  HDC hdcScreen = GetDC(hwnd);
  HDC hdcMem = CreateCompatibleDC(hdcScreen);

  // Get the dimensions of the window
  RECT rect;
  GetClientRect(hwnd, &rect);

  int width = rect.right - rect.left;
  int height = rect.bottom - rect.top;

  // Empty cv::Mat if window is minimized
  if(width == 0 || height == 0)
  {
    return cv::Mat();
  }

  // Create a bitmap compatible with the window's device context
  HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, width, height);
  SelectObject(hdcMem, hBitmap);

  // Copy the window's content to the bitmap
  BitBlt(hdcMem, 0, 0, width, height, hdcScreen, 0, 0, SRCCOPY);

  BITMAPINFOHEADER bi;
  bi.biSize = sizeof(BITMAPINFOHEADER);
  bi.biWidth = width;
  bi.biHeight = -height; // Negative height to ensure top-down DIB
  bi.biPlanes = 1;
  bi.biBitCount = 24; // 24 bits per pixel (RGB)
  bi.biCompression = BI_RGB;
  bi.biSizeImage = 0;
  bi.biXPelsPerMeter = 0;
  bi.biYPelsPerMeter = 0;
  bi.biClrUsed = 0;
  bi.biClrImportant = 0;

  // Create a Mat object using the pixel data
  cv::Mat screenshot(height, width, CV_8UC3);

  GetDIBits(hdcScreen, hBitmap, 0, height, screenshot.data, (BITMAPINFO*)&bi, DIB_RGB_COLORS);

  DeleteObject(hBitmap);
  DeleteDC(hdcMem);
  ReleaseDC(hwnd, hdcScreen);

  return screenshot;
}

cv::Mat Emulator::printscreen(const std::string &windowTitle, int x, int y, int width, int height)
{
  cv::Mat screenshot = printscreen(windowTitle);
  cv::Rect cropRegion(x, y, width, height);
  cv::Mat croped = screenshot(cropRegion).clone();

  return croped;
}

cv::Mat Emulator::printscreen(const std::string &windowTitle, Marker &marker)
{
  return printscreen(windowTitle, marker.x, marker.y, marker.width, marker.height);
}

cv::Vec3b Emulator::getPixelColor(const std::string &windowTitle, int x, int y)
{
  cv::Mat image = printscreen(windowTitle);
  cv::Vec3b color = image.at<cv::Vec3b>(y, x);
  return color;
}