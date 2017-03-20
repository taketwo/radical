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

#include "test.h"

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/count.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/variance.hpp>
#include <boost/accumulators/statistics/weighted_mean.hpp>
#include <boost/accumulators/statistics/weighted_variance.hpp>
#include <boost/mpl/list.hpp>

#include <radical/exceptions.h>

#include "utils/mean_image.h"

using namespace utils;

using ImageTypes = boost::mpl::list<unsigned char, unsigned short, int, float, double>;

const double TOLERANCE_DOUBLE = 1e-11;
const double TOLERANCE_FLOAT = 1e-6;

template <typename T>
T div0(T a, T b) {
  return b == 0.0 ? 0.0 : a / b;
}

// Behavior in uninitialized state (before first add)
BOOST_AUTO_TEST_CASE(UninitializedState) {
  MeanImage mi(false, 0);
  BOOST_REQUIRE(mi.getMean().empty());
  BOOST_REQUIRE(mi.getVariance().empty());
  BOOST_REQUIRE(mi.getVarianceInverse().empty());
}

// Return value of add/addWeighted in simple cases (no masking)
BOOST_AUTO_TEST_CASE(AddReturn) {
  {
    MeanImage mi(false, 1);
    BOOST_REQUIRE(mi.add(generateRandomImage(1, 1)) == true);               // should be done after adding one image
    BOOST_REQUIRE(mi.add(generateRandomImage(1, 1)) == true);               // and again...
    BOOST_REQUIRE(mi.add(generateRandomImage(1, 1)) == true);               // and again..
    BOOST_REQUIRE(mi.addWeighted(generateRandomImage(1, 1), 1.0) == true);  // and again..
  }
  {
    MeanImage mi(false, 2);
    BOOST_REQUIRE(mi.add(generateRandomImage(1, 1)) == false);               // need one more
    BOOST_REQUIRE(mi.add(generateRandomImage(1, 1)) == true);                // should be done after adding two images
    BOOST_REQUIRE(mi.add(generateRandomImage(1, 1)) == false);               // reset and need one more
    BOOST_REQUIRE(mi.add(generateRandomImage(1, 1)) == true);                // should be done after adding two images
    BOOST_REQUIRE(mi.addWeighted(generateRandomImage(1, 1), 1.0) == false);  // reset and need one more
    BOOST_REQUIRE(mi.addWeighted(generateRandomImage(1, 1), 1.0) == true);   // should be done after adding two images
  }
  {
    MeanImage mi(false, 0);
    BOOST_REQUIRE(mi.add(generateRandomImage(1, 1)) == false);               // will never be done
    BOOST_REQUIRE(mi.add(generateRandomImage(1, 1)) == false);               // never
    BOOST_REQUIRE(mi.addWeighted(generateRandomImage(1, 1), 1.0) == false);  // ever
    BOOST_REQUIRE(mi.add(generateRandomImage(1, 1)) == false);               // forever
  }
}

// Compatibility checks for the size/type of added images (no resetting)
BOOST_AUTO_TEST_CASE(AddMatCompatibilty) {
  cv::Mat image1(10, 5, CV_32FC3);
  cv::Mat image2(10, 5, CV_32FC2);
  cv::Mat image3(10, 5, CV_8UC1);
  cv::Mat image4(10, 5, CV_8UC3);
  cv::Mat image5(5, 10, CV_8UC3);
  cv::Mat image6(5, 10, CV_8UC1);
  cv::Mat image7(1, 5, CV_32FC3);
  cv::Mat image8(1, 5, CV_32FC2);

  // Every image should match the first one
  {
    MeanImage mi(false, 0);
    BOOST_REQUIRE_NO_THROW(mi.add(image1));
    BOOST_REQUIRE_NO_THROW(mi.add(image1));                      // same image, no throw
    BOOST_REQUIRE_NO_THROW(mi.add(image1));                      // same image, no throw
    BOOST_REQUIRE_THROW(mi.add(image2), radical::MatException);  // different size/type
    BOOST_REQUIRE_THROW(mi.add(image3), radical::MatException);  // different size/type
    BOOST_REQUIRE_THROW(mi.add(image4), radical::MatException);  // different size/type
    BOOST_REQUIRE_THROW(mi.add(image5), radical::MatException);  // different size/type
    BOOST_REQUIRE_THROW(mi.add(image6), radical::MatException);  // different size/type
    BOOST_REQUIRE_THROW(mi.add(image7), radical::MatException);  // different size/type
    BOOST_REQUIRE_THROW(mi.add(image8), radical::MatException);  // different size/type
  }

  // Reset after every addition → no exceptions
  {
    MeanImage mi(false, 1);
    BOOST_REQUIRE_NO_THROW(mi.add(image1));
    BOOST_REQUIRE_NO_THROW(mi.add(image2));
    BOOST_REQUIRE_NO_THROW(mi.add(image3));
    BOOST_REQUIRE_NO_THROW(mi.add(image4));
  }
}

