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

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#include <boost/format.hpp>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#if CV_MAJOR_VERSION >= 3
#include <opencv2/photo.hpp>                // Debevec calibration
#endif

#include <radical/radiometric_response.h>

#include "grabbers/grabber.h"

#include "utils/mean_image.h"
#include "utils/plot_radiometric_response.h"
#include "utils/program_options.h"

#include "dataset.h"

class Options : public OptionsBase {
 public:
  std::string data_source = "";
  std::string output;
  int exposure_min;
  int exposure_max;
  float exposure_factor;
  unsigned int num_average_frames = 5;
  unsigned int num_images = 5;
  unsigned int exposure_control_lag = 10;
  float convergence_threshold = 0.00001;
  std::string calibration_method = "engel";
  bool no_visualization = false;
  std::string save_dataset = "";

 protected:
  virtual void addOptions(boost::program_options::options_description& desc) override {
    namespace po = boost::program_options;
    desc.add_options()("output,o", po::value<std::string>(&output),
                       "Output filename with calibrated response function (default: camera model name + \".\" + camera "
                       "serial number + \".crf\" suffix)");
    desc.add_options()("min", po::value<int>(&exposure_min), "Minimum exposure (default: depends on the camera)");
    desc.add_options()("max", po::value<int>(&exposure_max), "Maximum exposure (default: depends on the camera)");
    desc.add_options()("factor,f", po::value<float>(&exposure_factor),
                       "Multiplication factor for exposure (default: to cover desired exposure range in 20 steps)");
    desc.add_options()("average,a", po::value<unsigned int>(&num_average_frames),
                       "Number of consecutive frames to average into each image (default: 5)");
    desc.add_options()("images,i", po::value<unsigned int>(&num_images),
                       "Number of images to take at each exposure setting (default: 5)");
    desc.add_options()("lag,l", po::value<unsigned int>(&exposure_control_lag),
                       "Number of frames to skip after changing exposure setting (default: 10)");
    desc.add_options()("threshold,t", po::value<float>(&convergence_threshold),
                       "Threshold for energy update after which convergence is declared (default: 0.00001)");
    desc.add_options()("method,m", po::value<std::string>(&calibration_method),
                       "Calibration method to use (default: engel)");
    desc.add_options()("no-visualization", po::bool_switch(&no_visualization),
                       "Do not visualize the calibration process and results");
    desc.add_options()("save-dataset,s", po::value<std::string>(&save_dataset),
                       "Save collected dataset in the given directory");
  }

  virtual void addPositional(boost::program_options::options_description& desc,
                             boost::program_options::positional_options_description& positional) override {
    namespace po = boost::program_options;
    desc.add_options()("data-source", po::value<std::string>(&data_source),
                       "Data source, either a camera (\"asus\", \"intel\"), or a path to dataset");
    positional.add("data-source", -1);
  }

  virtual void printHelp() override {
    std::cout << "Usage: calibrate_radiometric_response [options] <data-source>" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "Calibrate radiometric response of a camera. Two algorithms are available:" << std::endl;
    std::cout << " * Engel et al. (A Photometrically Calibrated Benchmark For Monocular Visual Odometry)" << std::endl;
    std::cout << " * Debevec and Malik (Recovering High Dynamic Range Radiance Maps from Photographs)" << std::endl;
    std::cout << "" << std::endl;
  }
};


class DataCollection {
 public:
  using Ptr = std::shared_ptr<DataCollection>;
  using ExposureRange = std::pair<int, int>;

  DataCollection(grabbers::Grabber::Ptr grabber, ExposureRange range, float factor, unsigned int average_frames,
                 unsigned int images, unsigned int control_lag)
  : grabber_(grabber), range_(range), factor_(factor), lag_(control_lag), mean_(average_frames),
    dataset_(new Dataset), num_images_(images) {
    BOOST_ASSERT(range.first <= range.second);
    BOOST_ASSERT(factor > 1.0);
    exposure_ = range_.first;
    skip_frames_ = lag_;
    images_to_accumulate_ = num_images_;
    std::cout << "Starting data collection" << std::endl;
    std::cout << "Exposure range: " << range.first << " → " << range.second << " with factor " << factor << std::endl;
    std::cout << "Exposure: " << exposure_ << std::flush;
  }

