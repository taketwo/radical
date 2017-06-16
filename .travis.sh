RADICAL_DIR=`pwd`
RADICAL_BUILD_DIR=$RADICAL_DIR/build
OPENCV_DIR=$HOME/opencv
CMAKE_DIR=$HOME/cmake
EIGEN_DIR=$HOME/eigen
GLOG_DIR=$HOME/glog
CERES_DIR=$HOME/ceres
GIT_LFS_DIR=$HOME/git-lfs
DOWNLOAD_DIR=$HOME/download

function test()
{
  git lfs pull
  make tests
}

function build()
{
  mkdir $RADICAL_BUILD_DIR && cd $RADICAL_BUILD_DIR
  cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DOpenCV_DIR=$OPENCV_DIR/${OPENCV_VERSION}/share/OpenCV \
    -DCeres_DIR=$CERES_DIR/${CERES_VERSION}/share/Ceres \
    -DBUILD_TESTS=ON
  make -j2
}

function install_cmake()
{
  local pkg_ver=$CMAKE_VERSION
  local pkg_url="https://cmake.org/files/v2.8/cmake-${pkg_ver}.tar.gz"
  local pkg_md5sum="17c6513483d23590cbce6957ec6d1e66"
  local pkg_src_dir=${DOWNLOAD_DIR}/cmake-${pkg_ver}
  local pkg_install_dir=$CMAKE_DIR/${pkg_ver}
  local cmake_exe=$pkg_install_dir/bin/cmake
  echo "Installing CMake ${pkg_ver}"
  if [[ -e ${cmake_exe} ]]; then
    local version=`$cmake_exe --version | grep -Po "(?<=version ).*"`
    if [[ "$version" = "$pkg_ver" ]]; then
      local modified=`stat -c %y ${cmake_exe} | cut -d ' ' -f1`
      echo " > Found cached installation of CMake"
      echo " > Version ${pkg_ver}, built on ${modified}"
      return 0
    fi
  fi
  download ${pkg_url} ${pkg_md5sum}
  if [[ $? -ne 0 ]]; then
    return $?
  fi
  tar xzf pkg
  cd ${pkg_src_dir}
  ./bootstrap --prefix=$pkg_install_dir
  make -j2 && make install
  return $?
}

function install_opencv()
{
  local pkg_ver=${OPENCV_VERSION}
  local pkg_url="https://github.com/opencv/opencv/archive/${pkg_ver}.tar.gz"
  case $pkg_ver in
    "2.4.8") local pkg_md5sum="9b8f1426bc01a1ae1e8b3bce11dc1e1c";;
    "3.1.0") local pkg_md5sum="70e1dd07f0aa06606f1bc0e3fa15abd3";;
  esac
  local pkg_src_dir=${DOWNLOAD_DIR}/opencv-${pkg_ver}
  local pkg_install_dir=$OPENCV_DIR/${pkg_ver}
  local cmake_config=$pkg_install_dir/share/OpenCV/OpenCVConfig.cmake
  echo "Installing OpenCV ${pkg_ver}"
  if [[ -e ${cmake_config} ]]; then
    local version=`grep -Po "(?<=OpenCV_VERSION )[0-9.]*" ${cmake_config}`
    if [[ "$version" = "$pkg_ver" ]]; then
      local modified=`stat -c %y ${cmake_config} | cut -d ' ' -f1`
      echo " > Found cached installation of OpenCV"
      echo " > Version ${pkg_ver}, built on ${modified}"
      return 0
    fi
  fi
  download ${pkg_url} ${pkg_md5sum}
  if [[ $? -ne 0 ]]; then
    return $?
  fi
  tar xzf pkg
  cd ${pkg_src_dir}
  mkdir -p build && cd build
  cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=$pkg_install_dir \
    -DWITH_IPP=OFF \
    -DWITH_WEBP=OFF \
    -DBUILD_opencv_apps=OFF \
    -DBUILD_opencv_calib3d=OFF \
    -DBUILD_opencv_features2d=OFF \
    -DBUILD_opencv_flann=OFF \
    -DBUILD_opencv_java=OFF \
    -DBUILD_opencv_ml=OFF \
    -DBUILD_opencv_objdetect=OFF \
    -DBUILD_opencv_stitching=OFF \
    -DBUILD_opencv_superres=OFF \
    -DBUILD_opencv_ts=OFF \
    -DBUILD_opencv_viz=OFF
  make -j2 && make install && touch ${cmake_config}
  return $?
}

function install_eigen()
{
  local pkg_ver=${EIGEN_VERSION}
  local pkg_url=" http://bitbucket.org/eigen/eigen/get/${pkg_ver}.tar.gz"
  local pkg_md5sum="8ad10ac703a78143a4062c9bda9d8fd3"
  local pkg_src_dir=${DOWNLOAD_DIR}/eigen-eigen-b9cd8366d4e8
  local pkg_install_dir=${EIGEN_DIR}/${pkg_ver}
  local pkgconfig=${pkg_install_dir}/share/pkgconfig/eigen3.pc
  echo "Installing Eigen ${pkg_ver}"
  if [[ -e ${pkgconfig} ]]; then
    local version=`grep -Po "(?<=Version: )[0-9.]*" ${pkgconfig}`
    if [[ "${version}" = "$pkg_ver" ]]; then
      local modified=`stat -c %y ${pkgconfig} | cut -d ' ' -f1`
      echo " > Found cached installation of Eigen"
      echo " > Version ${pkg_ver}, built on ${modified}"
      return 0
    fi
  fi
  download ${pkg_url} ${pkg_md5sum}
  if [[ $? -ne 0 ]]; then
    return $?
  fi
  tar xzf pkg
  cd ${pkg_src_dir}
  mkdir -p build && cd build
  cmake .. -DCMAKE_INSTALL_PREFIX=${pkg_install_dir}
  make install && touch ${pkgconfig}
  return $?
}

