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

#include <chrono>

#include <boost/algorithm/string.hpp>
#include <boost/throw_exception.hpp>

#include <librealsense/rs.hpp>

#include <grabbers/realsense_grabber.h>

namespace grabbers {

struct RealSenseGrabber::Impl {
  rs::context ctx;
  rs::device* device;

  cv::Size color_image_resolution = {0, 0};
  int next_frame_index = 0;

  uint64_t timestamp_zero_offset = 0;

  Impl() {
    if (ctx.get_device_count() == 0)
      BOOST_THROW_EXCEPTION(GrabberException("No RealSense devices connected"));

    device = ctx.get_device(0);
    // device->enable_stream(rs::stream::color, rs::preset::best_quality);
    device->enable_stream(rs::stream::color, 640, 480, rs::format::bgr8, 15);

    if (!device->is_stream_enabled(rs::stream::color) ||
        device->get_stream_format(rs::stream::color) != rs::format::bgr8)
      BOOST_THROW_EXCEPTION(GrabberException("Failed to create color stream with requested format"));

    color_image_resolution.width = device->get_stream_width(rs::stream::color);
    color_image_resolution.height = device->get_stream_height(rs::stream::color);

    device->start();
  }

  ~Impl() {
    if (device->is_streaming())
      device->stop();
    device->disable_stream(rs::stream::color);
  }

  double grabFrame(cv::Mat& color) {
    device->wait_for_frames();

    auto color_data = device->get_frame_data(rs::stream::color);
    memcpy(color.data, color_data, color.total() * color.elemSize());

    ++next_frame_index;
    return computeTimestamp(device->get_frame_timestamp(rs::stream::color));
  }

  double computeTimestamp(int device_timestamp) {
    if (timestamp_zero_offset == 0) {
      auto now = std::chrono::high_resolution_clock::now();
      auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
      timestamp_zero_offset = ms - device_timestamp;
    }
    return (timestamp_zero_offset + device_timestamp) * 0.001;
  }
};

RealSenseGrabber::RealSenseGrabber(const std::string& /*device_uri*/) : p(new Impl) {
}

RealSenseGrabber::~RealSenseGrabber() {}

inline bool RealSenseGrabber::hasMoreFrames() const {
  return true;
}

bool RealSenseGrabber::grabFrame(cv::OutputArray _color) {
  if (_color.kind() != cv::_InputArray::MAT)
    BOOST_THROW_EXCEPTION(GrabberException("Grabbing only into cv::Mat"));

  _color.create(p->color_image_resolution.height, p->color_image_resolution.width, CV_8UC3);
  cv::Mat color = _color.getMat();

  return p->grabFrame(color);
}

void RealSenseGrabber::setAutoWhiteBalanceEnabled(bool state) {
  p->device->set_option(rs::option::color_enable_auto_white_balance, state ? 1.0 : 0.0);
}

void RealSenseGrabber::setAutoExposureEnabled(bool state) {
  p->device->set_option(rs::option::color_enable_auto_exposure, state ? 1.0 : 0.0);
}

void RealSenseGrabber::setExposure(int exposure) {
  p->device->set_option(rs::option::color_exposure, exposure);
}

int RealSenseGrabber::getExposure() const {
  return p->device->get_option(rs::option::color_exposure);
}

std::pair<int, int> RealSenseGrabber::getExposureRange() const {
  double min, max, step;
  p->device->get_option_range(rs::option::color_exposure, min, max, step);
  return {min, max};
}

void RealSenseGrabber::setGain(int gain) {
  p->device->set_option(rs::option::color_gain, gain);
}

int RealSenseGrabber::getGain() const {
  return p->device->get_option(rs::option::color_gain);
}

std::pair<int, int> RealSenseGrabber::getGainRange() const {
  double min, max, step;
  p->device->get_option_range(rs::option::color_gain, min, max, step);
  return {min, max};
}

std::string RealSenseGrabber::getCameraModelName() const {
  std::string expected_prefix = "Intel RealSense ";
  std::string name = p->device->get_name();
  auto pos = name.find(expected_prefix);
  if (pos == std::string::npos)
    BOOST_THROW_EXCEPTION(GrabberException("Unable to extract camera model name from: " + name));
  name = name.substr(pos + expected_prefix.size());
  boost::algorithm::to_lower(name);
  return name;
}

std::string RealSenseGrabber::getCameraSerialNumber() const {
  return std::string(p->device->get_serial());
}

}  // namespace grabbers
