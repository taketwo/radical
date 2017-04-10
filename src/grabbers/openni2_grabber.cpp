/******************************************************************************
 * Copyright (c) 2016 Sergey Alexandrov
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

#include <OpenNI.h>

#include <grabbers/openni2_grabber.h>

namespace grabbers {

struct OpenNI2Grabber::Impl {
  openni::Device device;
  openni::VideoStream color_stream;
  openni::VideoFrameRef color_frame;
  std::vector<openni::VideoStream*> streams;

  cv::Size color_image_resolution = {640, 480};
  size_t color_image_size = 0;
  int num_frames = -1;
  int next_frame_index = 0;
  bool is_file = false;

  Impl() {
    openni::OpenNI::initialize();
  }

  void open(const char* uri) {
    if (device.open(uri) != openni::STATUS_OK)
      BOOST_THROW_EXCEPTION(GrabberException("Failed to open device")
                            << GrabberException::ErrorInfo(openni::OpenNI::getExtendedError()));

    if (color_stream.create(device, openni::SENSOR_COLOR) != openni::STATUS_OK)
      BOOST_THROW_EXCEPTION(GrabberException("Failed to create color stream")
                            << GrabberException::ErrorInfo(openni::OpenNI::getExtendedError()));

    openni::VideoMode color_mode;
    color_mode.setFps(30);
    color_mode.setResolution(color_image_resolution.width, color_image_resolution.height);
    color_mode.setPixelFormat(openni::PIXEL_FORMAT_RGB888);
    color_stream.setVideoMode(color_mode);
    color_image_size = color_image_resolution.width * color_image_resolution.height * 3;
    color_stream.setMirroringEnabled(false);

    if (color_stream.start() != openni::STATUS_OK) {
      color_stream.destroy();
      BOOST_THROW_EXCEPTION(GrabberException("Failed to start color stream")
                            << GrabberException::ErrorInfo(openni::OpenNI::getExtendedError()));
    }

    streams.push_back(&color_stream);

    auto control = device.getPlaybackControl();
    if (control != NULL) {
      // This is a file, make sure we get every frame
      control->setSpeed(-1.0f);
      control->setRepeatEnabled(false);
      num_frames = control->getNumberOfFrames(color_stream);
      is_file = true;
      if (num_frames == -1)
        BOOST_THROW_EXCEPTION(GrabberException("Unable to determine number of frames in ONI file"));
    }
  }

  ~Impl() {
    color_stream.stop();
    color_stream.destroy();
    device.close();
    openni::OpenNI::shutdown();
  }

  double grabFrame(cv::Mat& color) {
    int changed_index;
    auto status = openni::OpenNI::waitForAnyStream(streams.data(), 1, &changed_index);
    if (status != openni::STATUS_OK)
      return false;

    color_stream.readFrame(&color_frame);
    if (!color_frame.isValid())
      return false;

    auto tgt = color.data;
    auto src = reinterpret_cast<const uint8_t*>(color_frame.getData());
    for (size_t i = 0; i < color.total(); ++i) {
      *tgt++ = *(src + 2);
      *tgt++ = *(src + 1);
      *tgt++ = *(src + 0);
      src += 3;
    }

    ++next_frame_index;
    return true;
  }
};

OpenNI2Grabber::OpenNI2Grabber(const std::string& device_uri) : p(new Impl) {
  const char* uri = device_uri == "" ? openni::ANY_DEVICE : device_uri.c_str();
  p->open(uri);
}

OpenNI2Grabber::~OpenNI2Grabber() {}

inline bool OpenNI2Grabber::hasMoreFrames() const {
  return p->num_frames == -1 || p->next_frame_index < p->num_frames - 1;
}

bool OpenNI2Grabber::grabFrame(cv::OutputArray _color) {
  if (_color.kind() != cv::_InputArray::MAT)
    BOOST_THROW_EXCEPTION(GrabberException("Grabbing only into cv::Mat"));

  _color.create(p->color_image_resolution.height, p->color_image_resolution.width, CV_8UC3);
  cv::Mat color = _color.getMat();

  return p->grabFrame(color);
}

void OpenNI2Grabber::setAutoWhiteBalanceEnabled(bool state) {
  p->color_stream.getCameraSettings()->setAutoWhiteBalanceEnabled(state);
}

void OpenNI2Grabber::setAutoExposureEnabled(bool state) {
  p->color_stream.getCameraSettings()->setAutoExposureEnabled(state);
}

void OpenNI2Grabber::setExposure(int exposure) {
  p->color_stream.getCameraSettings()->setExposure(exposure);
}

int OpenNI2Grabber::getExposure() const {
  return p->color_stream.getCameraSettings()->getExposure();
}

std::pair<int, int> OpenNI2Grabber::getExposureRange() const {
  // This is somewhat random. While it is certainly possible to increase the exposure beyond 150,
  // most realistic scenes will be completely overexposed with this setting.
  return {1, 150};
}

void OpenNI2Grabber::setGain(int gain) {
  p->color_stream.getCameraSettings()->setGain(gain);
}

int OpenNI2Grabber::getGain() const {
  return p->color_stream.getCameraSettings()->getGain();
}

std::pair<int, int> OpenNI2Grabber::getGainRange() const {
  // These values were found experimentally.
  // The camera seems to ignore requests to set gain outside of this range.
  return {100, 1587};
}

std::string OpenNI2Grabber::getCameraModelName() const {
  std::string name = p->device.getDeviceInfo().getName();
  boost::algorithm::to_lower(name);
  return name;
}

std::string OpenNI2Grabber::getCameraSerialNumber() const {
  char serial[1024];
  p->device.getProperty(ONI_DEVICE_PROPERTY_SERIAL_NUMBER, &serial);
  return std::string(serial);
}

}  // namespace grabbers