// Compatibility checks for the size/type of mask
BOOST_AUTO_TEST_CASE(AddMaskCompatibility) {
  cv::Size size(10, 5);

  // Masks (CV_8UC1)
  {
    MeanImage mi1(false, 0);
    BOOST_REQUIRE_NO_THROW(mi1.add(cv::Mat(size, CV_8UC1), cv::Mat(size, CV_8UC1)));  // same size
    MeanImage mi2(false, 0);
    BOOST_REQUIRE_NO_THROW(mi2.add(cv::Mat(size, CV_32FC1), cv::Mat(size, CV_8UC1)));  // same size
    MeanImage mi3(false, 0);
    BOOST_REQUIRE_NO_THROW(mi3.add(cv::Mat(size, CV_32FC2), cv::Mat(size, CV_8UC1)));  // same size
    MeanImage mi4(false, 0);
    BOOST_REQUIRE_NO_THROW(mi4.add(cv::Mat(size, CV_32FC3), cv::Mat(size, CV_8UC1)));  // same size
    MeanImage mi5(false, 0);
    BOOST_REQUIRE_NO_THROW(mi5.add(cv::Mat(size, CV_16UC2), cv::Mat(size, CV_8UC1)));  // same size
    MeanImage mi6(false, 0);
    BOOST_REQUIRE_THROW(mi6.add(cv::Mat(size, CV_8SC1), cv::Mat(1, 1, CV_8UC1)), radical::MatException);  // wrong size
    MeanImage mi7(false, 0);
    BOOST_REQUIRE_THROW(mi7.add(cv::Mat(size, CV_32FC2), cv::Mat(1, 1, CV_8UC1)), radical::MatException);  // wrong size
  }

  // Other types → exception
  {
    MeanImage mi1(false, 0);
    BOOST_REQUIRE_THROW(mi1.add(cv::Mat(size, CV_8UC1), cv::Mat(size, CV_8UC3)),
                        radical::MatException);  // wrong channels
    MeanImage mi2(false, 0);
    BOOST_REQUIRE_THROW(mi2.add(cv::Mat(size, CV_32FC2), cv::Mat(size, CV_8UC2)),
                        radical::MatException);  // wrong channels
    MeanImage mi3(false, 0);
    BOOST_REQUIRE_THROW(mi3.add(cv::Mat(size, CV_8UC4), cv::Mat(size, CV_64FC2)),
                        radical::MatException);  // wrong channels
    MeanImage mi4(false, 0);
    BOOST_REQUIRE_THROW(mi4.add(cv::Mat(size, CV_32FC2), cv::Mat(size, CV_16UC1)),
                        radical::MatException);  // wrong type
    MeanImage mi5(false, 0);
    BOOST_REQUIRE_THROW(mi5.add(cv::Mat(size, CV_32FC1), cv::Mat(size, CV_32FC1)),
                        radical::MatException);  // wrong type
  }
}

// Compatibility checks for the size/type of weights
BOOST_AUTO_TEST_CASE(AddWeightsCompatibility) {
  cv::Size size(10, 5);

  // Weights (Mat CV_64F)
  {
    MeanImage mi1(false, 0);
    BOOST_REQUIRE_NO_THROW(mi1.addWeighted(cv::Mat(size, CV_8UC1), cv::Mat(size, CV_64FC1)));  // same size
    MeanImage mi2(false, 0);
    BOOST_REQUIRE_NO_THROW(mi2.addWeighted(cv::Mat(size, CV_32FC1), cv::Mat(size, CV_64FC1)));  // same size
    MeanImage mi3(false, 0);
    BOOST_REQUIRE_THROW(mi3.addWeighted(cv::Mat(size, CV_32FC2), cv::Mat(size, CV_64FC1)),
                        radical::MatException);  // only for single-channel image
    MeanImage mi4(false, 0);
    BOOST_REQUIRE_THROW(mi4.addWeighted(cv::Mat(size, CV_32FC3), cv::Mat(size, CV_64FC1)),
                        radical::MatException);  // only for single-channel image
    MeanImage mi5(false, 0);
    BOOST_REQUIRE_THROW(mi5.addWeighted(cv::Mat(size, CV_16UC2), cv::Mat(size, CV_64FC1)),
                        radical::MatException);  // only for single-channel image
    MeanImage mi6(false, 0);
    BOOST_REQUIRE_THROW(mi6.addWeighted(cv::Mat(size, CV_8SC1), cv::Mat(1, 2, CV_64FC1)),
                        radical::MatException);  // wrong size
    MeanImage mi7(false, 0);
    BOOST_REQUIRE_THROW(mi7.addWeighted(cv::Mat(size, CV_32FC1), cv::Mat(3, 1, CV_64FC1)),
                        radical::MatException);  // wrong size
  }

  // Weights (Scalar CV_64F)
  {
    MeanImage mi1(false, 0);
    BOOST_REQUIRE_NO_THROW(mi1.addWeighted(cv::Mat(size, CV_8UC1), 1.0));
    MeanImage mi2(false, 0);
    BOOST_REQUIRE_NO_THROW(mi2.addWeighted(cv::Mat(size, CV_32FC1), 3));
    MeanImage mi3(false, 0);
    BOOST_REQUIRE_NO_THROW(mi3.addWeighted(cv::Mat(size, CV_32FC2), 10));
    MeanImage mi4(false, 0);
    BOOST_REQUIRE_NO_THROW(mi4.addWeighted(cv::Mat(size, CV_32FC1), cv::Mat(1, 1, CV_64FC1)));  // effectively scalar
    MeanImage mi5(false, 0);
    BOOST_REQUIRE_NO_THROW(mi5.addWeighted(cv::Mat(size, CV_32FC3), cv::Mat(1, 1, CV_64FC1)));  // effectively scalar
  }

  // Other types → exception
  {
    MeanImage mi1(false, 0);
    BOOST_REQUIRE_THROW(mi1.addWeighted(cv::Mat(size, CV_8UC1), cv::Mat(size, CV_8UC3)),
                        radical::MatException);  // wrong channels
    MeanImage mi2(false, 0);
    BOOST_REQUIRE_THROW(mi2.addWeighted(cv::Mat(size, CV_32FC2), cv::Mat(size, CV_8UC2)),
                        radical::MatException);  // wrong channels
    MeanImage mi3(false, 0);
    BOOST_REQUIRE_THROW(mi3.addWeighted(cv::Mat(size, CV_8UC4), cv::Mat(size, CV_64FC2)),
                        radical::MatException);  // wrong channels
    MeanImage mi4(false, 0);
    BOOST_REQUIRE_THROW(mi4.addWeighted(cv::Mat(size, CV_32FC2), cv::Mat(size, CV_16UC1)),
                        radical::MatException);  // wrong type
    MeanImage mi5(false, 0);
    BOOST_REQUIRE_THROW(mi5.addWeighted(cv::Mat(size, CV_32FC1), cv::Mat(size, CV_32FC1)),
                        radical::MatException);  // wrong type
  }
}

