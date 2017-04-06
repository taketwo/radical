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

#pragma once

#include <opencv2/core/core.hpp>

#include <radical/radiometric_response.h>

namespace utils {

/** Plot (inverse) radiometric response (specified as a LUT) to a given canvas.
  *
  * If the response has a single channel, then \arg color should specify the color of the plotted curve. If the response
  * has three channels, then \arg color will be ignored and the curves will be plotted with Brue, Green, Red colors
  * respectively. */
void plotRadiometricResponse(const cv::Mat& response, cv::Mat& canvas, const cv::Scalar& color = {0, 0, 0, 0});

cv::Mat plotRadiometricResponse(const cv::Mat& response, const cv::Size& size = {512, 512},
                                const cv::Scalar& color = {0, 0, 0, 0});

cv::Mat plotRadiometricResponse(const radical::RadiometricResponse& rr, cv::Size size = {512, 512});
}
