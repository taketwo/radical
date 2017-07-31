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

#include <cassert>
#include <iostream>

#include <radical/exceptions.h>
#include <radical/check.h>
#include <radical/mat_io.h>

namespace radical {

static const uint32_t MAGIC = 0xC4A1FDD9;

void writeMat(const std::string& filename, const cv::Mat& mat) {
  std::ofstream file(filename, std::ios::out | std::ios::binary);
  if (!file.is_open())
    throw SerializationException("Failed to open file for writing cv::Mat", filename);

  writeMat(file, mat);
}

void writeMat(std::ofstream& file, const cv::Mat& mat) {
  Check("Serialized mat", mat).notEmpty().isContinuous().hasMaxDimensions(2);

  uint32_t type = mat.type();
  file.write((const char*)(&MAGIC), sizeof(uint32_t));
  file.write((const char*)(&type), sizeof(uint32_t));
  file.write((const char*)(&mat.dims), sizeof(uint32_t));
  file.write((const char*)(&mat.rows), sizeof(uint32_t));
  file.write((const char*)(&mat.cols), sizeof(uint32_t));
  file.write((const char*)(mat.data), mat.elemSize() * mat.total());
}

cv::Mat readMat(const std::string& filename) {
  std::ifstream file(filename, std::ios::in | std::ios::binary);
  if (!file.is_open())
    throw SerializationException("Failed to open file for reading cv::Mat", filename);
  return readMat(file);
}

cv::Mat readMat(std::ifstream& file) {
  uint32_t magic, type, dims, rows, cols;
  file.read((char*)(&magic), sizeof(uint32_t));
  if (magic != MAGIC)
    throw SerializationException("File does not contain a cv::Mat");
  file.read((char*)(&type), sizeof(uint32_t));
  file.read((char*)(&dims), sizeof(uint32_t));
  if (dims > 2)
    throw SerializationException("File contains a cv::Mat that is not 1- or 2-dimensional");
  file.read((char*)(&rows), sizeof(uint32_t));
  file.read((char*)(&cols), sizeof(uint32_t));
  cv::Mat mat(rows, cols, type);
  assert(mat.isContinuous());
  file.read((char*)(mat.data), mat.elemSize() * mat.total());
  file.close();
  return mat;
}

}  // namespace radical
