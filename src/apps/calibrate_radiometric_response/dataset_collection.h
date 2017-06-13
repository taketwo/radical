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

#include "grabbers/grabber.h"

#include "utils/mean_image.h"

#include "dataset.h"

class DatasetCollection {
 public:
  struct Parameters {
    int exposure_min;
    int exposure_max;
    float exposure_factor;
    unsigned int exposure_control_lag = 10;
    unsigned int num_average_frames = 25;
    unsigned int num_images = 1;
    unsigned int valid_intensity_min = 1;
    unsigned int valid_intensity_max = 254;
    unsigned int bloom_radius = 25;
  };

  DatasetCollection(grabbers::Grabber::Ptr grabber, const Parameters& params);

  bool addFrame(const cv::Mat& frame);

  Dataset::Ptr getDataset() const;

 private:
  cv::Mat computeSaturationMask(const cv::Mat& image);

  grabbers::Grabber::Ptr grabber_;
  Parameters params_;

  Dataset::Ptr dataset_;
  utils::MeanImage mean_;
  utils::MeanImage mean_mask_;
  int exposure_;
  int skip_frames_;
  int images_to_accumulate_;
  const cv::Mat morph_;
};
