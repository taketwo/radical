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

#include <grabbers/grabber.h>
#include <grabbers/openni2_grabber.h>

namespace grabbers {

Grabber::~Grabber() {}

Grabber::Ptr
createGrabber(const std::string& uri)
{

#if HAVE_OPENNI2
  try
  {
    if (uri == "openni" || uri == "openni2" || uri == "kinect" || uri == "asus" || uri == "any")
      return Grabber::Ptr(new OpenNI2Grabber);
    else
      return Grabber::Ptr(new OpenNI2Grabber(uri));
  }
  catch (GrabberException&)
  {
  }
#endif
  BOOST_THROW_EXCEPTION(GrabberException("Failed to create a grabber") <<
                        GrabberException::URI(uri));
  return nullptr;
}

}  // namespace grabbers
