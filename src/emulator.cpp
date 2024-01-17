#include "emulator.hpp"

void Emulator::destroy()
{
  for(auto &instance : instances)
  {
    instance.isWorking = false;
    instance.isRunning = false;
    instance.isDebugging = false;
  }
}

void Emulator::arrange()
{
  std::string res = util::parseCMD(this->console, {"arrange"});

  if(!res.empty())
  {
    LOGGER_DEBUG(res);
  }
}

void Emulator::list()
{
  std::string res = util::parseCMD(this->console, {"list"});

  if(!res.empty())
  {
    std::vector<std::string> lines = util::split(res, '\n');

    for(auto &line : lines)
    {
      // 0: -index, 1: nox_id, 2: -name, 4: top win handle, 5: PID
      std::vector<std::string> values = util::split(line, ',');
      bool existingInstance = false;

      // Update the existing instance
      for(auto &instance : instances)
      {
        // Found the instance
        if(instance.index == (uint16_t)std::stoi(values[0]))
        {
          existingInstance = true;

          instance.name = values[2];
          instance.hwnd = reinterpret_cast<HWND>((uintptr_t)std::stoul(values[3].c_str(), nullptr, 16));
          instance.PID = std::stoi(values[5]);
          instance.isRunning = instance.PID != 0;

          break;
        }
      }

      // Exits the lines loop's current iteration
      if(existingInstance)
      {
        continue;
      }

      // Push a new instance into the instances vector
      this->instances.push_back(Instance{
        .index = (uint16_t)std::stoi(values[0]),
        .name = values[2],
        .hwnd = reinterpret_cast<HWND>((uintptr_t)std::stoul(values[3].c_str(), nullptr, 16)),
        .PID = std::stoi(values[5]),
        .isRunning = std::stoi(values[5]) != 0
      });
    }
  }
}

void Emulator::launch(uint16_t index)
{
  std::string res = util::parseCMD(this->console, {"launch", std::format("-index:{}", index)});

  if(!res.empty())
  {
    LOGGER_DEBUG(res);
  }
}

void Emulator::quit(uint16_t index)
{
  std::string res = util::parseCMD(this->console, {"quit", std::format("-index:{}", index)});

  if(!res.empty())
  {
    LOGGER_DEBUG(res);
  }
}

void Emulator::runapp(uint16_t index, const std::string &packagename)
{
  std::string res = util::parseCMD(std::format("{} runapp -index:{} -packagename:{}", this->console, index, packagename));

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

  // Crop the image
  int top_crop = 32;
  int left_crop = 2;
  int right_crop = 40;
  int bottom_crop = 2;

  cv::Rect crop_region(left_crop, top_crop, width - left_crop - right_crop, height - top_crop - bottom_crop);
  cv::Mat cropped_screenshot = screenshot(crop_region).clone();

  return cropped_screenshot;
}
