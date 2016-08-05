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

#include <iostream>

#include <boost/program_options.hpp>

class OptionsBase {
 public:
  virtual ~OptionsBase() {}

  bool parse(int argc, const char** argv) {
    namespace po = boost::program_options;
    po::options_description general("Options");
    po::options_description hidden;
    po::positional_options_description positional;
    general.add_options()("help,h", "Print this help message");
    addOptions(general);
    addPositional(hidden, positional);
    po::options_description all;
    all.add(general).add(hidden);
    try {
      auto parser = po::command_line_parser(argc, argv).options(all).positional(positional);
      po::store(parser.run(), vm);
      if (vm.count("help")) {
        printHelp();
        std::cout << general << std::endl;
        return false;
      }
      po::notify(vm);
      validate();
    } catch (boost::program_options::error& e) {
      std::cerr << "Option parsing error: " << e.what() << std::endl;
      return false;
    }
    return true;
  }

  bool operator()(const std::string& name) const {
    return vm.count(name) != 0;
  }

 protected:
  virtual void addOptions(boost::program_options::options_description& /* desc */) {}

  virtual void addPositional(boost::program_options::options_description& /* desc */,
                             boost::program_options::positional_options_description& /* positional */) {}

  virtual void printHelp() {
    std::cout << "Help" << std::endl;
  }

  /** Validate supplied options.
    * This function is called directly after parsing command line.
    * Should throw boost::program_options::error if validation fails. */
  virtual void validate() {}

  boost::program_options::variables_map vm;
};
