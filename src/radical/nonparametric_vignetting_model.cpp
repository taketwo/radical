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

#include <radical/exceptions.h>
#include <radical/nonparametric_vignetting_model.h>

#include <radical/check.h>
#include <radical/mat_io.h>

namespace radical {

NonparametricVignettingModel::NonparametricVignettingModel(cv::InputArray _coefficients) {
  Check("Nonparametric vignetting model", _coefficients).notEmpty().hasType(CV_32FC3);
  coefficients_ = _coefficients.getMat();
}

NonparametricVignettingModel::NonparametricVignettingModel(const std::string& filename) {
  std::ifstream file(filename, std::ios::in | std::ios::binary);
  if (file.is_open()) {
    std::string line, name;
    {
      std::getline(file, line);
      std::stringstream(line) >> name;
      if (name != "NonparametricVignettingModel")
        throw SerializationException("Vignetting model stored in the file is not nonparametric", filename);
    }
    coefficients_ = readMat(file);
    file.close();
  } else {
    throw SerializationException("Unable to open vignetting model file", filename);
  }
}

std::string NonparametricVignettingModel::getName() const {
  return "nonparametric";
}

void NonparametricVignettingModel::save(const std::string& filename) const {
  std::ofstream file(filename, std::ios::out | std::ios::binary);
  if (file.is_open()) {
    file << "NonparametricVignettingModel\n";
    writeMat(file, coefficients_);
    file.close();
  } else {
    throw SerializationException("Unable to open file to save vignetting model", filename);
  }
}

cv::Vec3f NonparametricVignettingModel::operator()(const cv::Vec2f& p) const {
  return coefficients_.at<cv::Vec3f>(static_cast<int>(std::floor(p[1])), static_cast<int>(std::floor(p[0])));
}

cv::Size NonparametricVignettingModel::getImageSize() const {
  return coefficients_.size();
}

cv::Mat NonparametricVignettingModel::getModelCoefficients() const {
  return coefficients_;
}

}  // namespace radical
