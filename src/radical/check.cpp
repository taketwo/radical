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

#include <radical/exceptions.h>
#include <radical/check.h>

namespace radical {

Check::Check(const std::string& name, cv::InputArray m) : name_(name), m_(std::cref(m)) {}

const Check& Check::hasChannels(int channels) const {
  if (m_.get().channels() != channels)
    throw MatChannelsException(name_, channels, m_.get().channels());
  return *this;
}

const Check& Check::hasDepth(int depth) const {
  if (m_.get().depth() != depth)
    throw MatDepthException(name_, depth, m_.get().depth());
  return *this;
}

const Check& Check::hasMaxDimensions(int max_dims) const {
  if (m_.get().getMat().dims > max_dims)
    throw MatMaxDimensionsException(name_, max_dims, m_.get().getMat().dims);
  return *this;
}

const Check& Check::hasSize(cv::Size size) const {
  if (m_.get().size() != size)
    throw MatSizeException(name_, size, m_.get().size());
  return *this;
}

const Check& Check::hasSize(int width, int height) const {
  return hasSize({width, height});
}

const Check& Check::hasSize(int total) const {
  if (static_cast<int>(m_.get().total()) != total)
    throw MatSizeException(name_, {total, 1}, m_.get().size());
  return *this;
}

const Check& Check::hasType(int type) const {
  if (m_.get().type() != type)
    throw MatTypeException(name_, type, m_.get().type());
  return *this;
}

const Check& Check::isContinuous() const {
  if (!m_.get().getMat().isContinuous())
    throw MatException(name_ + " is not continuous");
  return *this;
}

const Check& Check::notEmpty() const {
  if (m_.get().empty())
    throw MatException(name_ + " is empty");
  return *this;
}

}  // namespace radical
