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
#include <radical/radiometric_response.h>

using namespace radical;

BOOST_AUTO_TEST_CASE(MatConstructor) {
  // Invalid initialization, should throw
  cv::Mat m;
  BOOST_CHECK_THROW(RadiometricResponse rr(m), MatException);
  m.create(1, 100, CV_32FC3);
  BOOST_CHECK_THROW(RadiometricResponse rr(m), MatSizeException);
  m.create(1, 256, CV_32FC1);
  BOOST_CHECK_THROW(RadiometricResponse rr(m), MatTypeException);
  m.create(1, 256, CV_8UC3);
  BOOST_CHECK_THROW(RadiometricResponse rr(m), MatTypeException);
  // Valid initialization
  m.create(1, 256, CV_32FC3);
  m.setTo(1.0f);
  BOOST_CHECK_NO_THROW(RadiometricResponse rr(m));
  // 256x1 is also valid
  m.create(256, 1, CV_32FC3);
  m.setTo(1.0f);
  BOOST_CHECK_NO_THROW(RadiometricResponse rr(m));
}

BOOST_AUTO_TEST_CASE(LoadConstructor) {
  // Invalid initialization, should throw
  BOOST_CHECK_THROW(RadiometricResponse rr(getTestFilename("file_that_does_not_exist.crf")), SerializationException);
  BOOST_CHECK_THROW(RadiometricResponse rr(getTestFilename("radiometric_response_empty.crf")), SerializationException);
  BOOST_CHECK_THROW(RadiometricResponse rr(getTestFilename("radiometric_response_invalid.crf")),
                    SerializationException);
  BOOST_CHECK_NO_THROW(RadiometricResponse rr(getTestFilename("radiometric_response_constant.crf")));
  BOOST_CHECK_NO_THROW(RadiometricResponse rr(getTestFilename("radiometric_response_identity.crf")));
}

BOOST_AUTO_TEST_CASE(DirectMapPixel) {
  {
    RadiometricResponse rr(getTestFilename("radiometric_response_identity.crf"));
    BOOST_CHECK_EQUAL(rr.directMap(cv::Vec3f(0, 0, 0)), cv::Vec3b(0, 0, 0));
    BOOST_CHECK_EQUAL(rr.directMap(cv::Vec3f(100, 200, 255)), cv::Vec3b(100, 200, 255));
  }
  {
    RadiometricResponse rr(getTestFilename("radiometric_response_scaling.crf"));
    BOOST_CHECK_EQUAL(rr.directMap(cv::Vec3f(1, 10, 100)), cv::Vec3b(1, 1, 1));
  }
}

BOOST_AUTO_TEST_CASE(DirectMapImage) {
  RadiometricResponse rr(getTestFilename("radiometric_response_identity.crf"));
  cv::Mat E(256, 1, CV_32FC3);
  cv::Mat I_expected(256, 1, CV_8UC3);
  for (int j = 0; j < 256; ++j) {
    E.at<cv::Vec3f>(0, j) = cv::Vec3f(j, j + 10, 255 - j);
    I_expected.at<cv::Vec3b>(0, j) = cv::Vec3b(j, std::min(255, j + 10), 255 - j);
  }
  cv::Mat I;
  rr.directMap(E, I);
  BOOST_CHECK_EQUAL_MAT(I, I_expected, cv::Vec3b);
}

BOOST_AUTO_TEST_CASE(DirectMapImageInvalid) {
  RadiometricResponse rr(getTestFilename("radiometric_response_identity.crf"));
  cv::Mat E, I;
  rr.directMap(E, I);
  BOOST_CHECK(I.empty());
  E.create(10, 10, CV_32FC2);
  BOOST_CHECK_THROW(rr.directMap(E, I), MatException);
}

BOOST_AUTO_TEST_CASE(InverseMapPixel) {
  auto log = [](const cv::Vec3f& v) { return cv::Vec3f(std::log(v[0]), std::log(v[1]), std::log(v[2])); };
  {
    RadiometricResponse rr(getTestFilename("radiometric_response_identity.crf"));
    // Normal map
    BOOST_CHECK_EQUAL(rr.inverseMap(cv::Vec3b(0, 0, 0)), cv::Vec3f(0, 0, 0));
    BOOST_CHECK_EQUAL(rr.inverseMap(cv::Vec3b(100, 200, 255)), cv::Vec3f(100, 200, 255));
    // Logarithm map
    BOOST_CHECK_EQUAL(rr.inverseLogMap(cv::Vec3b(1, 1, 1)), log(cv::Vec3f(1, 1, 1)));
    BOOST_CHECK_EQUAL(rr.inverseLogMap(cv::Vec3b(100, 200, 255)), log(cv::Vec3f(100, 200, 255)));
  }
  {
    RadiometricResponse rr(getTestFilename("radiometric_response_scaling.crf"));
    // Normal map
    BOOST_CHECK_EQUAL(rr.inverseMap(cv::Vec3b(0, 0, 0)), cv::Vec3f(0, 0, 0));
    BOOST_CHECK_EQUAL(rr.inverseMap(cv::Vec3b(1, 1, 1)), cv::Vec3f(1, 10, 100));
    BOOST_CHECK_EQUAL(rr.inverseMap(cv::Vec3b(2, 3, 4)), cv::Vec3f(2, 30, 400));
    // Logarithm map
    BOOST_CHECK_EQUAL(rr.inverseLogMap(cv::Vec3b(1, 1, 1)), log(cv::Vec3f(1, 10, 100)));
    BOOST_CHECK_EQUAL(rr.inverseLogMap(cv::Vec3b(2, 3, 4)), log(cv::Vec3f(2, 30, 400)));
  }
}

BOOST_AUTO_TEST_CASE(InverseMapImage) {
  RadiometricResponse rr(getTestFilename("radiometric_response_identity.crf"));
  cv::Mat I(256, 1, CV_8UC3);
  cv::Mat E_expected(256, 1, CV_32FC3), E_expected_log;
  for (int j = 0; j < 256; ++j) {
    I.at<cv::Vec3b>(0, j) = cv::Vec3b(j, j, 255 - j);
    E_expected.at<cv::Vec3f>(0, j) = cv::Vec3f(j, j, 255 - j);
  }
  cv::log(E_expected, E_expected_log);
  cv::Mat E;
  // Normal version
  rr.inverseMap(I, E);
  BOOST_CHECK_EQUAL_MAT(E, E_expected, cv::Vec3f);
  // Logarithm version
  rr.inverseLogMap(I, E);
  BOOST_CHECK_EQUAL_MAT(E, E_expected_log, cv::Vec3f);
}

BOOST_AUTO_TEST_CASE(InverseMapImageInvalid) {
  RadiometricResponse rr(getTestFilename("radiometric_response_identity.crf"));
  cv::Mat I, E;
  // Normal version
  rr.inverseMap(I, E);
  BOOST_CHECK(E.empty());
  // Logarithm version
  rr.inverseLogMap(I, E);
  BOOST_CHECK(E.empty());
  I.create(10, 10, CV_8UC1);
  BOOST_CHECK_THROW(rr.directMap(I, E), MatTypeException);
}

BOOST_AUTO_TEST_CASE(SaveLoad) {
  cv::Mat response(256, 1, CV_32FC3);
  response.setTo(10);
  auto f = getTemporaryFilename();
  RadiometricResponse(response).save(f);
  RadiometricResponse rr(f);
  BOOST_CHECK_EQUAL_MAT(rr.getInverseResponse(), response, cv::Vec3f);
}
