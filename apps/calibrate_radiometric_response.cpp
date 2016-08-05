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

#include <vector>

#include <opencv2/core/core.hpp>

#if CV_MAJOR_VERSION < 3
#error This app requires OpenCV 3.0 or above
#endif

#include <opencv2/highgui.hpp>
#include <opencv2/photo.hpp>

#include <radical/radiometric_response.h>

#include "grabbers/grabber.h"
#include "utils/program_options.h"

class Options : public OptionsBase {
 public:
  std::string output;
  int min;
  int max;
  unsigned int step;
  unsigned int average = 10;
  unsigned int lag = 10;

 protected:
  virtual void addOptions(boost::program_options::options_description& desc) override {
    namespace po = boost::program_options;
    desc.add_options()("output,o", po::value<std::string>(&output),
                       "Output filename (default: camera serial number + \".crf\" suffix)");
    desc.add_options()("min", po::value<int>(&min), "Minimum exposure (default: depends on the camera)");
    desc.add_options()("max", po::value<int>(&max), "Maximum exposure (default: depends on the camera)");
    desc.add_options()("step", po::value<unsigned int>(&step),
                       "Exposure step (default: to cover exposure range in 20 steps)");
    desc.add_options()("average", po::value<unsigned int>(&average),
                       "Number of frames to take at every exposure step (default: 10)");
    desc.add_options()("lag", po::value<unsigned int>(&lag),
                       "Number of frames to skip after changing exposure setting (default: 10)");
  }

  virtual void printHelp() override {
    std::cout << "Usage: calibrate_radiometric_response" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "Calibrate radiometric response of a camera." << std::endl;
    std::cout << "" << std::endl;
  }
};

class AverageImage {
 public:
  AverageImage(unsigned int num_images) : num_images_(num_images), counter_(0) {
    BOOST_ASSERT(num_images_);
  }

  bool add(const cv::Mat& image) {
    if (sum_.empty()) {
      sum_.create(image.size(), CV_32FC3);
      sum_.setTo(0);
    } else {
      BOOST_ASSERT(sum_.size() == image.size());
    }
    cv::Mat image_float;
    image.convertTo(image_float, CV_32FC3);
    sum_ += image_float;
    counter_++;
    return counter_ < num_images_;
  }

  cv::Mat get() {
    cv::Mat average;
    sum_ /= (1.0f * num_images_);
    sum_.convertTo(average, CV_8UC3);
    sum_.setTo(0);
    counter_ = 0;
    return average;
  }

 private:
  unsigned int num_images_;
  unsigned int counter_;
  cv::Mat sum_;
};

class DataCollection {
 public:
  using Ptr = std::shared_ptr<DataCollection>;
  using ExposureRange = std::pair<int, int>;

  DataCollection(grabbers::Grabber::Ptr grabber, ExposureRange range, int step, unsigned int average_frames,
                 unsigned int control_lag)
  : grabber_(grabber), range_(range), step_(step), lag_(control_lag), average_(average_frames) {
    BOOST_ASSERT(range.first <= range.second);
    BOOST_ASSERT(step > 0);
    exposure_ = range_.first;
    skip_frames_ = lag_;
    grabber_->setExposure(exposure_);
    std::cout << "Starting data collection" << std::endl;
    std::cout << "Exposure range: " << range.first << " → " << range.second << " with step " << step << std::endl;
    std::cout << "Exposure: " << exposure_;
  }

  bool addFrame(const cv::Mat& frame) {
    if (skip_frames_-- > 0)
      return false;
    if (average_.add(frame))
      return false;

    images_.push_back(average_.get());
    exposures_.push_back(exposure_);
    exposure_ += step_;
    skip_frames_ = lag_;
    grabber_->setExposure(exposure_);
    std::cout << " " << exposure_ << std::flush;
    return exposure_ > range_.second;
  }

  const std::vector<cv::Mat>& getImages() const {
    return images_;
  }
  const std::vector<int>& getExposures() const {
    return exposures_;
  }

 private:
  grabbers::Grabber::Ptr grabber_;
  ExposureRange range_;
  int step_;
  unsigned int lag_;
  int exposure_;
  AverageImage average_;
  std::vector<cv::Mat> images_;
  std::vector<int> exposures_;
  int skip_frames_;
};

int main(int argc, const char** argv) {
  Options options;
  if (!options.parse(argc, argv))
    return 1;

  auto grabber = grabbers::createGrabber();
  grabber->setAutoExposureEnabled(false);
  grabber->setAutoWhiteBalanceEnabled(false);

  if (!options("output"))
    options.output = grabber->getSerialNumber() + ".crf";
  if (!options("min"))
    options.min = grabber->getExposureRange().first;
  if (!options("max"))
    options.max = grabber->getExposureRange().second;
  if (!options("step"))
    options.step = (options.max - options.min) / 20;

  DataCollection::Ptr data_collection;
  data_collection.reset(
      new DataCollection(grabber, {options.min, options.max}, options.step, options.average, options.lag));

  cv::Mat frame;
  while (grabber->hasMoreFrames()) {
    grabber->grabFrame(frame);
    if (data_collection->addFrame(frame))
      break;
    cv::imshow("Camera", frame);
    if (cv::waitKey(30) >= 0)
      break;
  }

  std::cout << std::endl;
  std::cout << "Starting Debevec calibration procedure" << std::endl;
  auto debevec = cv::createCalibrateDebevec();
  cv::Mat response;
  debevec->process(data_collection->getImages(), response, data_collection->getExposures());
  std::cout << "Done, writing response to: " << options.output << std::endl;

  std::vector<cv::Mat> channels;
  cv::split(response, channels);

  for (size_t i = 0; i < 3; ++i)
    cv::sort(channels[i], channels[i], CV_SORT_EVERY_COLUMN | CV_SORT_ASCENDING);

  cv::merge(channels, response);

  radical::RadiometricResponse rr(response);
  rr.save(options.output);

  return 0;
}
