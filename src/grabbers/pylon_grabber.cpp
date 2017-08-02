/******************************************************************************
 * Copyright (c) 2017 Tim Caselitz
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 ******************************************************************************/

#include <boost/algorithm/string.hpp>
#include <boost/throw_exception.hpp>

#include <pylon/PylonIncludes.h>
#include <pylon/usb/BaslerUsbInstantCamera.h>

#include <opencv2/imgproc/imgproc.hpp>

#include <grabbers/pylon_grabber.h>

namespace grabbers {

struct PylonGrabber::Impl {

  std::unique_ptr<Pylon::CBaslerUsbInstantCamera> camera;

  cv::Size color_image_resolution = {765, 576};
  size_t color_image_size = 0;
  int next_frame_index = 0;

  Impl() {
    Pylon::PylonInitialize();
  }

  void open() {
    try {
      Pylon::CDeviceInfo info;
      info.SetDeviceClass(Pylon::CBaslerUsbInstantCamera::DeviceClass());

      camera.reset(new Pylon::CBaslerUsbInstantCamera(Pylon::CTlFactory::GetInstance().CreateFirstDevice(info)));
      camera->Open();

      camera->PixelFormat.SetValue(Basler_UsbCameraParams::PixelFormat_BGR8);

      color_image_size = color_image_resolution.width * color_image_resolution.height * 3;
    } catch (const GenericException &e) {
      BOOST_THROW_EXCEPTION(GrabberException("Failed to open camera")
                            << GrabberException::ErrorInfo(e.GetDescription()));
    }
  }

  ~Impl() {
    camera.release();
    Pylon::PylonTerminate();
  }

