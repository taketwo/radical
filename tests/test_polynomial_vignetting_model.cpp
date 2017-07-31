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
#include <radical/polynomial_vignetting_model.h>

using namespace radical;

BOOST_AUTO_TEST_CASE(MatConstructor) {
  // Invalid initialization, should throw
  cv::Mat m;
  cv::Size s(100, 100);
  BOOST_CHECK_THROW(PolynomialVignettingModel<3> vm(m, s), MatException);
  m.create(10, 10, CV_64FC3);
  BOOST_CHECK_THROW(PolynomialVignettingModel<3> vm(m, s), MatSizeException);
  m.create(1, 4, CV_64FC3);
  BOOST_CHECK_THROW(PolynomialVignettingModel<3> vm(m, s), MatSizeException);
  m.create(1, 5, CV_32FC1);
  BOOST_CHECK_THROW(PolynomialVignettingModel<3> vm(m, s), MatTypeException);
  // Valid initialization
  m.create(1, 5, CV_64FC3);
  m.setTo(1.0f);
  BOOST_CHECK_NO_THROW(PolynomialVignettingModel<3> vm(m, s));
  m.create(5, 1, CV_64FC3);
  m.setTo(1.0f);
  BOOST_CHECK_NO_THROW(PolynomialVignettingModel<3> vm(m, s));
}

BOOST_AUTO_TEST_CASE(LoadConstructor) {
  // Invalid initialization, should throw
  BOOST_CHECK_THROW(PolynomialVignettingModel<3> vm(getTestFilename("file_that_does_not_exist.vgn")),
                    SerializationException);
  BOOST_CHECK_THROW(PolynomialVignettingModel<3> vm(getTestFilename("vignetting_model_empty.vgn")),
                    SerializationException);
  // Valid initialization
  BOOST_CHECK_NO_THROW(PolynomialVignettingModel<3> vm(getTestFilename("polynomial_vignetting_model_identity.vgn")));
}

BOOST_AUTO_TEST_CASE(GetName) {
  PolynomialVignettingModel<3> vm(getTestFilename("polynomial_vignetting_model_identity.vgn"));
  BOOST_CHECK_EQUAL(vm.getName(), "polynomial 3");
}

BOOST_AUTO_TEST_CASE(GetImageSize) {
  PolynomialVignettingModel<3> vm(cv::Mat(1, 5, CV_64FC3), cv::Size(10, 10));
  BOOST_CHECK_EQUAL(vm.getImageSize().width, 10);
  BOOST_CHECK_EQUAL(vm.getImageSize().height, 10);
}

BOOST_AUTO_TEST_CASE(GetModelCoefficients) {
  cv::Mat m(5, 1, CV_64FC3);
  m.setTo(1.0f);
  PolynomialVignettingModel<3> vm(m, cv::Size(10, 10));
  BOOST_CHECK_EQUAL_MAT(vm.getModelCoefficients(), m, cv::Vec3d);
}

BOOST_AUTO_TEST_CASE(ModelEvaluation) {
  // Identity loaded from file
  {
    PolynomialVignettingModel<3> vm(getTestFilename("polynomial_vignetting_model_identity.vgn"));
    for (int row = 0; row < vm.getImageSize().height; ++row)
      for (int col = 0; col < vm.getImageSize().width; ++col) {
        BOOST_CHECK_EQUAL(vm(cv::Vec2f(row, col)), cv::Vec3f(1.0, 1.0, 1.0));
        BOOST_CHECK_EQUAL(vm(row, col), cv::Vec3f(1.0, 1.0, 1.0));
      }
  }
}

BOOST_AUTO_TEST_CASE(SaveLoad) {
  cv::Mat m(1, 5, CV_64FC3);
  m.setTo(1.0f);
  auto f = getTemporaryFilename();
  PolynomialVignettingModel<3>(m, cv::Size(640, 480)).save(f);
  PolynomialVignettingModel<3> vm(f);
  BOOST_CHECK_EQUAL_MAT(vm.getModelCoefficients(), m, cv::Vec3d);
  BOOST_CHECK_EQUAL(vm.getImageSize(), cv::Size(640, 480));
}
