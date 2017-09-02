/******************************************************************************
 * Copyright (c) 2017 Sergey Alexandrov
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

// In OpenCV 3 imread has been moved to imgcodecs module
#if CV_MAJOR_VERSION >= 3
#include <opencv2/imgcodecs/imgcodecs.hpp>
#endif

#include <boost/algorithm/string/predicate.hpp>

#include <radical/mat_io.h>
#include <radical/radiometric_response.h>
#include <radical/vignetting_response.h>

#include "grabbers/grabber.h"

#include "utils/arrange_images_in_grid.h"
#include "utils/key_code.h"
#include "utils/program_options.h"

using utils::KeyCode;

struct Options : public OptionsBase {
  std::string crf = "";
  std::string vgn = "";
  std::string source = "";
  bool alternate = false;
  float scale = 0.7;
  bool save = false;

  virtual void addOptions(boost::program_options::options_description& desc) override {
    namespace po = boost::program_options;
    desc.add_options()("alternate,a", po::bool_switch(&alternate),
                       "Alternate between original and cleared images on keypress");
    desc.add_options()("scale,s", po::value<float>(&scale)->default_value(scale),
                       "Scale the cleared irradiance map before re-applying camera response function");
    desc.add_options()("save", po::bool_switch(&save), "Save the cleared image");
  }

  virtual void addPositional(boost::program_options::options_description& desc,
                             boost::program_options::positional_options_description& positional) override {
    namespace po = boost::program_options;
    desc.add_options()("radiometric", po::value<std::string>(&crf), "Calibration file with radiometric response");
    desc.add_options()("vignetting", po::value<std::string>(&vgn), "Calibration file with vignetting response");
    desc.add_options()("image-source", po::value<std::string>(&source),
                       "Image source, either a camera (\"asus\", \"intel\"), or a PNG/JPG/MAT image");
    positional.add("radiometric", 1);
    positional.add("vignetting", 1);
    positional.add("image-source", -1);
  }

  virtual void printHelp() override {
    std::cout << "Usage: remove_vignetting [options] <radiometric-response> <vignetting-response> <image-source>"
              << std::endl;
    std::cout << "" << std::endl;
    std::cout << "Remove vignetting effects from a given image (png, jpg, or mat) stored in the " << std::endl;
    std::cout << "filesystem, or from images streamed by a camera. The original and cleared images are " << std::endl;
    std::cout << "displayed either side-by-side or alternatively (toggled by a keypress)." << std::endl;
    std::cout << "" << std::endl;
    std::cout << "The --scale option allows to adjust the brightness before re-applying the camera " << std::endl;
    std::cout << "response function. The end effect is same as changing the exposure time." << std::endl;
    std::cout << "" << std::endl;
    std::cout << "The --save option enables writing of the cleared image to the filesystem. The output " << std::endl;
    std::cout << "file will have the same extension as the input, and the \".clear\" suffix will be " << std::endl;
    std::cout << "added to the file stem. Note that this option is valid only for a single file input, " << std::endl;
    std::cout << "not a camera stream." << std::endl;
    std::cout << "" << std::endl;
    std::cout << "To exit the app press Esc." << std::endl;
    std::cout << "" << std::endl;
  }
};

class ImageDisplay {
 public:
  ImageDisplay(bool side_by_side = true) : side_by_side_(side_by_side), alternate_(1) {}

  bool operator()(const cv::Mat& img1, const cv::Mat& img2, int delay = 0) {
    if (delay <= 0) {
      KeyCode key = 0;
      while (key != KeyCode::ESC)
        key = show(img1, img2, delay);
      return true;
    }
    return show(img1, img2, delay) == KeyCode::ESC;
  }

 private:
  KeyCode show(const cv::Mat& img1, const cv::Mat& img2, int delay) {
    if (side_by_side_) {
      auto m = arrangeImagesInGrid({img1, img2}, {2, 1});
      cv::imshow("Images", m);
    } else {
      if (alternate_ == 0)
        cv::imshow("Image", img1);
      else if (alternate_ == 1)
        cv::imshow("Image", img2);
    }
    KeyCode key = cv::waitKey(delay);
    if (key != KeyCode::NO_KEY)
      alternate_ = (alternate_ + 1) % 2;
    return key;
  }

  bool side_by_side_;
  int alternate_;
};

int main(int argc, const char** argv) {
  Options options;
  if (!options.parse(argc, argv))
    return 1;

  radical::RadiometricResponse rr(options.crf);
  radical::VignettingResponse vr(options.vgn);

  auto remove = [&](const cv::Mat& img) {
    static cv::Mat tmp1, tmp2, img_cleared;
    rr.inverseMap(img, tmp1);
    cv::multiply(tmp1, options.scale, tmp1);
    vr.remove(tmp1, tmp2);
    rr.directMap(tmp2, img_cleared);
    return img_cleared;
  };

  auto addSuffix = [](const std::string& path) {
    return path.substr(0, path.length() - 4) + ".clear" + path.substr(path.length() - 4);
  };

  ImageDisplay display(!options.alternate);

  cv::Mat img;
  if (boost::ends_with(options.source, ".png") || boost::ends_with(options.source, ".jpg")) {
    img = cv::imread(options.source);
    display(img, remove(img));
    if (options.save)
      cv::imwrite(addSuffix(options.source), remove(img));
  } else if (boost::ends_with(options.source, ".mat")) {
    img = radical::readMat(options.source);
    display(img, remove(img));
    if (options.save)
      radical::writeMat(addSuffix(options.source), remove(img));
  } else {
    grabbers::Grabber::Ptr grabber;

    try {
      grabber = grabbers::createGrabber(options.source);
    } catch (grabbers::GrabberException&) {
      std::cerr << "Failed to create a grabber" << (options.source != "" ? " for camera " + options.source : "")
                << std::endl;
      return 1;
    }

    while (grabber->hasMoreFrames()) {
      grabber->grabFrame(img);
      if (display(img, remove(img), 30))
        break;
    }

    if (options.save) {
      std::cerr << "Saving cleared images is not supported when the input is a camera stream" << std::endl;
      return 2;
    }
  }

  return 0;
}
