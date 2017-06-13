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
#include <fstream>
#include <iostream>
#include <iostream>
#include <string>
#include <vector>

#include <boost/format.hpp>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#if CV_MAJOR_VERSION >= 3
#include <opencv2/photo.hpp>  // Debevec calibration
#endif

#include <radical/radiometric_response.h>

#include "grabbers/grabber.h"

#include "utils/plot_radiometric_response.h"
#include "utils/program_options.h"

#include "dataset.h"
#include "dataset_collection.h"
#include "optimization.h"

class Options : public OptionsBase {
 public:
  std::string data_source = "";
  std::string output;
  double convergence_threshold = 1e-5;
  std::string calibration_method = "engel";
  bool no_visualization = false;
  std::string save_dataset = "";
  DatasetCollection::Parameters dc;

 protected:
  virtual void addOptions(boost::program_options::options_description& desc) override {
    namespace po = boost::program_options;
    desc.add_options()("output,o", po::value<std::string>(&output),
                       "Output filename with calibrated response function (default: camera model name + \".\" + camera "
                       "serial number + \".crf\" suffix)");
    desc.add_options()("threshold,t", po::value<double>(&convergence_threshold),
                       "Threshold for energy update after which convergence is declared (default: 1e-5)");
    desc.add_options()("method,m", po::value<std::string>(&calibration_method),
                       "Calibration method to use (default: engel)");
    desc.add_options()("no-visualization", po::bool_switch(&no_visualization),
                       "Do not visualize the calibration process and results");
    desc.add_options()("save-dataset,s", po::value<std::string>(&save_dataset),
                       "Save collected dataset in the given directory");

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
    if (!options("exposure-min"))
      options.dc.exposure_min = grabber->getExposureRange().first;
    if (!options("exposure-max"))
      options.dc.exposure_max = grabber->getExposureRange().second;
    if (!options("factor"))
      options.dc.exposure_factor = std::pow(options.dc.exposure_max / options.dc.exposure_min, 1.0 / 30);

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

  cv::Mat response;

  if (options.calibration_method == "debevec") {
#if CV_MAJOR_VERSION >= 3
    std::cout << "Starting Debevec calibration procedure" << std::endl;
    std::vector<cv::Mat> images;
    std::vector<int> exposure_times;
    data->asImageAndExposureTimeVectors(images, exposure_times);
    auto debevec = cv::createCalibrateDebevec();
    debevec->process(images, response, exposure_times);
#else
    std::cerr << "Debevec calibration is supported only with OpenCV 3.0 and above.\n";
    return 2;
#endif
  } else if (options.calibration_method == "engel") {
    std::cout << "Starting Engel calibration procedure" << std::endl;
    auto data_channels = data->splitChannels();

    std::vector<Optimization> opts;
    for (const auto& data : data_channels)
      opts.emplace_back(&data, options.dc.valid_intensity_min, options.dc.valid_intensity_max);

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
      std::cout << boost::str(fmt_line % i % "B") << std::flush;

      for (size_t c = 0; c < opts.size(); ++c) {
        std::string info;
        if (opts[c].converged()) {
          info = boost::str(fmt_channel_converged % "*");
        } else {
          opts[c].optimizeIrradiance();
          auto e = opts[c].computeEnergy();
          if (energy[c] > 0) {
            diff[c] = energy[c] - e;
            if (diff[c] < options.convergence_threshold)
              opts[c].converged(true);
          }
          energy[c] = e;
          info = boost::str(fmt_channel_update % e % diff[c]);
        }
        std::cout << info << " |" << std::flush;
      }

      std::cout << std::endl;
      std::cout << boost::str(fmt_line % i % "U") << std::flush;

      for (size_t c = 0; c < opts.size(); ++c) {
        std::string info;
        if (opts[c].converged()) {
          info = boost::str(fmt_channel_converged % "*");
        } else {
          opts[c].optimizeInverseResponse();
          auto e = opts[c].computeEnergy();
          diff[c] = energy[c] - e;
          if (energy[c] > 0 && diff[c] < options.convergence_threshold)
            opts[c].converged(true);
          energy[c] = e;
          info = boost::str(fmt_channel_update % e % diff[c]);
        }
        std::cout << info << " |" << std::flush;
      }

      std::cout << std::endl;

      for (auto& p : opts)
        p.rescale();

      std::vector<cv::Mat> response_channels;
      for (auto& p : opts)
        response_channels.push_back(p.getOptimizedInverseResponse());
      cv::Mat response_double;
      cv::merge(response_channels, response_double);
      response_double.convertTo(response, CV_32FC3);
      imshow(utils::plotRadiometricResponse(response), 1);

      bool converged = true;
      for (size_t c = 0; c < opts.size(); ++c)
        converged &= opts[c].converged();

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
  //  * there is no useful mapping for 0 brightness, extrapolate for 1 and 2
  //  * shift and rescale response to 0..1 range
  std::vector<cv::Mat> channels;
  cv::split(response, channels);
  for (size_t i = 0; i < 3; ++i) {
    cv::sort(channels[i], channels[i], CV_SORT_EVERY_ROW | CV_SORT_ASCENDING);
    cv::divide(channels[i], channels[i].at<float>(255), channels[i]);
  }
  cv::merge(channels, response);

  std::cout << "Done, writing response to: " << options.output << std::endl;
  radical::RadiometricResponse rr(response);
  rr.save(options.output);

  imshow(utils::plotRadiometricResponse(rr));

  return 0;
}
