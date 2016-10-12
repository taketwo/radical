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

#ifdef HAVE_CERES

#include <ceres/ceres.h>

#include "model_fitting.h"

using ceres::Solve;
using ceres::Solver;
using ceres::Problem;
using ceres::CauchyLoss;
using ceres::AutoDiffCostFunction;

struct Residual
{
  double x_;
  double y_;
  double i_;

  Residual(int x, int y, float i) : x_(x), y_(y), i_(i) {}

  template <typename T>
  bool operator()(const T* const c, const T* const b, T* residual) const
  {
    auto dx = c[0] - T(x_);
    auto dy = c[1] - T(y_);
    auto r_2 = dx * dx + dy * dy;
    auto r_4 = r_2 * r_2;
    auto r_6 = r_4 * r_2;
    T v = T(1.0 + b[0] * r_2 + b[1] * r_4 + b[2] * r_6);
    residual[0] = T(i_) - v;
    return true;
  }
};

radical::PolynomialVignettingModel<3>::Ptr
fitPolynomialModel(cv::InputArray _data, bool fixed_center, unsigned int max_num_iterations, bool verbose)
{
  cv::Mat_<cv::Vec3f> data = _data.getMat();
  cv::Mat coeff(1, 5, CV_64FC3);
  for (size_t i = 0; i < 3; ++i)
  {
    double c[2];
    double b[3];

    c[0] = data.cols / 2;
    c[1] = data.rows / 2;
    int W = 640 / data.cols;
    b[0] = -7.5e-06 * std::pow(W, 2);
    b[1] =    5e-11 * std::pow(W, 4);
    b[2] =   -2e-16 * std::pow(W, 6);

    Problem problem;
    auto loss = new CauchyLoss(0.5);

    problem.AddParameterBlock(c, 2);
    problem.AddParameterBlock(b, 3);

    for (int y = 0; y < data.rows; ++y)
      for (int x = 0; x < data.cols; ++x)
        problem.AddResidualBlock(new AutoDiffCostFunction<Residual, 1, 2, 3>(new Residual{x, y, data(y, x)[i]}), loss, c, b);

    problem.SetParameterLowerBound(c, 0, c[0] * 0.9);
    problem.SetParameterUpperBound(c, 0, c[0] * 1.1);
    problem.SetParameterLowerBound(c, 1, c[1] * 0.9);
    problem.SetParameterUpperBound(c, 1, c[1] * 1.1);

    if (fixed_center)
      problem.SetParameterBlockConstant(c);

    Solver::Options options;
    options.max_num_iterations = max_num_iterations;
    options.linear_solver_type = ceres::DENSE_QR;
    options.minimizer_progress_to_stdout = false;

    Solver::Summary summary;
    Solve(options, &problem, &summary);
    if (verbose)
      std::cout << summary.FullReport() << std::endl;
    else
      std::cout << summary.BriefReport() << std::endl;

    std::cout << "Final   cx: " << c[0] << " cy: " << c[1] << " b1: " << b[0] << " b2: " << b[1] << " b3: " << b[2] << "\n";
    coeff.at<cv::Vec3d>(0, 0)[i] = c[0];
    coeff.at<cv::Vec3d>(0, 1)[i] = c[1];
    coeff.at<cv::Vec3d>(0, 2)[i] = b[0];
    coeff.at<cv::Vec3d>(0, 3)[i] = b[1];
    coeff.at<cv::Vec3d>(0, 4)[i] = b[2];
  }

  return std::make_shared<radical::PolynomialVignettingModel<3>>(coeff, data.size());
}

#endif
