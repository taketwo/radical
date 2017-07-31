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
#include <radical/vignetting_response.h>

using namespace radical;

BOOST_AUTO_TEST_CASE(Constructor) {
  // Invalid initialization, should throw
  BOOST_CHECK_THROW(VignettingResponse vv(getTestFilename("vignetting_model_empty.vgn")), SerializationException);
  // Valid initialization
  BOOST_CHECK_NO_THROW(VignettingResponse vv(getTestFilename("nonparametric_vignetting_model_identity.vgn")));
}

BOOST_AUTO_TEST_CASE(GetModel) {
  VignettingResponse vv(getTestFilename("nonparametric_vignetting_model_identity.vgn"));
  BOOST_CHECK(vv.getModel() != nullptr);
}

BOOST_AUTO_TEST_CASE(GetResponse) {
  VignettingResponse vv(getTestFilename("nonparametric_vignetting_model_identity.vgn"));
  {
    cv::Mat response_expected(10, 10, CV_32FC3);
    // Normal
    response_expected.setTo(1.0);
    BOOST_CHECK_EQUAL_MAT(vv.getResponse(), response_expected, cv::Vec3f);
    BOOST_CHECK_EQUAL_MAT(vv.getResponse({10, 10}), response_expected, cv::Vec3f);
    // Logarithm
    response_expected.setTo(0.0);
    BOOST_CHECK_EQUAL_MAT(vv.getLogResponse(), response_expected, cv::Vec3f);
    BOOST_CHECK_EQUAL_MAT(vv.getLogResponse({10, 10}), response_expected, cv::Vec3f);
  }
  {
    cv::Mat response_expected(4, 4, CV_32FC3);
    // Normal
    response_expected.setTo(1.0);
    BOOST_CHECK_EQUAL_MAT(vv.getResponse({4, 4}), response_expected, cv::Vec3f);
    // Logarithm
    response_expected.setTo(0.0);
    BOOST_CHECK_EQUAL_MAT(vv.getLogResponse({4, 4}), response_expected, cv::Vec3f);
  }
  {
    cv::Mat response_expected(20, 20, CV_32FC3);
    // Normal
    response_expected.setTo(1.0);
    BOOST_CHECK_EQUAL_MAT(vv.getResponse({20, 20}), response_expected, cv::Vec3f);
    // Logarithm
    response_expected.setTo(0.0);
    BOOST_CHECK_EQUAL_MAT(vv.getLogResponse({20, 20}), response_expected, cv::Vec3f);
  }
  {
    cv::Mat response_expected(53, 53, CV_32FC3);
    // Normal
    response_expected.setTo(1.0);
    BOOST_CHECK_EQUAL_MAT(vv.getResponse({53, 53}), response_expected, cv::Vec3f);
    // Logarithm
    response_expected.setTo(0.0);
    BOOST_CHECK_EQUAL_MAT(vv.getLogResponse({53, 53}), response_expected, cv::Vec3f);
  }
}

BOOST_AUTO_TEST_CASE(GetResponseInvalidScale) {
  VignettingResponse vv(getTestFilename("nonparametric_vignetting_model_identity.vgn"));
  BOOST_CHECK_THROW(vv.getResponse({30, 10}), Exception);
  BOOST_CHECK_THROW(vv.getLogResponse({30, 10}), Exception);
}

BOOST_AUTO_TEST_CASE(RemoveInvalid) {
  VignettingResponse vm(getTestFilename("nonparametric_vignetting_model_identity.vgn"));
  cv::Mat E, L;
  vm.remove(E, L);
  BOOST_CHECK(L.empty());
  E.create(10, 10, CV_32FC2);
  BOOST_CHECK_THROW(vm.remove(E, L), MatTypeException);
  BOOST_CHECK_THROW(vm.removeLog(E, L), MatTypeException);
  E.create(20, 10, CV_32FC3);
  BOOST_CHECK_THROW(vm.remove(E, L), Exception);     // invalid aspect ratio
  BOOST_CHECK_THROW(vm.removeLog(E, L), Exception);  // invalid aspect ratio
}

BOOST_AUTO_TEST_CASE(RemoveIdentity) {
  cv::Mat L;
  cv::Mat E(10, 10, CV_32FC3);
  cv::randu(E, cv::Scalar(0, 0, 0), cv::Scalar(1, 1, 1));
  VignettingResponse vm(getTestFilename("nonparametric_vignetting_model_identity.vgn"));
  // Normal
  vm.remove(E, L);
  BOOST_CHECK_EQUAL_MAT(L, E, cv::Vec3f);
  // Logarithm
  vm.removeLog(E, L);
  BOOST_CHECK_EQUAL_MAT(L, E, cv::Vec3f);
}

BOOST_AUTO_TEST_CASE(AddInvalid) {
  VignettingResponse vm(getTestFilename("nonparametric_vignetting_model_identity.vgn"));
  cv::Mat L, E;
  vm.add(L, E);
  BOOST_CHECK(E.empty());
  L.create(10, 10, CV_32FC2);
  BOOST_CHECK_THROW(vm.add(L, E), MatTypeException);
  BOOST_CHECK_THROW(vm.addLog(L, E), MatTypeException);
  L.create(10, 20, CV_32FC2);
  BOOST_CHECK_THROW(vm.add(L, E), Exception);     // invalid aspect ratio
  BOOST_CHECK_THROW(vm.addLog(L, E), Exception);  // invalid aspect ratio
}

BOOST_AUTO_TEST_CASE(AddIdentity) {
  cv::Mat E;
  cv::Mat L(10, 10, CV_32FC3);
  cv::randu(L, cv::Scalar(0, 0, 0), cv::Scalar(1, 1, 1));
  VignettingResponse vm(getTestFilename("nonparametric_vignetting_model_identity.vgn"));
  // Normal
  vm.add(L, E);
  BOOST_CHECK_EQUAL_MAT(E, L, cv::Vec3f);
  // Logarithm
  vm.addLog(L, E);
  BOOST_CHECK_EQUAL_MAT(E, L, cv::Vec3f);
}
