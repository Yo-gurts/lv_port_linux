# libfreetype 编译方法

1. 获取freetype
wget http://download.savannah.gnu.org/releases/freetype/freetype-2.10.0.tar.gz
tar -xvzf freetype-2.10.0.tar.gz

2. 编译
cd freetype-2.10.0

./configure \
  --host=arm-none-linux-musleabihf \
  --prefix=/path/to/arm-sysroot/usr \
  --with-png=no \  # 若需 libpng 支持，改为 --with-png=/path/to/arm-sysroot/usr
  --with-zlib=no \
  --without-harfbuzz  # 若无需 harfbuzz 支持

make -j$(nproc)

相关库文件在./objs/.libs/目录