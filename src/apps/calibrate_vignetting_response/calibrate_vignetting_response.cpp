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

#include <iostream>
#include <string>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <radical/exceptions.h>
#include <radical/nonparametric_vignetting_model.h>
#include <radical/polynomial_vignetting_model.h>
#include <radical/radiometric_response.h>
#include <radical/vignetting_response.h>

#include "grabbers/grabber.h"

#include "utils/arrange_images_in_grid.h"
#include "utils/mask_saturated_pixels.h"
#include "utils/mean_image.h"
#include "utils/plot_polynomial_vignetting_model.h"
#include "utils/program_options.h"

#include "blob_tracker.h"
#include "model_fitting.h"

class Options : public OptionsBase {
 public:
  std::string camera = "";
  std::string output;
  std::string crf;
  unsigned int num_samples = 100;
  unsigned int exposure = 20;
  std::string model = "nonparametric";
  bool fixed_center = false;

 protected:
  virtual void addOptions(boost::program_options::options_description& desc) override {
    namespace po = boost::program_options;
    desc.add_options()("output,o", po::value<std::string>(&output),
                       "Output filename with calibrated vignetting response (default: camera model name + \".\" + "
                       "camera serial number + \".vgn\" suffix)");
    desc.add_options()(
        "crf", po::value<std::string>(&crf),
        "Camera response function (default: camera model name + \".\" camera serial number + \".crf\" suffix)");
    desc.add_options()("num-samples,s", po::value<unsigned int>(&num_samples),
                       "Number of samples to collect for each pixel (default: 100)");
    desc.add_options()("exposure,e", po::value<unsigned int>(&exposure), "Initial exposure time (default: 20)");
    desc.add_options()("model,m", po::value<std::string>(&model), "Vignetting model type (default: nonparametric)");
    desc.add_options()("fixed-center,c", po::bool_switch(&fixed_center),
                       "Fix model center of symmetry to image center (only for polynomial model)");
  }

  virtual void addPositional(boost::program_options::options_description& desc,
                             boost::program_options::positional_options_description& positional) override {
    namespace po = boost::program_options;
    desc.add_options()("camera", po::value<std::string>(&camera), "Camera to calibrate (\"asus\", \"intel\")");
    positional.add("camera", -1);
  }

  virtual void printHelp() override {
    std::cout << "Usage: calibrate_vignetting_response [options] <camera>" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "Calibrate vignetting response of a camera. Two vignetting models are available:" << std::endl;
    std::cout << " * nonparametric" << std::endl;
    std::cout << " * polynomial" << std::endl;
    std::cout << "" << std::endl;
  }

  virtual void validate() override {
#ifndef HAVE_CERES
    if (model == "polynomial")
      throw boost::program_options::error("unable to calibrate polynomial vignetting model because the app was compiled without Ceres");
#endif
    if (model != "nonparametric" && model != "polynomial")
      throw boost::program_options::error("unknown vignetting model type " + model);
  }
};

int main(int argc, const char** argv) {
  Options options;
  if (!options.parse(argc, argv))
    return 1;

  auto imshow = [&options](const cv::Mat& image, int w = -1) {
    if (image.empty())
      return -1;
    cv::imshow("Calibration", image);
    return cv::waitKey(w);
  };

  cv::Mat data;

  grabbers::Grabber::Ptr grabber;

  try {
    grabber = grabbers::createGrabber(options.camera);
  } catch (grabbers::GrabberException& e) {
    std::cerr << "Failed to create a grabber" << (options.camera != "" ? " for camera " + options.camera : "")
              << std::endl;
    return 1;
  }

  if (!options("output"))
    options.output = grabber->getCameraUID() + ".vgn";
  if (!options("crf"))
    options.crf = grabber->getCameraUID() + ".crf";

  radical::RadiometricResponse::Ptr rr;

  try {
    rr.reset(new radical::RadiometricResponse(options.crf));
  } catch (radical::Exception& e) {
    std::cerr << "Failed to load radiometric response: " << e.what() << std::endl;
    return 1;
  }

  grabber->setAutoExposureEnabled(false);
  grabber->setAutoWhiteBalanceEnabled(false);

  auto range = grabber->getExposureRange();

  // Change exposure to requested initial value and let user adjust it
  int exposure = options.exposure;
  int key = 0;
  while (key != 13) {
    int old_exposure = exposure;
    cv::Mat frame;
    grabber->grabFrame(frame);
    key = imshow(frame, 30);
    if (key == 43 || key == 65362 || key == 65363)
      ++exposure;
    else if (key == 45 || key == 65364 || key == 65361)
      --exposure;
    if (old_exposure != exposure) {
      if (exposure > range.second)
        exposure = range.second;
      else if (exposure < range.first)
        exposure = range.first;
      else {
        grabber->setExposure(exposure);
        std::cout << "Exposure: " << old_exposure << " → " << exposure << std::endl;
      }
    }
  }

  std::cout << "Starting data collection" << std::endl;

  MeanImage mean(options.num_samples);
  BlobTracker tracker;

  cv::Mat frame;
  cv::Mat mask;
  cv::Mat irradiance;
  cv::Mat mean_color;
  int frame_counter = 0;

  while (grabber->hasMoreFrames()) {
    grabber->grabFrame(frame);
    tracker(frame, mask);
    maskSaturatedPixels(frame, mask);
    rr->inverseMap(frame, irradiance);
    if (mean.add(irradiance, mask))
      break;
    cv::Mat masked;
    frame.copyTo(masked, mask);

    // Mean computation and especially direct mapping is expensive, so we do not do it every frame
    if (frame_counter++ % 10 == 0)
      rr->directMap(mean.getMean(), mean_color);

    cv::Mat m8u;
    mean.getNumSamples(true).convertTo(m8u, CV_8U, 255);
    cv::cvtColor(m8u, m8u, CV_GRAY2BGR);  // duplicate channels
    for (auto i = m8u.begin<cv::Vec3b>(); i != m8u.end<cv::Vec3b>(); ++i)
      if (*i == cv::Vec3b(255, 255, 255))
        *i = cv::Vec3b(77, 175, 74);

    auto m = arrangeImagesInGrid({frame, masked, m8u, mean_color}, {2, 2});
    imshow(m, 30);
  }

  data = mean.getMean();

  // Normalize each channel separately
  std::vector<cv::Mat> channels;
  cv::split(data, channels);
  for (auto& c : channels) {
    double min, max;
    cv::minMaxLoc(c, &min, &max);
    c /= max;
  }
  cv::merge(channels, data);

  radical::VignettingModel::Ptr model;
  cv::Mat plot;

  if (options.model == "nonparametric") {
    model.reset(new radical::NonparametricVignettingModel(data));
#ifdef HAVE_CERES
  } else if (options.model == "polynomial") {
    auto poly_model = fitPolynomialModel(data, options.fixed_center, 50, false);
    plot = plotPolynomialVignettingModel(*poly_model);
    model = poly_model;
#endif
  }

  std::cout << "Done, writing response to: " << options.output << std::endl;
  model->save(options.output);

  imshow(plot, -1);

  return 0;
}
