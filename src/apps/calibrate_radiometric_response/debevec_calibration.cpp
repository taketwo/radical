/******************************************************************************
 * Copyright (c) 2017 Sergey Alexandrov
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

#ifdef HAVE_CERES

#include <iostream>

#include <boost/assert.hpp>

#include <opencv2/imgproc/imgproc.hpp>

#include <ceres/ceres.h>

#include "utils/colors.h"
#include "utils/plot_radiometric_response.h"

#include "debevec_calibration.h"

struct Residual {
  double t_;
  unsigned char i_;

  Residual(double t, unsigned char i) : t_(t), i_(i) {}

  template <typename T>
  bool operator()(const T* const x, const T* const a, T* residual) const {
    residual[0] = x[0] + T(t_) - a[i_];
    return true;
  }
};

struct RegularizationResidual {
  RegularizationResidual() {}

  template <typename T>
  bool operator()(const T* const a, T* residual) const {
    for (int i = 1; i < 255; ++i)
      residual[i - 1] = a[i - 1] - a[i] * T(2) + a[i + 1];
    return true;
  }
};

struct CeresIterationCallback : public ceres::IterationCallback {
  DebevecCalibration* p;
  bool print_, visualize_;
  CeresIterationCallback(DebevecCalibration* parent, bool print, bool visualize)
  : p(parent), print_(print), visualize_(visualize) {}
  ceres::CallbackReturnType virtual operator()(const ceres::IterationSummary& summary) override {
    if (print_)
      p->printIteration(summary.iteration + 1, summary.cost, summary.cost_change);
    if (visualize_)
      p->visualizeProgress();
    return ceres::SOLVER_CONTINUE;
  }
};

cv::Mat DebevecCalibration::calibrateChannel(const Dataset& dataset) {
  CeresIterationCallback callback{this, verbosity_ > 0, static_cast<bool>(imshow_)};
  dataset_ = &dataset;
  selectPixels();

  X_.resize(locations_.size(), 0);
  U_.create(256, 1);
  for (size_t i = 0; i < 256; ++i)
    U_(i) = std::log(0.5 + i / 256.0);

  auto U = reinterpret_cast<double*>(U_.data);

  printHeader();

  ceres::Problem problem;
  problem.AddParameterBlock(U, 256);
  std::vector<int> constant_255;
  constant_255.push_back(128);
  problem.SetParameterization(U, new ceres::SubsetParameterization(256, constant_255));

  auto loss = new ceres::HuberLoss(0.05);
  auto scaling = new ceres::ScaledLoss(nullptr, lambda_ * lambda_, ceres::DO_NOT_TAKE_OWNERSHIP);

  std::vector<unsigned int> c(locations_.size(), 0);

  // Create residual blocks and compute initial irradiance estimates
  for (const auto& t : dataset_->getExposureTimes()) {
    for (const auto& image : dataset_->getImages(t)) {
      for (size_t i = 0; i < locations_.size(); ++i) {
        auto lt = std::log(t);
        auto p = image.at<uint8_t>(locations_[i]);
        if (isPixelValid(p)) {
          problem.AddResidualBlock(new ceres::AutoDiffCostFunction<Residual, 1, 1, 256>(new Residual{lt, p}), loss,
                                   &X_[i], U);
          X_[i] += U_(p) - lt;
          c[i] += 1;
        }
      }
    }
  }

  problem.AddResidualBlock(
      new ceres::AutoDiffCostFunction<RegularizationResidual, 254, 256>(new RegularizationResidual), scaling, U);

  for (size_t i = 0; i < locations_.size(); ++i)
    X_[i] /= c[i];

  ceres::Solver::Options options;
  options.max_num_iterations = max_num_iterations_;
  options.linear_solver_type = ceres::SPARSE_SCHUR;
  options.logging_type = ceres::SILENT;
  options.minimizer_progress_to_stdout = false;
  options.update_state_every_iteration = true;

  if (verbosity_ || imshow_)
    options.callbacks.push_back(&callback);

  ceres::Solver::Summary summary;
  ceres::Solve(options, &problem, &summary);

  printFooter();

  if (verbosity_ > 1)
    std::cout << summary.FullReport() << std::endl;

  cv::Mat response;
  U_.convertTo(response, CV_32F);
  cv::exp(response, response);

  return response;
}

void DebevecCalibration::selectPixels() {
  BOOST_ASSERT(dataset_ != nullptr);

  locations_.clear();
  auto times = dataset_->getExposureTimes();

  const int M = min_samples_;
  std::vector<int> hist(256, 0);

  for (size_t i = 0; i < times.size(); ++i) {
    auto t = times[i];
    auto image = dataset_->getImages(t)[0];

    cv::Mat indices;
    cv::Mat flat = image.reshape(1, 1);
    cv::sortIdx(flat, indices, cv::SORT_EVERY_ROW + cv::SORT_DESCENDING);

    int x = 0;
    unsigned char intensity = max_valid_;

    while (intensity >= min_valid_) {
      while (hist[intensity] < M && x < indices.size().area()) {
        auto index = indices.at<int>(x++);
        auto p = flat.at<uint8_t>(index);
        if (p > intensity)
          continue;
        else if (p < intensity) {
          --x;  // to repeat this iteration again
          break;
        }
        locations_.push_back(index);
        for (size_t j = i; j < times.size(); ++j) {
          auto t = times[j];
          auto p = dataset_->getImages(t)[0].at<uint8_t>(index);
          if (!isPixelValid(p))
            continue;
          ++hist[p];
        }
      }
      intensity--;
    }

    break;
  }
}

void DebevecCalibration::visualizeProgress() {
  cv::Mat response;
  U_.convertTo(response, CV_32F);
  cv::exp(response, response);

  cv::Mat canvas(512, 512, CV_8UC3);
  canvas.setTo(255);

  double min, max;
  cv::minMaxIdx(response, &min, &max);

  response -= min;
  response /= (max - min);

  float x_scale = static_cast<float>(canvas.size().width) / 256;
  float y_scale = static_cast<float>(canvas.size().height) / 1.0;

  for (const auto& t : dataset_->getExposureTimes()) {
    for (const auto& image : dataset_->getImages(t)) {
      for (size_t i = 0; i < locations_.size(); ++i) {
        auto p = image.at<uint8_t>(locations_[i]);
        if (isPixelValid(p)) {
          auto v = std::exp(X_[i]) * t;
          cv::circle(canvas, cv::Point(p * x_scale, canvas.size().height - (v - min) / (max - min) * y_scale),
                     std::ceil(x_scale), utils::colors::BGR_LIGHT[channel_], -1);
        }
      }
    }
  }

  utils::plotRadiometricResponse(response, canvas, utils::colors::BGR[channel_]);
  imshow_(canvas);
}

void DebevecCalibration::setMinSamplesPerIntensityLevel(unsigned int min_samples) {
  min_samples_ = min_samples;
}

void DebevecCalibration::setSmoothingLambda(double lambda) {
  lambda_ = lambda;
}

#endif
