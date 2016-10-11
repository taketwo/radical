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

#pragma once

#include <memory>
#include <string>

#include <opencv2/core/core.hpp>

namespace radical {

class VignettingModel {
 public:
  using Ptr = std::shared_ptr<VignettingModel>;
  using ConstPtr = std::shared_ptr<const VignettingModel>;

  virtual ~VignettingModel() {}

  /** Get the name of the vignetting model. */
  virtual std::string getName() const = 0;

  /** Write vignetting model to a file. */
  virtual void save(const std::string& filename) const = 0;

  /** Evaluate the model at a given image location.
    *
    * Pixel coordinates do not need to be integer. Some vignetting models may
    * be able to interpolate between pixels.
    *
    * Note: it is the responsibility of the user to make sure that the location
    * is within valid range (\sa getImageSize()). */
  virtual cv::Vec3f operator()(const cv::Vec2f& p) const = 0;

  /** Evaluate the model at a given pixel location. */
  cv::Vec3f operator()(float x, float y) const {
    return operator()({x, y});
  }

  /** Get image size for which the model is valid. */
  virtual cv::Size getImageSize() const = 0;

  /** Get model coefficients. */
  virtual cv::Mat getModelCoefficients() const = 0;

  /** Load the vignetting model stored in a given file.
    * This function will try to load every implemented vignetting model from the given file. The first one that succeeds
    * in loading will be returned. If the file does not contain any valid vignetting model, \c nullptr is returned. */
  static Ptr load(const std::string& filename);
};

}  // namespace radical
