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

#include "utils/key_code.h"

namespace utils {

KeyCode::KeyCode(int code) : code_lsb_(code % 256) {}

bool KeyCode::operator==(Key key) const {
  switch (key) {
    case Key::ENTER:
      return code_lsb_ == 13       // Enter, Numpad (Ubuntu + OpenCV 3)
             || code_lsb_ == 10    // Enter (Ubuntu + OpenCV 2)
             || code_lsb_ == 141;  // Numpad (Ubuntu + OpenCV 2)
    case Key::ESC:
      return code_lsb_ == 27;
    case Key::ARROW_UP:
      return code_lsb_ == 82;
    case Key::ARROW_DOWN:
      return code_lsb_ == 84;
    case Key::ARROW_RIGHT:
      return code_lsb_ == 83;
    case Key::ARROW_LEFT:
      return code_lsb_ == 81;
    case Key::PLUS:
      return code_lsb_ == 171     // Numpad (Ubuntu + OpenCV 2)
             || code_lsb_ == 43;  // Numpad (Ubuntu + OpenCV 3)
    case Key::MINUS:
      return code_lsb_ == 173     // Numpad (Ubuntu + OpenCV 2)
             || code_lsb_ == 45;  // Numpad (Ubuntu + OpenCV 3)
    case Key::NO_KEY:
      return code_lsb_ == 255;
  }
  return false;
}

bool KeyCode::operator!=(Key key) const {
  return !this->operator==(key);
}
}
