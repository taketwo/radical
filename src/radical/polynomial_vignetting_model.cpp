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

#include <fstream>
#include <iostream>
#include <sstream>

#include <radical/exceptions.h>
#include <radical/polynomial_vignetting_model.h>
#include <radical/check.h>
#include <radical/mat_io.h>

namespace radical {

template <unsigned int Degree>
PolynomialVignettingModel<Degree>::PolynomialVignettingModel(cv::InputArray _coefficients, cv::Size image_size) {
  Check("Polynomial vignetting model", _coefficients).notEmpty().hasSize(Degree + 2).hasType(CV_64FC3);
  coefficients_ = _coefficients.getMat();
  image_size_ = image_size;
}

template <unsigned int Degree>
PolynomialVignettingModel<Degree>::PolynomialVignettingModel(const std::string& filename) {
  std::ifstream file(filename, std::ios::in | std::ios::binary);
  if (file.is_open()) {
    std::string line, name;
    unsigned int degree, width, height;
    {
      std::getline(file, line);
      std::stringstream(line) >> name >> degree >> width >> height;
      if (name != "PolynomialVignettingModel" || degree != Degree) {
        std::stringstream msg;
        msg << "Vignetting model stored in the file is not polynomial of degree " << Degree;
        throw SerializationException(msg.str(), filename);
      }
    }
    image_size_ = cv::Size(width, height);
    coefficients_ = readMat(file);
    // TODO: Check read coefficients
    file.close();
  } else {
    throw SerializationException("Unable to open vignetting model file", filename);
  }
}

template <unsigned int Degree>
std::string PolynomialVignettingModel<Degree>::getName() const {
  std::stringstream name;
  name << "polynomial " << Degree;
  return name.str();
}

template <unsigned int Degree>
void PolynomialVignettingModel<Degree>::save(const std::string& filename) const {
  std::ofstream file(filename, std::ios::out | std::ios::binary);
  if (file.is_open()) {
    std::stringstream header;
    header << "PolynomialVignettingModel " << Degree << " " << image_size_.width << " " << image_size_.height << "\n";
    auto str = header.str();
    file.write(str.c_str(), str.size());
    writeMat(file, coefficients_);
    file.close();
  } else {
    throw SerializationException("Unable to open file to save vignetting model", filename);
  }
}

template <unsigned int Degree>
cv::Vec3f PolynomialVignettingModel<Degree>::operator()(const cv::Vec2f& p) const {
  cv::Vec3f result = {1.0, 1.0, 1.0};
  for (int i = 0; i < 3; ++i) {
    auto coeff = coefficients_.ptr<cv::Vec3d>();
    auto dx = coeff[0][i] - p[0];
    auto dy = coeff[1][i] - p[1];
    double radius_sqr = dx * dx + dy * dy;
    double r = radius_sqr;
    for (unsigned int j = 2; j < Degree + 2; ++j) {
      result[i] += r * coeff[j][i];
      r *= radius_sqr;
    }
  }
  return result;
}

template <unsigned int Degree>
cv::Size PolynomialVignettingModel<Degree>::getImageSize() const {
  return image_size_;
}

template <unsigned int Degree>
cv::Mat PolynomialVignettingModel<Degree>::getModelCoefficients() const {
  return coefficients_;
}

template class PolynomialVignettingModel<3>;

}  // namespace radical
