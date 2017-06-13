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

#include <opencv2/imgproc/imgproc.hpp>

#include "blob_tracker.h"

BlobTracker::BlobTracker() {
  dilate_element_ = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5));
  erode_element_ = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(18, 18));
}

void BlobTracker::operator()(cv::InputArray _image, cv::OutputArray _mask) {
  cv::Mat image = _image.getMat();
  if (!tracking_) {
    position_ = cv::Point(image.cols / 2, image.rows / 2);
    tracking_ = true;
  }
  cv::Mat mask(image.rows + 2, image.cols + 2, CV_8UC1);
  mask.setTo(0);
  cv::Rect box;
  cv::floodFill(image, mask, position_, cv::Scalar(255, 40, 20), &box, cv::Scalar(5, 5, 5), cv::Scalar(5, 5, 5),
                8 | (255 << 8) | cv::FLOODFILL_MASK_ONLY);
  cv::dilate(mask, mask, dilate_element_);
  cv::erode(mask, mask, erode_element_);
  position_ = cv::Point(box.x + box.width / 2, box.y + box.height / 2);
  mask(cv::Rect(1, 1, image.cols, image.rows)).copyTo(_mask);
}
