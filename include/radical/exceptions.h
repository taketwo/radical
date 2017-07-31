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

#include <stdexcept>

#include <opencv2/core/core.hpp>

namespace radical {

class Exception : public std::runtime_error {
 public:
  explicit Exception(const std::string& message) : std::runtime_error(message) {}
};

class SerializationException : public Exception {
 public:
  explicit SerializationException(const std::string& message) : Exception(message) {}

  explicit SerializationException(const std::string& message, const std::string& filename)
  : Exception(message), filename(filename) {}

  const std::string filename;
};

class MatException : public Exception {
 public:
  explicit MatException(const std::string& message) : Exception(message) {}
};

class MatChannelsException : public MatException {
 public:
  explicit MatChannelsException(const std::string& mat_name, int expected_channels, int actual_channels)
  : MatException(mat_name + " does not have expected number of channels"), expected_channels(expected_channels),
    actual_channels(actual_channels) {}

  const int expected_channels;
  const int actual_channels;
};

class MatDepthException : public MatException {
 public:
  explicit MatDepthException(const std::string& mat_name, int expected_depth, int actual_depth)
  : MatException(mat_name + " does not have expected depth"), expected_depth(expected_depth),
    actual_depth(actual_depth) {}

  const int expected_depth;
  const int actual_depth;
};

class MatMaxDimensionsException : public MatException {
 public:
  explicit MatMaxDimensionsException(const std::string& mat_name, int expected_max_dims, int actual_dims)
  : MatException(mat_name + " has more than expected dimensions"), expected_max_dims(expected_max_dims),
    actual_dims(actual_dims) {}

  const int expected_max_dims;
  const int actual_dims;
};

class MatSizeException : public MatException {
 public:
  explicit MatSizeException(const std::string& mat_name, const cv::Size& expected_size, const cv::Size& actual_size)
  : MatException(mat_name + " does not have expected size"), expected_size(expected_size), actual_size(actual_size) {}

  const cv::Size expected_size;
  const cv::Size actual_size;
};

class MatTypeException : public MatException {
 public:
  explicit MatTypeException(const std::string& mat_name, int expected_type, int actual_type)
  : MatException(mat_name + " does not have expected type"), expected_type(expected_type), actual_type(actual_type) {}

  const int expected_type;
  const int actual_type;
};

}  // namespace radical
