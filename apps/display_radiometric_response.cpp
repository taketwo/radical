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

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

// In OpenCV 3 imwrite has been moved to imgcodecs module
#if CV_MAJOR_VERSION >= 3
#include <opencv2/imgcodecs/imgcodecs.hpp>
#endif

#include <radical/radiometric_response.h>

#include "utils/program_options.h"

class Options : public OptionsBase {
 public:
  std::string r_response;
  bool save = false;

 protected:
  virtual void addOptions(boost::program_options::options_description& desc) override {
    namespace po = boost::program_options;
    desc.add_options()("save,s", po::bool_switch(&save), "Save to PNG file");
  }

  virtual void addPositional(boost::program_options::options_description& desc,
                             boost::program_options::positional_options_description& positional) override {
    namespace po = boost::program_options;
    desc.add_options()("crf", po::value<std::string>(&r_response), "Calibration file with radiometric response");
    positional.add("crf", 1);
  }

  virtual void printHelp() override {
    std::cout << "Usage: display_radiometric_response [options] <radiometric-response>" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "Visualizes radiometric response stored in a calibration file." << std::endl;
    std::cout << "" << std::endl;
    std::cout << "With the --save option the displayed image will also be written to the disk." << std::endl;
    std::cout << "File name is constructed by appending \".png\" to the input file path." << std::endl;
    std::cout << "" << std::endl;
  }
};

int main(int argc, const char** argv) {
  Options options;
  if (!options.parse(argc, argv))
    return 1;

  radical::RadiometricResponse rr(options.r_response);
  auto min_radiance = rr.inverseMap(cv::Vec3b(0, 0, 0));
  auto max_radiance = rr.inverseMap(cv::Vec3b(255, 255, 255));

  std::cout << "Loaded radiometric response from file \"" << options.r_response << "\"" << std::endl;
  std::cout << "Irradiance range: " << min_radiance << " - " << max_radiance << std::endl;

  const int WIDTH = 640;
  const int HEIGHT = 480;

  cv::Mat plot(HEIGHT, WIDTH, CV_8UC3);
  plot.setTo(255);

  float min = std::min(std::min(min_radiance[0], min_radiance[1]), min_radiance[2]);
  float max = std::max(std::max(max_radiance[0], max_radiance[1]), max_radiance[2]);
  float x_scale = 1.0f * WIDTH / (256);
  float y_scale = 1.0f * HEIGHT / (max - min);

  const cv::Scalar CHANNELS[] = {{255, 0, 0}, {0, 255, 0}, {0, 0, 255}};

  for (size_t i = 0; i < 256; ++i) {
    auto pt = rr.inverseMap(cv::Vec3b(i, i, i));
    for (int c = 0; c < 3; ++c)
      cv::circle(plot, cv::Point(i * x_scale, HEIGHT - pt[c] * y_scale), 3, CHANNELS[c], -1);
  }

  if (options.save) {
    auto output = options.r_response + ".png";
    cv::imwrite(output, plot);
    std::cout << "Saved radiometric response visualization to file \"" << output << "\"" << std::endl;
  }

  cv::imshow("Radiometric response", plot);
  cv::waitKey(-1);

  return 0;
}