  bool addFrame(const cv::Mat& frame) {
    if (skip_frames_-- > 0)
      return false;

    if (!mean_.add(dilateSaturatedAreas(frame)))
      return false;

    dataset_->emplace_back(mean_.getMean(), exposure_);

    if (--images_to_accumulate_ > 0)
      return false;

    exposure_ += std::ceil(exposure_ * (factor_ - 1.0));
    skip_frames_ = lag_;
    images_to_accumulate_ = num_images_;
    grabber_->setExposure(exposure_);
    std::cout << " " << exposure_ << std::flush;

    if (exposure_ > range_.second) {
      std::cout << std::endl;
      return true;
    }
    return false;
  }

  Dataset::Ptr getDataset() const {
    return dataset_;
  }

 private:
  cv::Mat dilateSaturatedAreas(const cv::Mat& image) {
    static const cv::Mat morph = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5), cv::Point(2, 2));
    cv::Mat dilated;
    cv::threshold(image, dilated, 254, 255, cv::THRESH_BINARY);
    cv::dilate(dilated, dilated, morph);
    return cv::max(image, dilated);
  }

  grabbers::Grabber::Ptr grabber_;
  ExposureRange range_;
  float factor_;
  const unsigned int lag_;
  MeanImage mean_;
  int exposure_;
  Dataset::Ptr dataset_;
  int skip_frames_;
  const unsigned int num_images_;
  int images_to_accumulate_;
};

bool isSaturated(uint8_t intensity) {
  return intensity == 0 || intensity == 255;
}

struct Optimization {
  const Dataset* data;
  cv::Mat E;
  cv::Mat G;
  bool converged = false;

  Optimization(const Dataset* data) : data(data) {
    auto num_images = data->size();

    E.create(data->front().first.size(), CV_64FC1);
    E.setTo(0);
    for (size_t i = 0; i < data->size(); ++i) {
      const auto& image = data->at(i).first;
      for (int i = 0; i < image.rows; ++i)
        for (int j = 0; j < image.cols; ++j)
          E.at<double>(i, j) += static_cast<double>(image.at<uint8_t>(i, j)) / num_images / 256;
    }

    G.create(1, 256, CV_64FC1);
    for (int i = 0; i < 256; ++i)
      G.at<double>(i) = (1.0 / 256.0) * i;
  }

  void optimizeInverseResponse() {
    // Eqn. 7
    std::vector<double> sum_omega_k(256, 0);
    std::vector<int> size_omega_k(256, 0);

    for (size_t n = 0; n < data->size(); ++n) {
      const auto& image = data->at(n).first;
      const auto& exposure = data->at(n).second;
      for (int i = 0; i < image.rows; ++i)
        for (int j = 0; j < image.cols; ++j) {
          const auto& p = image.at<uint8_t>(i, j);
          if (isSaturated(p))
            continue;
          sum_omega_k[p] += exposure * E.at<double>(i, j);
          size_omega_k[p] += 1;
        }
    }

    for (int k = 1; k < 256; ++k) {
      G.at<double>(k) = sum_omega_k[k] /= size_omega_k[k];
      if (!std::isfinite(G.at<double>(k)) && k > 1)
        G.at<double>(k) = 2 * G.at<double>(k - 1) - G.at<double>(k - 2);
    }
  }

