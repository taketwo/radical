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

#include <radical/exceptions.h>
#include <radical/vignetting_model.h>
#include <radical/vignetting_response.h>

#include "utils/program_options.h"

class Options : public OptionsBase {
 public:
  std::string v_response;
  bool fused = false;
  bool save = false;
  std::string colormap = "";
  int colormap_id = -1;

 protected:
  virtual void addOptions(boost::program_options::options_description& desc) override {
    namespace po = boost::program_options;
    desc.add_options()("fused,f", po::bool_switch(&fused), "Display response channels fused as an RGB image")(
        "colormap,c", po::value<std::string>(&colormap), "Display response channels with color map")(
        "save,s", po::bool_switch(&save), "Save to PNG file and exit");
  }

  virtual void addPositional(boost::program_options::options_description& desc,
                             boost::program_options::positional_options_description& positional) override {
    namespace po = boost::program_options;
    desc.add_options()("vignetting", po::value<std::string>(&v_response), "Calibration file with vignetting response");
    positional.add("vignetting", 1);
  }

  virtual void printHelp() override {
    std::cout << "Usage: display_vignetting_response [options] <vignetting-response>" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "Visualizes vignetting response stored in a calibration file. Displays either a" << std::endl;
    std::cout << "single RGB image, where the response channels are fused, or three channels" << std::endl;
    std::cout << "separately (Red, Green, Blue). In the latter mode optionally a color map can be" << std::endl;
    std::cout << "applied (supported only with OpenCV >= 3.0). Color maps: parula, autumn, summer." << std::endl;
    std::cout << "" << std::endl;
    std::cout << "With the --save option the plotted response will be written to the disk instead" << std::endl;
    std::cout << "of showing on the screen. Output file name is constructed by appending \".png\"" << std::endl;
    std::cout << "to the input file path." << std::endl;
    std::cout << "" << std::endl;
  }

  virtual void validate() override {
#if CV_MAJOR_VERSION >= 3
    if (colormap == "parula")
      colormap_id = cv::COLORMAP_PARULA;
    else if (colormap == "autumn")
      colormap_id = cv::COLORMAP_AUTUMN;
    else if (colormap == "summer")
      colormap_id = cv::COLORMAP_SUMMER;
    else if (colormap != "")
      throw boost::program_options::error("unknown colormap name " + colormap);
#else
    if (colormap != "")
      throw boost::program_options::error("OpenCV >= 3.0 is required for colormaps support");
#endif
  }
};

int main(int argc, const char** argv) {
  Options options;
  if (!options.parse(argc, argv))
    return 1;

  radical::VignettingResponse::Ptr vr;
  try {
    vr.reset(new radical::VignettingResponse(options.v_response));
  } catch (radical::SerializationException& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  }

  std::cout << "Loaded vignetting response from file \"" << options.v_response << "\"" << std::endl;
  std::cout << "Vignetting model type: " << vr->getModel()->getName() << std::endl;
  std::cout << "Native image resolution of the model: " << vr->getModel()->getImageSize() << std::endl;

  std::string mode = "fused";
  if (!options.fused) {
    mode = "separate channels";
    if (options.colormap_id != -1)
      mode += " (colormap: " + options.colormap + ")";
  }

  std::cout << "Display mode: " << mode << std::endl;

  cv::Mat response;
  vr->getResponse().convertTo(response, CV_8UC3, 255);

  if (!options.fused) {
    std::vector<cv::Mat> channels;
    cv::split(response, channels);
    cv::Mat stacked;
    for (size_t i = 0; i < channels.size(); ++i)
      stacked.push_back(channels[2 - i]);
    stacked.reshape(response.rows, response.cols * static_cast<int>(channels.size()));
    if (options.colormap_id == -1) {
      response = stacked;
    } else {
#if CV_MAJOR_VERSION >= 3
      cv::Mat colormapped;
      cv::applyColorMap(stacked, colormapped, options.colormap_id);
      response = colormapped;
#endif
    }
  }

  if (options.save) {
    auto output = options.v_response + ".png";
    cv::imwrite(output, response);
    std::cout << "Saved vignetting response visualization to file \"" << output << "\"" << std::endl;
  } else {
    cv::imshow("Vignetting response", response);
    cv::waitKey(-1);
  }

  return 0;
}
