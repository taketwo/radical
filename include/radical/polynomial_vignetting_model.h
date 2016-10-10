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

#include <radical/vignetting_model.h>

namespace radical {

/** This model parameterizes vignetting response using an even order polynomial.
  *
  * Specifically, vignetting response at a given image location x is computed as:
  * \f[
  *     V(\mathbf{x}) = 1 + \sum_{n=1}^{k}\beta_{n}(\mathbf{x} - \mathbf{c})^{2n},
  * \f]
  *
  * where c is the center of symmetry of the vignetting response.
  * Template argument \c Degree is the order of the polynomial divided by two (k from the formula).
  * Each color channel has its own model coefficients. The number of coefficients per channel is \c Degree + 2. First
  * two numbers define c, and the remaing are betas.
  *
  * \note The implementation is generic and supports polynomials of different degree. However, the model is explicitly
  * instantiated only with \c Degree = 3. */
template <unsigned int Degree>
class PolynomialVignettingModel : public VignettingModel {
 public:
  using Ptr = std::shared_ptr<PolynomialVignettingModel<Degree>>;

  PolynomialVignettingModel(cv::InputArray coefficients, cv::Size image_size);

  PolynomialVignettingModel(const std::string& filename);

  virtual std::string getName() const override;

  virtual void save(const std::string& filename) const override;

  virtual cv::Vec3f operator()(const cv::Vec2f& p) const override;

  using VignettingModel::operator();

  virtual cv::Size getImageSize() const override;

  virtual cv::Mat getModelCoefficients() const override;

 private:
  cv::Mat coefficients_;
  cv::Size image_size_;
};

}  // namespace radical
