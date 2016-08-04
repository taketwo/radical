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

#include <boost/throw_exception.hpp>

#include <radical/check.h>
#include <radical/exceptions.h>

namespace radical {

Check::Check(const std::string& name, cv::InputArray m) : name_(name), m_(std::cref(m)) {}

const Check& Check::notEmpty() const {
  if (m_.get().empty())
    BOOST_THROW_EXCEPTION(MatException(name_ + " is empty"));
  return *this;
}

const Check& Check::hasSize(cv::Size size) const {
  if (m_.get().size() != size)
    BOOST_THROW_EXCEPTION(MatException(name_ + " does not have expected size")
                          << MatException::ExpectedSize(size) << MatException::ActualSize(m_.get().size()));
  return *this;
}

const Check& Check::hasSize(int width, int height) const {
  return hasSize({width, height});
}

const Check& Check::hasSize(int total) const {
  if (static_cast<int>(m_.get().total()) != total)
    BOOST_THROW_EXCEPTION(MatException(name_ + " does not have expected size")
                          << MatException::ExpectedSize({total, 1}) << MatException::ActualSize(m_.get().size()));
  return *this;
}

const Check& Check::hasType(int type) const {
  if (m_.get().type() != type)
    BOOST_THROW_EXCEPTION(MatException(name_ + " does not have expected type")
                          << MatException::ExpectedType(type) << MatException::ActualType(m_.get().type()));
  return *this;
}

}  // namespace radical