  void optimizeIrradiance() {
    // Eqn. 8
    cv::Mat sum_t2_i(data->front().first.size(), CV_64FC1);
    sum_t2_i.setTo(0);
    E.setTo(0);

    for (size_t n = 0; n < data->size(); ++n) {
      const auto& image = data->at(n).first;
      const auto& exposure = data->at(n).second;
      for (int i = 0; i < image.rows; ++i)
        for (int j = 0; j < image.cols; ++j) {
          const auto& p = image.at<uint8_t>(i, j);
          if (isSaturated(p))
            continue;
          E.at<double>(i, j) += G.at<double>(p) * exposure;
          sum_t2_i.at<double>(i, j) += exposure * exposure;
        }
    }

    cv::divide(E, sum_t2_i, E);
  }

  void rescale() {
    double min, max;
    cv::minMaxLoc(G, &min, &max);
    auto scale = 1.0 / max;
    cv::multiply(G, scale, G);
    cv::multiply(E, scale, E);
  }

  double computeEnergy() {
    long double energy = 0;
    long unsigned int num = 0;
    for (const auto& d : *data) {
      for (int i = 0; i < d.first.rows; ++i)
        for (int j = 0; j < d.first.cols; ++j)
          if (!isSaturated(d.first.at<uint8_t>(i, j))) {
            long double r = G.at<double>(d.first.at<uint8_t>(i, j)) - d.second * E.at<double>(i, j);
            energy += r * r;
            num += 1;
          }
    }
    return std::sqrt(energy / num);
  }
};