// Return type/size of getMean
BOOST_AUTO_TEST_CASE(GetMeanReturn) {
  MeanImage mi(false, 0);
  auto image = generateRandomImage(10, 5);
  mi.add(image);
  BOOST_CHECK_EQUAL(mi.getMean(true).size(), image.size());
  BOOST_CHECK_EQUAL(mi.getMean(true).type(), image.type());
  BOOST_CHECK_EQUAL(mi.getMean(false).size(), image.size());
  BOOST_CHECK_EQUAL(mi.getMean(false).depth(), CV_64F);
  BOOST_CHECK_EQUAL(mi.getMean(false).channels(), image.channels());
}

// Return type/size of getVariance and getVarianceInverse
BOOST_AUTO_TEST_CASE(GetVarianceReturn) {
  auto image = generateRandomImage(10, 5);

  // Variance computation enabled
  {
    MeanImage mi(true, 0);
    mi.add(image);
    BOOST_CHECK_EQUAL(mi.getVariance().size(), image.size());
    BOOST_CHECK_EQUAL(mi.getVariance().depth(), CV_64F);
    BOOST_CHECK_EQUAL(mi.getVariance().channels(), image.channels());
    BOOST_CHECK_EQUAL(mi.getVarianceInverse().size(), image.size());
    BOOST_CHECK_EQUAL(mi.getVarianceInverse().depth(), CV_64F);
    BOOST_CHECK_EQUAL(mi.getVarianceInverse().channels(), image.channels());
  }

  // Variance computation disabled, should be the same
  {
    MeanImage mi(false, 0);
    mi.add(image);
    BOOST_CHECK_EQUAL(mi.getVariance().size(), image.size());
    BOOST_CHECK_EQUAL(mi.getVariance().depth(), CV_64F);
    BOOST_CHECK_EQUAL(mi.getVariance().channels(), image.channels());
    BOOST_CHECK_EQUAL(mi.getVarianceInverse().size(), image.size());
    BOOST_CHECK_EQUAL(mi.getVarianceInverse().depth(), CV_64F);
    BOOST_CHECK_EQUAL(mi.getVarianceInverse().channels(), image.channels());
  }
}

// Return type/size of getNumSamples
BOOST_AUTO_TEST_CASE(GetNumSamplesReturn) {
  auto image = generateRandomImage(10, 5);

  // Unlimited accumulation
  {
    MeanImage mi(false, 0);
    mi.add(image);
    BOOST_CHECK_EQUAL(mi.getNumSamples(false).size(), image.size());
    BOOST_CHECK_EQUAL(mi.getNumSamples(false).type(), CV_32SC1);
    BOOST_CHECK_EQUAL(mi.getNumSamples(true).size(), image.size());
    BOOST_CHECK_EQUAL(mi.getNumSamples(true).type(), CV_32SC1);
  }

  // With limit
  {
    MeanImage mi(false, 10);
    mi.add(image);
    BOOST_CHECK_EQUAL(mi.getNumSamples(false).size(), image.size());
    BOOST_CHECK_EQUAL(mi.getNumSamples(false).type(), CV_32SC1);
    BOOST_CHECK_EQUAL(mi.getNumSamples(true).size(), image.size());
    BOOST_CHECK_EQUAL(mi.getNumSamples(true).type(), CV_32FC1);
  }
}

