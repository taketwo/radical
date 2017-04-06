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

#include <stdexcept>

#include <boost/exception/info.hpp>
#include <boost/exception/exception.hpp>

#include <opencv2/core/core.hpp>

namespace radical {

class Exception : public boost::exception, public std::runtime_error {
 public:
  Exception(const std::string& message) : std::runtime_error(message) {}
};

class SerializationException : public Exception {
 public:
  SerializationException(const std::string& message) : Exception(message) {}
  using Filename = boost::error_info<struct tag_filename, std::string>;
};

class MatException : public Exception {
 public:
  MatException(const std::string& message) : Exception(message) {}
  using ExpectedSize = boost::error_info<struct tag_expected_size, cv::Size>;
  using ActualSize = boost::error_info<struct tag_actual_size, cv::Size>;
  using ExpectedType = boost::error_info<struct tag_expected_type, int>;
  using ActualType = boost::error_info<struct tag_actual_type, int>;
  using ExpectedChannels = boost::error_info<struct tag_expected_channels, int>;
  using ActualChannels = boost::error_info<struct tag_actual_channels, int>;
  using ExpectedDepth = boost::error_info<struct tag_expected_depth, int>;
  using ActualDepth = boost::error_info<struct tag_actual_depth, int>;
};

}  // namespace radical
