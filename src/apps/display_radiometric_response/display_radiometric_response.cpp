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

#include "utils/plot_radiometric_response.h"
#include "utils/program_options.h"

class Options : public OptionsBase {
 public:
  std::string r_response;
  bool save = false;

 protected:
  virtual void addOptions(boost::program_options::options_description& desc) override {
    namespace po = boost::program_options;
    desc.add_options()("save,s", po::bool_switch(&save), "Save to PNG file and exit");
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
    std::cout << "Plots radiometric response stored in a calibration file and displays it on the screen." << std::endl;
    std::cout << "" << std::endl;
    std::cout << "With the --save option the plot will be written to the disk instead of showing on the screen." << std::endl;
    std::cout << "Output file name is constructed by appending \".png\" to the input file path." << std::endl;
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

  auto plot = utils::plotRadiometricResponse(rr);

  if (options.save) {
    auto output = options.r_response + ".png";
    cv::imwrite(output, plot);
    std::cout << "Saved radiometric response visualization to file \"" << output << "\"" << std::endl;
  } else {
    cv::imshow("Radiometric response", plot);
    cv::waitKey(-1);
  }

  return 0;
}
