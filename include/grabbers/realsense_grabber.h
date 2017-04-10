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

#include <grabbers/grabber.h>

namespace grabbers {

class RealSenseGrabber : public Grabber {
 public:
  using Ptr = std::shared_ptr<RealSenseGrabber>;

  RealSenseGrabber(const std::string& device_uri = "");

  virtual ~RealSenseGrabber();

  virtual bool hasMoreFrames() const override;

  virtual bool grabFrame(cv::OutputArray color) override;

  virtual void setAutoWhiteBalanceEnabled(bool state = true) override;

  virtual void setAutoExposureEnabled(bool state = true) override;

  virtual void setExposure(int exposure) override;

  virtual int getExposure() const override;

  virtual std::pair<int, int> getExposureRange() const override;

  virtual void setGain(int gain) override;

  virtual int getGain() const override;

  virtual std::pair<int, int> getGainRange() const override;

  virtual std::string getCameraModelName() const override;

  virtual std::string getCameraSerialNumber() const override;

 private:
  struct Impl;
  std::unique_ptr<Impl> p;
};

}  // namespace grabbers