  double grabFrame(cv::Mat& color) {
    try {
      camera->StartGrabbing(1);
      Pylon::CGrabResultPtr grab_result;
      camera->RetrieveResult(5000, grab_result, Pylon::TimeoutHandling_ThrowException);
      if (grab_result->GrabSucceeded()) {
        cv::Mat image(grab_result->GetHeight(), grab_result->GetWidth(), CV_8UC3, grab_result->GetBuffer());
        cv::resize(image, color, color_image_resolution, 0, 0, cv::INTER_AREA);
      }
      ++next_frame_index;
      return true;
    } catch (const GenericException &e) {
      BOOST_THROW_EXCEPTION(GrabberException("Failed to grab frame")
                            << GrabberException::ErrorInfo(e.GetDescription()));
      return false;
    }
  }
};

PylonGrabber::PylonGrabber() : p(new Impl) {
  p->open();
}

PylonGrabber::~PylonGrabber() {}

inline bool PylonGrabber::hasMoreFrames() const {
  return true;
}

bool PylonGrabber::grabFrame(cv::OutputArray _color) {
  if (_color.kind() != cv::_InputArray::MAT)
    BOOST_THROW_EXCEPTION(GrabberException("Grabbing only into cv::Mat"));

  _color.create(p->color_image_resolution.height, p->color_image_resolution.width, CV_8UC3);
  cv::Mat color = _color.getMat();

  return p->grabFrame(color);
}

void PylonGrabber::setAutoWhiteBalanceEnabled(bool state) {
  try {
    if (state) {
      p->camera->LightSourcePreset.SetValue(Basler_UsbCameraParams::LightSourcePreset_Daylight5000K);
      p->camera->BalanceWhiteAuto.SetValue(Basler_UsbCameraParams::BalanceWhiteAuto_Continuous);
    } else {
      p->camera->LightSourcePreset.SetValue(Basler_UsbCameraParams::LightSourcePreset_Off);
      p->camera->BalanceWhiteAuto.SetValue(Basler_UsbCameraParams::BalanceWhiteAuto_Off);
      p->camera->BalanceRatioSelector.SetValue(Basler_UsbCameraParams::BalanceRatioSelector_Red);
      p->camera->BalanceRatio.SetValue(0.57 * p->camera->BalanceRatio.GetMax());
      p->camera->BalanceRatioSelector.SetValue(Basler_UsbCameraParams::BalanceRatioSelector_Green);
      p->camera->BalanceRatio.SetValue(0.36 * p->camera->BalanceRatio.GetMax());
      p->camera->BalanceRatioSelector.SetValue(Basler_UsbCameraParams::BalanceRatioSelector_Blue);
      p->camera->BalanceRatio.SetValue(1.00 * p->camera->BalanceRatio.GetMax());
    }
  } catch (const GenericException &e) {
    BOOST_THROW_EXCEPTION(GrabberException("Failed to set auto white balance")
                          << GrabberException::ErrorInfo(e.GetDescription()));
  }
}

void PylonGrabber::setAutoExposureEnabled(bool state) {
  try {
    if (state) {
      p->camera->ExposureAuto.SetValue(Basler_UsbCameraParams::ExposureAuto_Continuous);
    } else {
      p->camera->ExposureAuto.SetValue(Basler_UsbCameraParams::ExposureAuto_Off);
    }
  } catch (const GenericException &e) {
    BOOST_THROW_EXCEPTION(GrabberException("Failed to set auto exposure")
                          << GrabberException::ErrorInfo(e.GetDescription()));
  }
}

void PylonGrabber::setExposure(int exposure) {
  try {
    p->camera->ExposureTime.SetValue(1000.0 * (double)exposure);
  } catch (const GenericException &e) {
    BOOST_THROW_EXCEPTION(GrabberException("Failed to set exposure")
                          << GrabberException::ErrorInfo(e.GetDescription()));
  }
}

int PylonGrabber::getExposure() const {
  try {
    return (int)(p->camera->ExposureTime.GetValue() / 1000.0 + 0.5);
  } catch (const GenericException &e) {
    BOOST_THROW_EXCEPTION(GrabberException("Failed to get exposure")
                          << GrabberException::ErrorInfo(e.GetDescription()));
    return -1;
  }
}

std::pair<int, int> PylonGrabber::getExposureRange() const {
  // This is somewhat random. While it is certainly possible to increase the exposure beyond 150,
  // most realistic scenes will be completely overexposed with this setting.
  return {1, 150};
}

void PylonGrabber::setGain(int gain) {
  try {
    p->camera->Gain.SetValue(20.0 * std::log((double)gain / 100.));
  } catch (const GenericException &e) {
    BOOST_THROW_EXCEPTION(GrabberException("Failed to set gain")
                          << GrabberException::ErrorInfo(e.GetDescription()));
  }
}

int PylonGrabber::getGain() const {
  try {
    return (int)(100 * std::pow(10, p->camera->Gain.GetValue() / 20.) + 0.5);
  } catch (const GenericException &e) {
    BOOST_THROW_EXCEPTION(GrabberException("Failed to get gain")
                          << GrabberException::ErrorInfo(e.GetDescription()));
    return -1;
  }
}

std::pair<int, int> PylonGrabber::getGainRange() const {
  return {100, 6309};
}

std::string PylonGrabber::getCameraModelName() const {
  try {
    std::string name = (std::string)(p->camera->GetDeviceInfo().GetModelName());
    boost::algorithm::to_lower(name);
    return name;
  } catch (const GenericException &e) {
    BOOST_THROW_EXCEPTION(GrabberException("Failed to get camera model name")
                          << GrabberException::ErrorInfo(e.GetDescription()));
    return "";
  }
}

std::string PylonGrabber::getCameraSerialNumber() const {
  try {
    return (std::string)(p->camera->GetDeviceInfo().GetSerialNumber());
  } catch (const GenericException &e) {
    BOOST_THROW_EXCEPTION(GrabberException("Failed to get camera serial number")
                          << GrabberException::ErrorInfo(e.GetDescription()));
    return "";
  }
}

}  // namespace grabbers
