/******************************************************************************
 * Copyright (c) 2016-2017 Sergey Alexandrov
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
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/format.hpp>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <radical/radiometric_response.h>

#include "grabbers/grabber.h"

#include "utils/plot_radiometric_response.h"
#include "utils/program_options.h"

#include "calibration.h"
#include "dataset.h"
#include "dataset_collection.h"
#include "engel_calibration.h"

#if HAVE_CERES
#include "debevec_calibration.h"
#define DEFAULT_METHOD "debevec"
#else
#define DEFAULT_METHOD "engel"
#endif

class Options : public OptionsBase {
 public:
  std::string data_source = "";
  std::string output;
  double convergence_threshold = 1e-5;
  std::string calibration_method = DEFAULT_METHOD;
  bool no_visualization = false;
  std::string save_dataset = "";
  DatasetCollection::Parameters dc;
  unsigned int verbosity = 1;
  unsigned int min_samples = 5;
  bool interactive = false;
  double smoothing = 50;
  bool print = false;

 protected:
  virtual void addOptions(boost::program_options::options_description& desc) override {
    namespace po = boost::program_options;
    desc.add_options()("output,o", po::value<std::string>(&output),
                       "Output filename with calibrated response function (default: camera model name + \".\" + camera "
                       "serial number + \".crf\" suffix)");
    desc.add_options()("threshold,t", po::value<double>(&convergence_threshold),
                       "Threshold for energy update after which convergence is declared (default: 1e-5)");
    desc.add_options()("method,m", po::value<std::string>(&calibration_method)->default_value(calibration_method),
                       "Calibration method to use");
    desc.add_options()("min-samples", po::value<unsigned int>(&min_samples)->default_value(min_samples),
                       "Min number of samples per intensity level (only for debevec method)");
    desc.add_options()("smoothing", po::value<double>(&smoothing)->default_value(smoothing),
                       "Smoothing lambda (only for debevec method)");
    desc.add_options()("no-visualization", po::bool_switch(&no_visualization),
                       "Do not visualize the calibration process and results");
    desc.add_options()("verbosity,v", po::value<unsigned int>(&verbosity)->default_value(verbosity),
                       "Verbosity level (0 - silent, 1 - normal, 2 - verbose)");
    desc.add_options()("interactive", po::bool_switch(&interactive),
                       "Wait for a keypress after each optimization iteration");
    desc.add_options()("print", po::bool_switch(&print), "Print calibrated response function to stdout");

    boost::program_options::options_description dcopt("Data collection");
    dcopt.add_options()("exposure-min", po::value<int>(&dc.exposure_min),
                        "Minimum exposure (default: depends on the camera)");
    dcopt.add_options()("exposure-max", po::value<int>(&dc.exposure_max),
                        "Maximum exposure (default: depends on the camera)");
    dcopt.add_options()("factor,f", po::value<float>(&dc.exposure_factor),
                        "Multiplication factor for exposure (default: to cover desired exposure range in 30 steps)");
    dcopt.add_options()("average,a",
                        po::value<unsigned int>(&dc.num_average_frames)->default_value(dc.num_average_frames),
                        "Number of consecutive frames to average into each image");
    dcopt.add_options()("images,i", po::value<unsigned int>(&dc.num_images)->default_value(dc.num_images),
                        "Number of images to take at each exposure setting");
    dcopt.add_options()("lag,l",
                        po::value<unsigned int>(&dc.exposure_control_lag)->default_value(dc.exposure_control_lag),
                        "Number of frames to skip after changing exposure setting");
    dcopt.add_options()("valid-min",
                        po::value<unsigned int>(&dc.valid_intensity_min)->default_value(dc.valid_intensity_min),
                        "Minimum valid intensity value of the sensor");
    dcopt.add_options()("valid-max",
                        po::value<unsigned int>(&dc.valid_intensity_max)->default_value(dc.valid_intensity_max),
                        "Maximum valid intensity value of the sensor");
    dcopt.add_options()("bloom-radius", po::value<unsigned int>(&dc.bloom_radius)->default_value(dc.bloom_radius),
                        "Radius of the blooming effect");
    dcopt.add_options()("save-dataset,s", po::value<std::string>(&save_dataset),
                        "Save collected dataset in the given directory");
    desc.add(dcopt);
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
    std::cout << "Working range of the sensor can be specified with --valid-min/--valid-max options." << std::endl;
    std::cout << "Pixels with intensity values outside this range do not contribute to the energy and" << std::endl;
    std::cout << "irradiance computation, however radiometric response is estimated for them as well." << std::endl;
    std::cout << "" << std::endl;
  }

  virtual void validate() override {
    if (dc.valid_intensity_max > 255) {
      throw boost::program_options::error("maximum valid intensity can not exceed 255");
    }
    if (dc.valid_intensity_min > 255) {
      throw boost::program_options::error("minimum valid intensity can not exceed 255");
    }
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
      auto dir = boost::filesystem::canonical(options.data_source);
      options.output = (dir / dir.filename()).string() + ".crf";
    }
    if (options.verbosity)
      std::cout << "Loaded dataset from: " << options.data_source << std::endl;
  } else {
    grabbers::Grabber::Ptr grabber;

    try {
      grabber = grabbers::createGrabber(options.data_source);
    } catch (grabbers::GrabberException&) {
      std::cerr << "Failed to create a grabber"
                << (options.data_source != "" ? " for camera " + options.data_source : "") << std::endl;
      return 1;
    }

    grabber->setAutoExposureEnabled(false);
    grabber->setAutoWhiteBalanceEnabled(false);

    if (!options("output"))
      options.output = grabber->getCameraUID() + ".crf";
    if (!options("exposure-min"))
      options.dc.exposure_min = grabber->getExposureRange().first;
    if (!options("exposure-max"))
      options.dc.exposure_max = grabber->getExposureRange().second;
    if (!options("factor"))
      options.dc.exposure_factor = std::pow(1.0f * options.dc.exposure_max / options.dc.exposure_min, 1.0f / 30);

    // Change exposure to requested min value, wait some time until it works
    cv::Mat frame;
    for (size_t i = 0; i < 100; ++i) {
      grabber->grabFrame(frame);
      imshow(frame, 30);
      grabber->setExposure(options.dc.exposure_min);
    }

    DatasetCollection data_collection(grabber, options.dc);

    while (grabber->hasMoreFrames()) {
      grabber->grabFrame(frame);
      imshow(frame, 30);
      if (data_collection.addFrame(frame))
        break;
    }

    data = data_collection.getDataset();

    if (options.save_dataset != "") {
      if (options.verbosity)
        std::cout << "Saving dataset to: " << options.save_dataset << std::endl;
      data->save(options.save_dataset);
      std::ofstream file(options.save_dataset + "/DESCRIPTION.txt");
      if (file.is_open()) {
        file << "Camera: " << grabber->getCameraUID() << std::endl;
        file << "Resolution: " << data->getImageSize() << std::endl;
        file << "Exposure range: " << options.dc.exposure_min << " " << options.dc.exposure_max << std::endl;
        file << "Exposure factor: " << options.dc.exposure_factor << std::endl;
        file << "Images per exposure time: " << options.dc.num_images << std::endl;
        file << "Frames averaged into an image: " << options.dc.num_average_frames << std::endl;
        file.close();
      }
    }
  }

  Calibration::Ptr calibration;

  if (options.calibration_method == "engel") {
    auto cal = std::make_shared<EngelCalibration>();
    cal->setConvergenceThreshold(options.convergence_threshold);
    calibration = cal;
  } else if (options.calibration_method == "debevec") {
#if HAVE_CERES
    auto cal = std::make_shared<DebevecCalibration>();
    cal->setMinSamplesPerIntensityLevel(options.min_samples);
    cal->setSmoothingLambda(options.smoothing);
    calibration = cal;
#else
    std::cerr << "Debevec calibration is not supported because the project was compiled without Ceres.\n";
    return 2;
#endif
  } else {
    std::cerr << "Unknown calibration method: " << options.calibration_method
              << ". Please specify \"engel\" or \"debevec\".\n";
    return 1;
  }

  auto limshow = [=](const cv::Mat& image) {
    cv::imshow("Calibration", image);
    cv::waitKey(options.interactive ? -1 : 1);
  };

  calibration->setValidPixelRange(static_cast<unsigned char>(options.dc.valid_intensity_min),
                                  static_cast<unsigned char>(options.dc.valid_intensity_max));
  calibration->setVerbosity(options.verbosity);
  if (!options.no_visualization)
    calibration->setVisualizeProgress(limshow);

  cv::Mat response = calibration->calibrate(*data);

  if (options.verbosity)
    std::cout << "Done, writing response to: " << options.output << std::endl;
  radical::RadiometricResponse rr(response);
  rr.save(options.output);

  imshow(utils::plotRadiometricResponse(rr));

  if (options.print) {
    std::vector<cv::Mat> response_channels;
    cv::split(response, response_channels);
    for (const auto& ch : response_channels) {
      for (int i = 0; i < static_cast<int>(ch.total()); ++i)
        std::cout << ch.at<float>(i) << " ";
      std::cout << std::endl;
    }
  }

  return 0;
}