// Check the correctness of mean/variance/num samples computation (no mask, no weight)
BOOST_AUTO_TEST_CASE_TEMPLATE(MeanVarianceNumSamplesComputation, T, ImageTypes) {
  using namespace boost::accumulators;
  using Accumulator = accumulator_set<double, stats<tag::mean, tag::variance, tag::count>>;
  Accumulator acc1, acc2;

  setRNGSeed(0);
  auto numbers = generateRandomVector<T>(10, 0, 100);

  const int NUM_SAMPLES = 5;

  MeanImage mi1(false, 0);            // without variance, unlimited
  MeanImage mi2(true, 0);             // with variance, unlimited
  MeanImage mi3(false, NUM_SAMPLES);  // without variance, limited
  MeanImage mi4(true, NUM_SAMPLES);   // with variance, limited
  cv::Mat_<T> image(1, 1);

  for (size_t i = 0; i < numbers.size(); ++i) {
    const auto& n = numbers[i];
    int j = i % NUM_SAMPLES;

    if (j == 0)
      acc2 = Accumulator();

    acc1(n);
    acc2(n);
    image(0, 0) = n;

    mi1.add(image);
    BOOST_REQUIRE_CLOSE(mi1.getMean(true).template at<T>(0, 0), static_cast<double>(cv::saturate_cast<T>(mean(acc1))),
                        TOLERANCE_DOUBLE);
    BOOST_REQUIRE_CLOSE(mi1.getMean(false).template at<double>(0, 0), mean(acc1), TOLERANCE_DOUBLE);
    BOOST_REQUIRE_CLOSE(mi1.getVariance().template at<double>(0, 0), 0, TOLERANCE_DOUBLE);
    BOOST_REQUIRE_CLOSE(mi1.getVarianceInverse().template at<double>(0, 0), 0, TOLERANCE_DOUBLE);
    BOOST_REQUIRE_EQUAL(mi1.getNumSamples(false).template at<int>(0, 0), count(acc1));
    BOOST_REQUIRE_EQUAL(mi1.getNumSamples(true).template at<int>(0, 0), count(acc1));

    mi2.add(image);
    BOOST_REQUIRE_CLOSE(mi2.getMean(true).template at<T>(0, 0), static_cast<double>(cv::saturate_cast<T>(mean(acc1))),
                        TOLERANCE_DOUBLE);
    BOOST_REQUIRE_CLOSE(mi2.getMean(false).template at<double>(0, 0), mean(acc1), TOLERANCE_DOUBLE);
    BOOST_REQUIRE_CLOSE(mi2.getVariance().template at<double>(0, 0), variance(acc1), TOLERANCE_DOUBLE);
    BOOST_REQUIRE_CLOSE(mi2.getVarianceInverse().template at<double>(0, 0), div0(1.0, variance(acc1)),
                        TOLERANCE_DOUBLE);
    BOOST_REQUIRE_EQUAL(mi2.getNumSamples(false).template at<int>(0, 0), count(acc1));
    BOOST_REQUIRE_EQUAL(mi2.getNumSamples(true).template at<int>(0, 0), count(acc1));

    mi3.add(image);
    BOOST_REQUIRE_CLOSE(mi3.getMean(true).template at<T>(0, 0), static_cast<double>(cv::saturate_cast<T>(mean(acc2))),
                        TOLERANCE_DOUBLE);
    BOOST_REQUIRE_CLOSE(mi3.getMean(false).template at<double>(0, 0), mean(acc2), TOLERANCE_DOUBLE);
    BOOST_REQUIRE_CLOSE(mi3.getVariance().template at<double>(0, 0), 0, TOLERANCE_DOUBLE);
    BOOST_REQUIRE_CLOSE(mi3.getVarianceInverse().template at<double>(0, 0), 0, TOLERANCE_DOUBLE);
    BOOST_REQUIRE_EQUAL(mi3.getNumSamples(false).template at<int>(0, 0), j + 1);
    BOOST_REQUIRE_CLOSE(mi3.getNumSamples(true).template at<float>(0, 0), (1.0f + j) / NUM_SAMPLES, TOLERANCE_FLOAT);

    mi4.add(image);
    BOOST_REQUIRE_CLOSE(mi4.getMean(true).template at<T>(0, 0), static_cast<double>(cv::saturate_cast<T>(mean(acc2))),
                        TOLERANCE_DOUBLE);
    BOOST_REQUIRE_CLOSE(mi4.getMean(false).template at<double>(0, 0), mean(acc2), TOLERANCE_DOUBLE);
    BOOST_REQUIRE_CLOSE(mi4.getVariance().template at<double>(0, 0), variance(acc2), TOLERANCE_DOUBLE);
    BOOST_REQUIRE_CLOSE(mi4.getVarianceInverse().template at<double>(0, 0), div0(1.0, variance(acc2)),
                        TOLERANCE_DOUBLE);
    BOOST_REQUIRE_EQUAL(mi4.getNumSamples(false).template at<int>(0, 0), j + 1);
    BOOST_REQUIRE_CLOSE(mi4.getNumSamples(true).template at<float>(0, 0), (1.0f + j) / NUM_SAMPLES, TOLERANCE_FLOAT);
  }
}

// Check the correctness of mean/variance/num samples computation (masking, no weight)
BOOST_AUTO_TEST_CASE_TEMPLATE(MeanVarianceNumSamplesComputationWithMasking, T, ImageTypes) {
  using namespace boost::accumulators;
  using Accumulator = accumulator_set<double, stats<tag::mean, tag::variance, tag::count>>;
  Accumulator acc1, acc2;

  setRNGSeed(1);
  auto numbers = generateRandomVector<T>(20, 0, 100);
  auto masks = generateRandomVector<uint8_t>(20, 0, 1);
  masks[0] = 1;  // make sure first number is averaged, otherwise boost accumulator gives NaN

  const int NUM_SAMPLES = 5;

  MeanImage mi1(false, 0);            // without variance, unlimited
  MeanImage mi2(true, 0);             // with variance, unlimited
  MeanImage mi3(false, NUM_SAMPLES);  // without variance, limited
  MeanImage mi4(true, NUM_SAMPLES);   // with variance, limited

  cv::Mat_<T> image(1, 1);
  cv::Mat_<uint8_t> mask(1, 1);

  for (size_t i = 0; i < numbers.size(); ++i) {
    const auto& n = numbers[i];
    const auto& m = masks[i];

    if (count(acc2) == NUM_SAMPLES) {
      acc2 = Accumulator();
    }

    if (m) {
      acc1(n);
      acc2(n);
    }

    image(0, 0) = n;
    mask(0, 0) = m;

    mi1.add(image, mask);
    BOOST_REQUIRE_CLOSE(mi1.getMean(true).template at<T>(0, 0), static_cast<double>(cv::saturate_cast<T>(mean(acc1))),
                        TOLERANCE_DOUBLE);
    BOOST_REQUIRE_CLOSE(mi1.getMean(false).template at<double>(0, 0), mean(acc1), TOLERANCE_DOUBLE);
    BOOST_REQUIRE_CLOSE(mi1.getVariance().template at<double>(0, 0), 0, TOLERANCE_DOUBLE);
    BOOST_REQUIRE_CLOSE(mi1.getVarianceInverse().template at<double>(0, 0), 0, TOLERANCE_DOUBLE);
    BOOST_REQUIRE_EQUAL(mi1.getNumSamples(false).template at<int>(0, 0), count(acc1));
    BOOST_REQUIRE_EQUAL(mi1.getNumSamples(true).template at<int>(0, 0), count(acc1));

    mi2.add(image, mask);
    BOOST_REQUIRE_CLOSE(mi2.getMean(true).template at<T>(0, 0), static_cast<double>(cv::saturate_cast<T>(mean(acc1))),
                        TOLERANCE_DOUBLE);
    BOOST_REQUIRE_CLOSE(mi2.getMean(false).template at<double>(0, 0), mean(acc1), TOLERANCE_DOUBLE);
    BOOST_REQUIRE_CLOSE(mi2.getVariance().template at<double>(0, 0), variance(acc1), TOLERANCE_DOUBLE);
    BOOST_REQUIRE_CLOSE(mi2.getVarianceInverse().template at<double>(0, 0), div0(1.0, variance(acc1)),
                        TOLERANCE_DOUBLE);
    BOOST_REQUIRE_EQUAL(mi2.getNumSamples(false).template at<int>(0, 0), count(acc1));
    BOOST_REQUIRE_EQUAL(mi2.getNumSamples(true).template at<int>(0, 0), count(acc1));

    mi3.add(image, mask);
    BOOST_REQUIRE_CLOSE(mi3.getMean(true).template at<T>(0, 0), static_cast<double>(cv::saturate_cast<T>(mean(acc2))),
                        TOLERANCE_DOUBLE);
    BOOST_REQUIRE_CLOSE(mi3.getMean(false).template at<double>(0, 0), mean(acc2), TOLERANCE_DOUBLE);
    BOOST_REQUIRE_CLOSE(mi3.getVariance().template at<double>(0, 0), 0, TOLERANCE_DOUBLE);
    BOOST_REQUIRE_CLOSE(mi3.getVarianceInverse().template at<double>(0, 0), 0, TOLERANCE_DOUBLE);
    BOOST_REQUIRE_EQUAL(mi3.getNumSamples(false).template at<int>(0, 0), count(acc2));
    BOOST_REQUIRE_CLOSE(mi3.getNumSamples(true).template at<float>(0, 0), static_cast<float>(count(acc2)) / NUM_SAMPLES,
                        TOLERANCE_FLOAT);

    mi4.add(image, mask);
    BOOST_REQUIRE_CLOSE(mi4.getMean(true).template at<T>(0, 0), static_cast<double>(cv::saturate_cast<T>(mean(acc2))),
                        TOLERANCE_DOUBLE);
    BOOST_REQUIRE_CLOSE(mi4.getMean(false).template at<double>(0, 0), mean(acc2), TOLERANCE_DOUBLE);
    BOOST_REQUIRE_CLOSE(mi4.getVariance().template at<double>(0, 0), variance(acc2), TOLERANCE_DOUBLE);
    BOOST_REQUIRE_CLOSE(mi4.getVarianceInverse().template at<double>(0, 0), div0(1.0, variance(acc2)),
                        TOLERANCE_DOUBLE);
    BOOST_REQUIRE_EQUAL(mi4.getNumSamples(false).template at<int>(0, 0), count(acc2));
    BOOST_REQUIRE_CLOSE(mi4.getNumSamples(true).template at<float>(0, 0), static_cast<float>(count(acc2)) / NUM_SAMPLES,
                        TOLERANCE_FLOAT);
  }
}

