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

#include "test.h"

#include <radical/exceptions.h>
#include <radical/nonparametric_vignetting_model.h>

using namespace radical;

BOOST_AUTO_TEST_CASE(MatConstructor) {
  // Invalid initialization, should throw
  cv::Mat m;
  BOOST_CHECK_THROW(NonparametricVignettingModel vm(m), MatException);
  m.create(10, 10, CV_8UC3);
  BOOST_CHECK_THROW(NonparametricVignettingModel vm(m), MatTypeException);
  m.create(10, 10, CV_32FC1);
  BOOST_CHECK_THROW(NonparametricVignettingModel vm(m), MatTypeException);
  // Valid initialization
  m.create(10, 10, CV_32FC3);
  m.setTo(1.0f);
  BOOST_CHECK_NO_THROW(NonparametricVignettingModel vm(m));
}

BOOST_AUTO_TEST_CASE(LoadConstructor) {
  // Invalid initialization, should throw
  BOOST_CHECK_THROW(NonparametricVignettingModel vm(getTestFilename("file_that_does_not_exist.vgn")),
                    SerializationException);
  BOOST_CHECK_THROW(NonparametricVignettingModel vm(getTestFilename("vignetting_model_empty.vgn")),
                    SerializationException);
  // Valid initialization
  BOOST_CHECK_NO_THROW(NonparametricVignettingModel vm(getTestFilename("nonparametric_vignetting_model_identity.vgn")));
}

BOOST_AUTO_TEST_CASE(GetName) {
  NonparametricVignettingModel vm(getTestFilename("nonparametric_vignetting_model_identity.vgn"));
  BOOST_CHECK_EQUAL(vm.getName(), "nonparametric");
}

BOOST_AUTO_TEST_CASE(GetImageSize) {
  NonparametricVignettingModel vm(cv::Mat(10, 10, CV_32FC3));
  BOOST_CHECK_EQUAL(vm.getImageSize().width, 10);
  BOOST_CHECK_EQUAL(vm.getImageSize().height, 10);
}

BOOST_AUTO_TEST_CASE(GetModelCoefficients) {
  cv::Mat m(10, 10, CV_32FC3);
  m.setTo(1.0f);
  NonparametricVignettingModel vm(m);
  BOOST_CHECK_EQUAL_MAT(vm.getModelCoefficients(), m, cv::Vec3f);
}

BOOST_AUTO_TEST_CASE(ModelEvaluation) {
  // Model with random numbers
  {
    cv::Mat m(10, 10, CV_32FC3);
    cv::randu(m, cv::Scalar(0, 0, 0), cv::Scalar(1, 1, 1));
    NonparametricVignettingModel vm(m);
    for (int row = 0; row < m.rows; ++row)
      for (int col = 0; col < m.cols; ++col) {
        BOOST_CHECK_EQUAL(vm(cv::Vec2f(row, col)), m.at<cv::Vec3f>(col, row));
        BOOST_CHECK_EQUAL(vm(cv::Vec2f(row + 0.1, col + 0.8)), m.at<cv::Vec3f>(col, row));
        BOOST_CHECK_EQUAL(vm(row, col), m.at<cv::Vec3f>(col, row));
      }
  }
  // Identity loaded from file
  {
    NonparametricVignettingModel vm(getTestFilename("nonparametric_vignetting_model_identity.vgn"));
    for (int row = 0; row < vm.getImageSize().height; ++row)
      for (int col = 0; col < vm.getImageSize().width; ++col) {
        BOOST_CHECK_EQUAL(vm(cv::Vec2f(row, col)), cv::Vec3f(1.0, 1.0, 1.0));
        BOOST_CHECK_EQUAL(vm(row, col), cv::Vec3f(1.0, 1.0, 1.0));
      }
  }
}

BOOST_AUTO_TEST_CASE(SaveLoad) {
  cv::Mat m(10, 10, CV_32FC3);
  m.setTo(1.0f);
  auto f = getTemporaryFilename();
  NonparametricVignettingModel(m).save(f);
  NonparametricVignettingModel vm(f);
  BOOST_CHECK_EQUAL_MAT(vm.getModelCoefficients(), m, cv::Vec3f);
}
