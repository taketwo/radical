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

#include <radical/exceptions.h>
#include <radical/nonparametric_vignetting_model.h>
#include <radical/polynomial_vignetting_model.h>
#include <radical/vignetting_model.h>

namespace radical {

#define TRY_LOAD(Model)                    \
  if (!model)                              \
    try {                                  \
      model.reset(new Model(filename));    \
    } catch (SerializationException&) {    \
    }

VignettingModel::Ptr VignettingModel::load(const std::string& filename) {
  VignettingModel::Ptr model = nullptr;

  TRY_LOAD(NonparametricVignettingModel);
  TRY_LOAD(PolynomialVignettingModel<3>);

  return model;
}

}  // namespace radical