// Check the correctness of mean/variance/num samples computation (no masking, weights)
BOOST_AUTO_TEST_CASE_TEMPLATE(MeanVarianceNumSamplesComputationWithWeights, T, ImageTypes) {
  using namespace boost::accumulators;
  using Accumulator = accumulator_set<double, stats<tag::weighted_mean, tag::weighted_variance>, double>;
  Accumulator acc1, acc2;

  setRNGSeed(2);
  auto numbers = generateRandomVector<T>(20, 0, 100);
  auto weights = generateRandomVector<double>(20, 0, 5);

  const int NUM_SAMPLES = 5;

  MeanImage mi1(false, 0);            // without variance, unlimited
  MeanImage mi2(true, 0);             // with variance, unlimited
  MeanImage mi3(false, NUM_SAMPLES);  // without variance, limited
  MeanImage mi4(true, NUM_SAMPLES);   // with variance, limited
  MeanImage mi5(false, 0);            // without variance, unlimited, for scalar weights
  MeanImage mi6(true, 0);             // with variance, unlimited, for scalar weights
  weights[3] = 0;                     // Force several weights to zero
  weights[15] = 0;                    // (so that corresponding pixels are excluded)

  cv::Mat_<T> image(1, 1);
  cv::Mat_<double> w(1, 1);

  int count = 0;
  for (size_t i = 0; i < numbers.size(); ++i) {
    const auto& n = numbers[i];

    if (count == NUM_SAMPLES) {
      count = 0;
      acc2 = Accumulator();
    }

    if (weights[i] > 0)
      ++count;

    acc1(n, weight = weights[i]);
    acc2(n, weight = weights[i]);

    image(0, 0) = numbers[i];
    w(0, 0) = weights[i];

    mi1.addWeighted(image, w);
    BOOST_REQUIRE_CLOSE(mi1.getMean(true).template at<T>(0, 0),
                        static_cast<double>(cv::saturate_cast<T>(weighted_mean(acc1))), TOLERANCE_DOUBLE);
    BOOST_REQUIRE_CLOSE(mi1.getMean(false).template at<double>(0, 0), weighted_mean(acc1), TOLERANCE_DOUBLE);
    BOOST_REQUIRE_CLOSE(mi1.getVariance().template at<double>(0, 0), 0, TOLERANCE_DOUBLE);
    BOOST_REQUIRE_CLOSE(mi1.getVarianceInverse().template at<double>(0, 0), 0, TOLERANCE_DOUBLE);

    mi2.addWeighted(image, w);
    BOOST_REQUIRE_CLOSE(mi2.getMean(true).template at<T>(0, 0),
                        static_cast<double>(cv::saturate_cast<T>(weighted_mean(acc1))), TOLERANCE_DOUBLE);
    BOOST_REQUIRE_CLOSE(mi2.getMean(false).template at<double>(0, 0), weighted_mean(acc1), TOLERANCE_DOUBLE);
    BOOST_REQUIRE_CLOSE(mi2.getVariance().template at<double>(0, 0), weighted_variance(acc1), TOLERANCE_DOUBLE);
    BOOST_REQUIRE_CLOSE(mi2.getVarianceInverse().template at<double>(0, 0), div0(1.0, weighted_variance(acc1)),
                        TOLERANCE_DOUBLE);

    mi3.addWeighted(image, w);
    BOOST_REQUIRE_CLOSE(mi3.getMean(true).template at<T>(0, 0),
                        static_cast<double>(cv::saturate_cast<T>(weighted_mean(acc2))), TOLERANCE_DOUBLE);
    BOOST_REQUIRE_CLOSE(mi3.getMean(false).template at<double>(0, 0), weighted_mean(acc2), TOLERANCE_DOUBLE);
    BOOST_REQUIRE_CLOSE(mi3.getVariance().template at<double>(0, 0), 0, TOLERANCE_DOUBLE);
    BOOST_REQUIRE_CLOSE(mi3.getVarianceInverse().template at<double>(0, 0), 0, TOLERANCE_DOUBLE);
    BOOST_REQUIRE_EQUAL(mi3.getNumSamples(false).template at<int>(0, 0), count);
    BOOST_REQUIRE_CLOSE(mi3.getNumSamples(true).template at<float>(0, 0), static_cast<float>(count) / NUM_SAMPLES,
                        TOLERANCE_FLOAT);

    mi4.addWeighted(image, w);
    BOOST_REQUIRE_CLOSE(mi4.getMean(true).template at<T>(0, 0),
                        static_cast<double>(cv::saturate_cast<T>(weighted_mean(acc2))), TOLERANCE_DOUBLE);
    BOOST_REQUIRE_CLOSE(mi4.getMean(false).template at<double>(0, 0), weighted_mean(acc2), TOLERANCE_DOUBLE);
    BOOST_REQUIRE_CLOSE(mi4.getVariance().template at<double>(0, 0), weighted_variance(acc2), TOLERANCE_DOUBLE);
    BOOST_REQUIRE_CLOSE(mi4.getVarianceInverse().template at<double>(0, 0), div0(1.0, weighted_variance(acc2)),
                        TOLERANCE_DOUBLE);
    BOOST_REQUIRE_EQUAL(mi4.getNumSamples(false).template at<int>(0, 0), count);
    BOOST_REQUIRE_CLOSE(mi4.getNumSamples(true).template at<float>(0, 0), static_cast<float>(count) / NUM_SAMPLES,
                        TOLERANCE_FLOAT);

    mi5.addWeighted(image, weights[i]);
    BOOST_REQUIRE_CLOSE(mi5.getMean(true).template at<T>(0, 0),
                        static_cast<double>(cv::saturate_cast<T>(weighted_mean(acc1))), TOLERANCE_DOUBLE);
    BOOST_REQUIRE_CLOSE(mi5.getMean(false).template at<double>(0, 0), weighted_mean(acc1), TOLERANCE_DOUBLE);
    BOOST_REQUIRE_CLOSE(mi5.getVariance().template at<double>(0, 0), 0, TOLERANCE_DOUBLE);
    BOOST_REQUIRE_CLOSE(mi5.getVarianceInverse().template at<double>(0, 0), 0, TOLERANCE_DOUBLE);

    mi6.addWeighted(image, weights[i]);
    BOOST_REQUIRE_CLOSE(mi6.getMean(true).template at<T>(0, 0),
                        static_cast<double>(cv::saturate_cast<T>(weighted_mean(acc1))), TOLERANCE_DOUBLE);
    BOOST_REQUIRE_CLOSE(mi6.getMean(false).template at<double>(0, 0), weighted_mean(acc1), TOLERANCE_DOUBLE);
    BOOST_REQUIRE_CLOSE(mi6.getVariance().template at<double>(0, 0), weighted_variance(acc1), TOLERANCE_DOUBLE);
    BOOST_REQUIRE_CLOSE(mi6.getVarianceInverse().template at<double>(0, 0), div0(1.0, weighted_variance(acc1)),
                        TOLERANCE_DOUBLE);
  }
}