function install_glog()
{
  local pkg_ver=${GLOG_VERSION}
  local pkg_url="https://github.com/google/glog/archive/v${pkg_ver}.tar.gz"
  local pkg_md5sum="5df6d78b81e51b90ac0ecd7ed932b0d4"
  local pkg_src_dir=${DOWNLOAD_DIR}/glog-${pkg_ver}
  local pkg_install_dir=${GLOG_DIR}/${pkg_ver}
  local pkgconfig=${pkg_install_dir}/lib/pkgconfig/libglog.pc
  echo "Installing GLOG ${pkg_ver}"
  if [[ -e ${pkgconfig} ]]; then
    local version=`grep -Po "(?<=Version: )[0-9.]*" ${pkgconfig}`
    if [[ "${version}" = "$pkg_ver" ]]; then
      local modified=`stat -c %y ${pkgconfig} | cut -d ' ' -f1`
      echo " > Found cached installation of GLOG"
      echo " > Version ${pkg_ver}, built on ${modified}"
      return 0
    fi
  fi
  download ${pkg_url} ${pkg_md5sum}
  if [[ $? -ne 0 ]]; then
    return $?
  fi
  tar xzf pkg
  cd ${pkg_src_dir}
  ./configure --prefix=$pkg_install_dir && make -j4 && make install
  make -j2 && make install && touch ${pkgconfig}
  return $?
}

function install_ceres()
{
  local pkg_ver=${CERES_VERSION}
  local pkg_url="http://ceres-solver.org/ceres-solver-${pkg_ver}.tar.gz"
  local pkg_md5sum="dbf9f452bd46e052925b835efea9ab16"
  local pkg_src_dir=${DOWNLOAD_DIR}/ceres-solver-${pkg_ver}
  local pkg_install_dir=${CERES_DIR}/${pkg_ver}
  local cmake_config=${pkg_install_dir}/share/Ceres/CeresConfig.cmake
  echo "Installing Ceres ${pkg_ver}"
  if [[ -e ${cmake_config} ]]; then
    local version=`grep -Po "(?<=CERES_VERSION )[0-9.]*" ${cmake_config}`
    if [[ "${version}" = "$pkg_ver" ]]; then
      local modified=`stat -c %y ${cmake_config} | cut -d ' ' -f1`
      echo " > Found cached installation of Ceres"
      echo " > Version ${pkg_ver}, built on ${modified}"
      return 0
    fi
  fi
  download ${pkg_url} ${pkg_md5sum}
  if [[ $? -ne 0 ]]; then
    return $?
  fi
  tar xzf pkg
  cd ${pkg_src_dir}
  mkdir -p build && cd build
  cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=$pkg_install_dir \
    -DEIGEN_INCLUDE_DIR=$EIGEN_DIR/$EIGEN_VERSION/include/eigen3 \
    -DGLOG_INCLUDE_DIR=$GLOG_DIR/$GLOG_VERSION/include \
    -DGLOG_LIBRARY=$GLOG_DIR/$GLOG_VERSION/lib/libglog.so \
    -DBUILD_EXAMPLES=OFF \
    -DBUILD_TESTING=OFF
  make -j2 && make install && touch ${cmake_config}
  return $?
}

function install_git_lfs() {
  local pkg_ver=${GIT_LFS_VERSION}
  local pkg_url="https://github.com/git-lfs/git-lfs/releases/download/v${pkg_ver}/git-lfs-linux-amd64-${pkg_ver}.tar.gz"
  local pkg_md5sum="c9b605a02dd68537d028e8a446fdee14"
  local pkg_src_dir=${DOWNLOAD_DIR}/git-lfs-${pkg_ver}
  local pkg_install_dir=${GIT_LFS_DIR}/${pkg_ver}
  local git_lfs_exe=$pkg_install_dir/git-lfs
  echo "Installing git-lfs ${pkg_ver}"
  if [[ -e ${git_lfs_exe} ]]; then
    local version=`$git_lfs_exe | grep -Po "(?<=git-lfs/).*(?= \()"`
    if [[ "$version" = "$pkg_ver" ]]; then
      local modified=`stat -c %y ${git_lfs_exe} | cut -d ' ' -f1`
      echo " > Found cached installation of git-lfs"
      echo " > Version ${pkg_ver}, downloaded on ${modified}"
      return 0
    fi
  fi
  download ${pkg_url} ${pkg_md5sum}
  if [[ $? -ne 0 ]]; then
    return $?
  fi
  tar xzf pkg
  mkdir -p $pkg_install_dir && cp ${pkg_src_dir}/git-lfs $pkg_install_dir
  return $?
}

function download()
{
  mkdir -p $DOWNLOAD_DIR && cd $DOWNLOAD_DIR && rm -rf *
  wget --no-check-certificate --output-document=pkg $1
  if [[ $? -ne 0 ]]; then
    return $?
  fi
  if [[ $# -ge 2 ]]; then
    echo "$2  pkg" > "md5"
    md5sum -c "md5" --quiet --status
    if [[ $? -ne 0 ]]; then
      echo "MD5 mismatch"
      return 1
    fi
  fi
  return 0
}

CMAKE_VERSION="2.8.12.2"
EIGEN_VERSION="3.2.10"
GLOG_VERSION="0.3.5"
CERES_VERSION="1.10.0"
GIT_LFS_VERSION="2.1.1"
export PATH=$CMAKE_DIR/$CMAKE_VERSION/bin:$GIT_LFS_DIR/$GIT_LFS_VERSION:$PATH

install_cmake && install_opencv && install_eigen && install_glog && install_ceres && install_git_lfs && build && test