int main(int argc, const char** argv) {
  Options options;
  if (!options.parse(argc, argv))
    return 1;

  auto imshow = [&options](const cv::Mat& image, int w = -1) {
    if (!options.no_visualization) {
      cv::imshow("Calibration", image);
      cv::waitKey(w);
    }
  };

  auto data = Dataset::load(options.data_source);
  if (data) {
    if (!options("output")) {
      std::cerr << "When calibrating from existing dataset, output calibration filename should be explicitly set"
                << std::endl;
      return 2;
    }
    std::cout << "Loaded dataset from: " << options.data_source << std::endl;
  } else {
    grabbers::Grabber::Ptr grabber;

    try {
      grabber = grabbers::createGrabber(options.data_source);
    } catch (grabbers::GrabberException& e) {
      std::cerr << "Failed to create a grabber"
                << (options.data_source != "" ? " for camera " + options.data_source : "") << std::endl;
      return 1;
    }

    grabber->setAutoExposureEnabled(false);
    grabber->setAutoWhiteBalanceEnabled(false);

    if (!options("output"))
      options.output = grabber->getCameraUID() + ".crf";
    if (!options("min"))
      options.exposure_min = grabber->getExposureRange().first;
    if (!options("max"))
      options.exposure_max = grabber->getExposureRange().second;
    if (!options("factor"))
      options.exposure_factor = std::pow(options.exposure_max / options.exposure_min, 1.0 / 20);

    // Change exposure to requested min value, wait some time until it works
    cv::Mat frame;
    for (size_t i = 0; i < 100; ++i) {
      grabber->grabFrame(frame);
      imshow(frame, 30);
      grabber->setExposure(options.exposure_min);
    }

    DataCollection::Ptr data_collection;
    data_collection.reset(new DataCollection(grabber, {options.exposure_min, options.exposure_max},
                                             options.exposure_factor, options.num_average_frames, options.num_images,
                                             options.exposure_control_lag));

    while (grabber->hasMoreFrames()) {
      grabber->grabFrame(frame);
      imshow(frame, 30);
      if (data_collection->addFrame(frame))
        break;
    }

    data = data_collection->getDataset();
  }

  cv::Mat response;

  if (options.save_dataset != "") {
    std::cout << "Saving dataset to: " << options.save_dataset << std::endl;
    data->save(options.save_dataset);
  }

  if (options.calibration_method == "debevec") {
#if CV_MAJOR_VERSION >= 3
    std::cout << "Starting Debevec calibration procedure" << std::endl;
    auto debevec = cv::createCalibrateDebevec();
    debevec->process(data->getImages(), response, data->getExposures());
#else
    std::cerr << "Debevec calibration is supported only with OpenCV 3.0 and above.\n";
    return 2;
#endif
  } else if (options.calibration_method == "engel") {
    std::cout << "Starting Engel calibration procedure" << std::endl;
    auto data_channels = data->splitChannels();

    std::vector<Optimization> opts;
    for (const auto& data : data_channels)
      opts.emplace_back(&data);

    boost::format fmt_header1("| %=5s | %=72s |");
    boost::format fmt_header2("| %=5s | %=22s | %=22s | %=22s |");
    boost::format fmt_line("|  %2i %s |");
    boost::format fmt_channel_update("%10.6f | %10.6f");
    boost::format fmt_channel_converged("%=23s");

    std::cout << boost::str(fmt_header1 % "Iter" % "Energy / Diff") << std::endl;
    std::cout << boost::str(fmt_header2 % "" % "Blue" % "Green" % "Red") << std::endl;

    std::vector<double> energy(opts.size(), 0);
    std::vector<double> diff(opts.size(), 0);

    for (size_t i = 1; i < 100; ++i) {
      std::cout << boost::str(fmt_line % i % "G");

      for (size_t c = 0; c < opts.size(); ++c) {
        std::string info;
        if (opts[c].converged) {
          info = boost::str(fmt_channel_converged % "*");
        } else {
          opts[c].optimizeInverseResponse();
          auto e = opts[c].computeEnergy();
          diff[c] = energy[c] - e;
          if (energy[c] > 0 && diff[c] < options.convergence_threshold)
            opts[c].converged = true;
          energy[c] = e;
          info = boost::str(fmt_channel_update % e % diff[c]);
        }
        std::cout << info << " |";
      }

      std::cout << std::endl;
      std::cout << boost::str(fmt_line % i % "E");

      for (size_t c = 0; c < opts.size(); ++c) {
        std::string info;
        if (opts[c].converged) {
          info = boost::str(fmt_channel_converged % "*");
        } else {
          opts[c].optimizeIrradiance();
          auto e = opts[c].computeEnergy();
          if (energy[c] > 0) {
            diff[c] = energy[c] - e;
            if (diff[c] < options.convergence_threshold)
              opts[c].converged = true;
          }
          energy[c] = e;
          info = boost::str(fmt_channel_update % e % diff[c]);
        }
        std::cout << info << " |";
      }

      std::cout << std::endl;

      for (auto& p : opts)
        p.rescale();

      std::vector<cv::Mat> response_channels;
      for (auto& p : opts)
        response_channels.push_back(p.G);
      cv::Mat response_double;
      cv::merge(response_channels, response_double);
      response_double.convertTo(response, CV_32FC3);
      imshow(plotRadiometricResponse(response), 1);

      bool converged = true;
      for (size_t c = 0; c < opts.size(); ++c)
        converged &= opts[c].converged;

      if (converged)
        break;
    }
  } else {
    std::cerr << "Unknown calibration method: " << options.calibration_method
              << ". Please specify \"engel\" or \"debevec\".\n";
    return 1;
  }

  // Post-process the response:
  //  * sort to ensure invertability
  //  * there is no useful mapping for 0 brightness, extrapolate for 1 ond 2
  //  * shift and rescale response to 0..1 range
  std::vector<cv::Mat> channels;
  cv::split(response, channels);
  for (size_t i = 0; i < 3; ++i) {
    cv::sort(channels[i], channels[i], CV_SORT_EVERY_COLUMN | CV_SORT_ASCENDING);
    cv::subtract(channels[i], 2 * channels[i].at<float>(1) - channels[i].at<float>(2), channels[i]);
    channels[i].at<float>(0) = 0;
    cv::divide(channels[i], channels[i].at<float>(255), channels[i]);
  }
  cv::merge(channels, response);

  std::cout << "Done, writing response to: " << options.output << std::endl;
  radical::RadiometricResponse rr(response);
  rr.save(options.output);

  imshow(plotRadiometricResponse(rr));

  return 0;
}