// Check the correctness of mean/variance/num samples computation (masking, weights)
BOOST_AUTO_TEST_CASE_TEMPLATE(MeanVarianceNumSamplesComputationWithMasksWeights, T, ImageTypes) {
  using namespace boost::accumulators;
  using Accumulator = accumulator_set<double, stats<tag::weighted_mean, tag::weighted_variance>, double>;
  Accumulator acc1, acc2;

  setRNGSeed(2);
  auto numbers = generateRandomVector<T>(20, 0, 100);
  auto weights = generateRandomVector<double>(20, 0, 5);
  auto masks = generateRandomVector<uint8_t>(20, 0, 1);
  masks[0] = 1;  // make sure first number is averaged, otherwise boost accumulator gives NaN

  const int NUM_SAMPLES = 5;

  MeanImage mi1(false, 0);            // without variance, unlimited
  MeanImage mi2(true, 0);             // with variance, unlimited
  MeanImage mi3(false, NUM_SAMPLES);  // without variance, limited
  MeanImage mi4(true, NUM_SAMPLES);   // with variance, limited
  MeanImage mi5(false, 0);            // without variance, unlimited, for scalar weights
  MeanImage mi6(true, 0);             // with variance, unlimited, for scalar weights
  weights[3] = 0;                     // Force several weights to zero
  weights[15] = 0;                    // (so that corresponding pixels are excluded)

  cv::Mat_<T> image(1, 1);
  cv::Mat_<double> w(1, 1);
  cv::Mat_<uint8_t> mask(1, 1);

  int count = 0;
  for (size_t i = 0; i < numbers.size(); ++i) {
    const auto& n = numbers[i];
    const auto& m = masks[i];

    if (count == NUM_SAMPLES) {
      count = 0;
      acc2 = Accumulator();
    }

    if (weights[i] > 0 && m)
      ++count;

    if (m) {
      acc1(n, weight = weights[i]);
      acc2(n, weight = weights[i]);
    }

    image(0, 0) = n;
    w(0, 0) = weights[i];
    mask(0, 0) = m;

    mi1.addWeighted(image, w, mask);
    BOOST_REQUIRE_CLOSE(mi1.getMean(true).template at<T>(0, 0),
                        static_cast<double>(cv::saturate_cast<T>(weighted_mean(acc1))), TOLERANCE_DOUBLE);
    BOOST_REQUIRE_CLOSE(mi1.getMean(false).template at<double>(0, 0), weighted_mean(acc1), TOLERANCE_DOUBLE);
    BOOST_REQUIRE_CLOSE(mi1.getVariance().template at<double>(0, 0), 0, TOLERANCE_DOUBLE);
    BOOST_REQUIRE_CLOSE(mi1.getVarianceInverse().template at<double>(0, 0), 0, TOLERANCE_DOUBLE);

    mi2.addWeighted(image, w, mask);
    BOOST_REQUIRE_CLOSE(mi2.getMean(true).template at<T>(0, 0),
                        static_cast<double>(cv::saturate_cast<T>(weighted_mean(acc1))), TOLERANCE_DOUBLE);
    BOOST_REQUIRE_CLOSE(mi2.getMean(false).template at<double>(0, 0), weighted_mean(acc1), TOLERANCE_DOUBLE);
    BOOST_REQUIRE_CLOSE(mi2.getVariance().template at<double>(0, 0), weighted_variance(acc1), TOLERANCE_DOUBLE);
    BOOST_REQUIRE_CLOSE(mi2.getVarianceInverse().template at<double>(0, 0), div0(1.0, weighted_variance(acc1)),
                        TOLERANCE_DOUBLE);

    mi3.addWeighted(image, w, mask);
    BOOST_REQUIRE_CLOSE(mi3.getMean(true).template at<T>(0, 0),
                        static_cast<double>(cv::saturate_cast<T>(weighted_mean(acc2))), TOLERANCE_DOUBLE);
    BOOST_REQUIRE_CLOSE(mi3.getMean(false).template at<double>(0, 0), weighted_mean(acc2), TOLERANCE_DOUBLE);
    BOOST_REQUIRE_CLOSE(mi3.getVariance().template at<double>(0, 0), 0, TOLERANCE_DOUBLE);
    BOOST_REQUIRE_CLOSE(mi3.getVarianceInverse().template at<double>(0, 0), 0, TOLERANCE_DOUBLE);
    BOOST_REQUIRE_EQUAL(mi3.getNumSamples(false).template at<int>(0, 0), count);
    BOOST_REQUIRE_CLOSE(mi3.getNumSamples(true).template at<float>(0, 0), static_cast<float>(count) / NUM_SAMPLES,
                        TOLERANCE_FLOAT);

    mi4.addWeighted(image, w, mask);
    BOOST_REQUIRE_CLOSE(mi4.getMean(true).template at<T>(0, 0),
                        static_cast<double>(cv::saturate_cast<T>(weighted_mean(acc2))), TOLERANCE_DOUBLE);
    BOOST_REQUIRE_CLOSE(mi4.getMean(false).template at<double>(0, 0), weighted_mean(acc2), TOLERANCE_DOUBLE);
    BOOST_REQUIRE_CLOSE(mi4.getVariance().template at<double>(0, 0), weighted_variance(acc2), TOLERANCE_DOUBLE);
    BOOST_REQUIRE_CLOSE(mi4.getVarianceInverse().template at<double>(0, 0), div0(1.0, weighted_variance(acc2)),
                        TOLERANCE_DOUBLE);
    BOOST_REQUIRE_EQUAL(mi4.getNumSamples(false).template at<int>(0, 0), count);
    BOOST_REQUIRE_CLOSE(mi4.getNumSamples(true).template at<float>(0, 0), static_cast<float>(count) / NUM_SAMPLES,
                        TOLERANCE_FLOAT);

    mi5.addWeighted(image, weights[i], mask);
    BOOST_REQUIRE_CLOSE(mi5.getMean(true).template at<T>(0, 0),
                        static_cast<double>(cv::saturate_cast<T>(weighted_mean(acc1))), TOLERANCE_DOUBLE);
    BOOST_REQUIRE_CLOSE(mi5.getMean(false).template at<double>(0, 0), weighted_mean(acc1), TOLERANCE_DOUBLE);
    BOOST_REQUIRE_CLOSE(mi5.getVariance().template at<double>(0, 0), 0, TOLERANCE_DOUBLE);
    BOOST_REQUIRE_CLOSE(mi5.getVarianceInverse().template at<double>(0, 0), 0, TOLERANCE_DOUBLE);

    mi6.addWeighted(image, w, mask);
    BOOST_REQUIRE_CLOSE(mi6.getMean(true).template at<T>(0, 0),
                        static_cast<double>(cv::saturate_cast<T>(weighted_mean(acc1))), TOLERANCE_DOUBLE);
    BOOST_REQUIRE_CLOSE(mi6.getMean(false).template at<double>(0, 0), weighted_mean(acc1), TOLERANCE_DOUBLE);
    BOOST_REQUIRE_CLOSE(mi6.getVariance().template at<double>(0, 0), weighted_variance(acc1), TOLERANCE_DOUBLE);
    BOOST_REQUIRE_CLOSE(mi6.getVarianceInverse().template at<double>(0, 0), div0(1.0, weighted_variance(acc1)),
                        TOLERANCE_DOUBLE);
  }
}

