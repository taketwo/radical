#pragma once

#include <grabbers/grabber.h>

namespace grabbers {

class PylonGrabber : public Grabber {
 public:
  using Ptr = std::shared_ptr<PylonGrabber>;

  PylonGrabber();

  virtual ~PylonGrabber();

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
