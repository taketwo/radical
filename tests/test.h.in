#define TEST_DATA_DIR "@TEST_DATA_DIR@"

#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>

#include <opencv2/core/core.hpp>

inline std::string getTestFilename(const std::string& relative_path) {
  boost::filesystem::path p(std::string(TEST_DATA_DIR));
  return (p / relative_path).make_preferred().string();
}

inline std::string getTemporaryFilename() {
  boost::filesystem::path p(boost::filesystem::temp_directory_path());
  return (p / boost::filesystem::unique_path()).make_preferred().string();
  // TODO: return an object which deletes the file upon destruction
}

inline void setRNGSeed(int seed) {
#if CV_MAJOR_VERSION > 3 || (CV_MAJOR_VERSION == 3 && CV_MINOR_VERSION > 1)
  cv::setRNGSeed(seed);   // this function was added in 3.2
#else
  cv::theRNG().state = seed;
#endif
}

inline cv::Mat_<cv::Vec3b> generateRandomImage(int height, int width) {
  cv::Mat_<cv::Vec3b> image(height, width);
  cv::randu(image, cv::Scalar(0, 0, 0), cv::Scalar(255, 255, 255));
  return image;
}

inline cv::Mat_<cv::Vec3b> generateRandomImage(const cv::Size& size) {
  return generateRandomImage(size.height, size.width);
}

template<typename T> std::vector<T> generateRandomVector(size_t size, T min, T max) {
  std::vector<T> vector(size);
  cv::randu(vector, min, max);
  return vector;
}

template<typename T> T generateRandomNumber(T min, T max) {
  std::vector<T> vector(1);
  cv::randu(vector, min, max);
  return vector[0];
}

/** Test two cv::Mat for equality (same size, type, and elements). */
template <typename T>
boost::test_tools::predicate_result compareMat(const cv::Mat& m1, const cv::Mat& m2) {
  if (m1.size() != m2.size()) {
    boost::test_tools::predicate_result result(false);
    result.message() << "Different sizes [" << m1.size() << " != " << m2.size() << "]";
    return result;
  }
  if (m1.type() != m2.type()) {
    boost::test_tools::predicate_result result(false);
    result.message() << "Different types [" << m1.type() << " != " << m2.type() << "]";
    return result;
  }
  for (int row = 0; row < m1.rows; ++row) {
    for (int col = 0; col < m1.cols; ++col) {
      const auto& v1 = m1.at<T>(row, col);
      const auto& v2 = m2.at<T>(row, col);
      if (v1 != v2) {
        boost::test_tools::predicate_result result(false);
        result.message() << "Elements at position [" << row << ", " << col << "] are different [" << v1 << " != " << v2
                         << "]";
        return result;
      }
    }
  }
  return true;
}

/** Test each element of a cv::Mat with a given element for equality. */
template <typename T>
boost::test_tools::predicate_result compareMat(const cv::Mat& m, const T& element) {
  T e(element);
  for (int row = 0; row < m.rows; ++row) {
    for (int col = 0; col < m.cols; ++col) {
      const auto& v = m.at<T>(row, col);
      if (v != e) {
        boost::test_tools::predicate_result result(false);
        result.message() << "Element at position [" << row << ", " << col << "] is different [" << v << " != " << e
                         << "]";
        return result;
      }
    }
  }
  return true;
}

#define BOOST_CHECK_EQUAL_MAT(M1, M2, T) BOOST_CHECK((compareMat<T>(M1, M2)))
#define BOOST_REQUIRE_EQUAL_MAT(M1, M2, T) BOOST_REQUIRE((compareMat<T>(M1, M2)))