// After accumulating a single sample, variance should be exactly zero, and mean should be exactly the input
BOOST_AUTO_TEST_CASE(MeanVarianceAfterSingleSample) {
  cv::Mat image(cv::Mat_<double>(1, 1) << 0.000883048);

  cv::Mat weight_zero(cv::Mat_<double>(1, 1) << 0.0);
  cv::Mat weight_nonzero(cv::Mat_<double>(1, 1) << 6.80625e+06);

  cv::Mat mask_zero(cv::Mat_<uint8_t>(1, 1) << 0);
  cv::Mat mask_nonzero(cv::Mat_<uint8_t>(1, 1) << 255);

  // No weight, no mask
  {
    MeanImage mi(true, 0);
    mi.add(image);
    BOOST_CHECK_EQUAL(mi.getMean().at<double>(0, 0), image.at<double>(0, 0));
    BOOST_CHECK_EQUAL(mi.getVariance().at<double>(0, 0), 0.0);
    BOOST_REQUIRE_EQUAL(mi.getVarianceInverse().at<double>(0, 0), 0.0);
  }

  // Mask
  {
    MeanImage mi(true, 0);
    mi.add(image, mask_nonzero);
    BOOST_CHECK_EQUAL(mi.getMean().at<double>(0, 0), image.at<double>(0, 0));
    BOOST_CHECK_EQUAL(mi.getVariance().at<double>(0, 0), 0.0);
    for (size_t i = 0; i < 100; ++i) {
      mi.add(image, mask_zero);
      BOOST_REQUIRE_EQUAL(mi.getMean().at<double>(0, 0), image.at<double>(0, 0));
      BOOST_REQUIRE_EQUAL(mi.getVariance().at<double>(0, 0), 0.0);
      BOOST_REQUIRE_EQUAL(mi.getVarianceInverse().at<double>(0, 0), 0.0);
    }
  }

  // Weight
  {
    MeanImage mi(true, 0);
    mi.addWeighted(image, weight_nonzero);
    BOOST_CHECK_EQUAL(mi.getMean().at<double>(0, 0), image.at<double>(0, 0));
    BOOST_CHECK_EQUAL(mi.getVariance().at<double>(0, 0), 0.0);
    for (size_t i = 0; i < 100; ++i) {
      mi.addWeighted(image, weight_zero);
      BOOST_REQUIRE_EQUAL(mi.getMean().at<double>(0, 0), image.at<double>(0, 0));
      BOOST_REQUIRE_EQUAL(mi.getVariance().at<double>(0, 0), 0.0);
      BOOST_REQUIRE_EQUAL(mi.getVarianceInverse().at<double>(0, 0), 0.0);
    }
  }

  // Weight scalar
  {
    MeanImage mi(true, 0);
    mi.addWeighted(image, 6.80625e+06);
    BOOST_CHECK_EQUAL(mi.getMean().at<double>(0, 0), image.at<double>(0, 0));
    BOOST_CHECK_EQUAL(mi.getVariance().at<double>(0, 0), 0.0);
    for (size_t i = 0; i < 100; ++i) {
      mi.addWeighted(image, 0.0);
      BOOST_REQUIRE_EQUAL(mi.getMean().at<double>(0, 0), image.at<double>(0, 0));
      BOOST_REQUIRE_EQUAL(mi.getVariance().at<double>(0, 0), 0.0);
      BOOST_REQUIRE_EQUAL(mi.getVarianceInverse().at<double>(0, 0), 0.0);
    }
  }

  // Weight implicit / weight explicit
  {
    MeanImage mi(true, 0);
    mi.add(image);
    BOOST_CHECK_EQUAL(mi.getMean().at<double>(0, 0), image.at<double>(0, 0));
    BOOST_CHECK_EQUAL(mi.getVariance().at<double>(0, 0), 0.0);
    for (size_t i = 0; i < 100; ++i) {
      mi.addWeighted(image, weight_zero);
      BOOST_REQUIRE_EQUAL(mi.getMean().at<double>(0, 0), image.at<double>(0, 0));
      BOOST_REQUIRE_EQUAL(mi.getVariance().at<double>(0, 0), 0.0);
      BOOST_REQUIRE_EQUAL(mi.getVarianceInverse().at<double>(0, 0), 0.0);
    }
  }

  // Weight implicit / mask explicit
  {
    MeanImage mi(true, 0);
    mi.add(image);
    BOOST_CHECK_EQUAL(mi.getMean().at<double>(0, 0), image.at<double>(0, 0));
    BOOST_CHECK_EQUAL(mi.getVariance().at<double>(0, 0), 0.0);
    for (size_t i = 0; i < 100; ++i) {
      mi.add(image, mask_zero);
      BOOST_REQUIRE_EQUAL(mi.getMean().at<double>(0, 0), image.at<double>(0, 0));
      BOOST_REQUIRE_EQUAL(mi.getVariance().at<double>(0, 0), 0.0);
      BOOST_REQUIRE_EQUAL(mi.getVarianceInverse().at<double>(0, 0), 0.0);
    }
  }
}

// Check the correctness of mean/variance computation when all samples are the same
BOOST_AUTO_TEST_CASE_TEMPLATE(MeanVarianceSameSamples, T, ImageTypes) {
  const int NUM_SAMPLES = 5;

  MeanImage mi1(true, 0);
  MeanImage mi2(true, NUM_SAMPLES);

  // Four elements to make sure we experence "strange" behavior in OpenCV
  // See: https://github.com/opencv/opencv/issues/8413
  cv::Mat_<T> image(1, 4);
  cv::randu(image, cv::Scalar(0, 0, 0), cv::Scalar(1, 1, 1));

  cv::Mat image_double;
  image.convertTo(image_double, CV_64F);

  for (size_t i = 0; i < 1000; ++i) {
    mi1.add(image);
    BOOST_REQUIRE_EQUAL_MAT(mi1.getMean(true), image, T);
    BOOST_REQUIRE_EQUAL_MAT(mi1.getMean(false), image_double, double);
    BOOST_REQUIRE_EQUAL_MAT(mi1.getVariance(), 0, double);
    BOOST_REQUIRE_EQUAL_MAT(mi1.getVarianceInverse(), 0, double);

    mi2.addWeighted(image, 0.5);
    BOOST_REQUIRE_EQUAL_MAT(mi2.getMean(true), image, T);
    BOOST_REQUIRE_EQUAL_MAT(mi2.getMean(false), image_double, double);
    BOOST_REQUIRE_EQUAL_MAT(mi2.getVariance(), 0, double);
    BOOST_REQUIRE_EQUAL_MAT(mi2.getVarianceInverse(), 0, double);
  }
}

BOOST_AUTO_TEST_CASE(MeanWithSingleImage) {
  auto image = generateRandomImage(10, 10);
  MeanImage mi(false, 0);
  mi.add(image);
  BOOST_CHECK_EQUAL_MAT(mi.getMean(), image, cv::Vec3b);
}
