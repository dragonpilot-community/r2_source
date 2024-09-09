#!/usr/bin/bash -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null && pwd)"

cd $DIR

BUILD_DIR=/data/openpilot
SOURCE_DIR="$(git rev-parse --show-toplevel)"

FILES_SRC="release/files_eon"
DEVEL_BRANCH="d2"
TYPE="development"

# set git identity
source $DIR/identity.sh
export GIT_SSH_COMMAND="ssh -i /data/gitkey"

echo "[-] Setting up repo T=$SECONDS"
if [ -d $BUILD_DIR ]; then
  cd $BUILD_DIR
  git fetch origin
  git checkout $DEVEL_BRANCH
  git clean -xdf
  git reset --hard origin/$DEVEL_BRANCH
else
  rm -rf $BUILD_DIR
  cd /data/
  git clone https://github.com/dragonpilot-community/dragonpilot.git openpilot -b $DEVEL_BRANCH --single-branch
  cd $BUILD_DIR
fi

echo "[-] erasing old openpilot T=$SECONDS"
find . -maxdepth 1 -not -path './.git' -not -name '.' -not -name '..' -exec rm -rf '{}' \;

# do the files copy
echo "[-] copying files T=$SECONDS"
cd $SOURCE_DIR
cp -pR --parents $(cat release/files_common) $BUILD_DIR/
cp -pR --parents $(cat $FILES_SRC) $BUILD_DIR/

# in the directory
cd $BUILD_DIR

# do not bother to compile body
rm -fr body
rm -fr selfdrive/body

rm -f panda/board/obj/*

VERSION=$(date '+%Y.%m.%d')
echo "#define COMMA_VERSION \"$VERSION\"" > common/version.h

echo "[-] committing version $VERSION T=$SECONDS"
git add -f .
git commit -a -m "dragonpilot $TYPE version for EON/C2"
git branch --set-upstream-to=origin/$DEVEL_BRANCH

# Build panda firmware
pushd panda/
scons -j$(nproc) -u .
mv board/obj/panda.bin.signed /tmp/panda.bin.signed
mv board/obj/bootstub.panda.bin /tmp/bootstub.panda.bin
rm -fr board/obj/*
popd

# Build
export PYTHONPATH="$BUILD_DIR"
scons -j$(nproc)

# Build no nav ui
NONAV=1 scons -j$(nproc) selfdrive/ui/

# Ensure no submodules in release
if test "$(git submodule--helper list | wc -l)" -gt "0"; then
  echo "submodules found:"
  git submodule--helper list
  exit 1
fi
git submodule status

# Cleanup
find . -name '*.a' -delete
find . -name '*.o' -delete
find . -name '*.os' -delete
find . -name '*.pyc' -delete
find . -name 'moc_*' -delete
find . -name '*.cc' -delete
find . -name '__pycache__' -delete
find selfdrive/ui/ -name '*.h' -delete
rm -rf panda/board panda/certs panda/crypto
rm -rf .sconsign.dblite Jenkinsfile release/
rm -fr selfdrive/legacy_modeld/models/supercombo.dlc
rm -fr selfdrive/hybrid_modeld/models/supercombo.dlc
rm -fr selfdrive/hybrid_modeld/models/*.onnx
rm -fr selfdrive/hybrid_modeld/models/*_badweights.thneed

rm -fr selfdrive/ui/replay/
# Move back signed panda fw
mkdir -p panda/board/obj
# stock panda
mv /tmp/panda.bin.signed panda/board/obj/panda.bin.signed
mv /tmp/bootstub.panda.bin panda/board/obj/bootstub.panda.bin

# Restore third_party
git checkout third_party/

# dp clean up
rm -fr selfdrive/test
rm -fr cereal/gen
rm -fr selfdrive/legacy_modeld/models/*badweights*
rm -fr cereal/messaging/*.pyx
rm -fr cereal/visionipc/*.pyx
rm -fr cereal/messaging/tests
rm -fr selfdrive/manager/test
rm -fr third_party/libyuv/x86_64
rm -fr opendbc/can/*.pyx
rm -fr opendbc/can/*.hpp
rm -fr opendbc/can/tests
find . -name 'SConscript' -delete
find . -name 'SConstruct' -delete
rm -fr release/
rm -fr selfdrive/legacy_modeld/models/*.onnx
rm -fr tinygrad/
rm -fr tinygrad_repo/
rm -fr laika/
rm -fr system/loggerd/
rm -fr third_party/acados/larch64/
rm -fr third_party/acados/x86_64/
rm -fr selfdrive/assets/training/
rm -fr system/hardware/tici/updater
rm -fr selfdrive/legacy_modeld/thneed/compile
rm -fr third_party/snpe/aarch64-ubuntu-gcc7.5/
rm -fr third_party/snpe/larch64/
rm -fr third_party/snpe/x86*
rm -fr third_party/snpe/dsp/libsnpe_dsp_v68_domains_v3_skel.so
find . -name '*.c' -delete
find . -name '*.cpp' -delete
cp $SOURCE_DIR/panda/board/obj/bootstub.panda_h7.bin $BUILD_DIR/panda/board/obj/
cp $SOURCE_DIR/panda/board/obj/panda_h7.bin.signed $BUILD_DIR/panda/board/obj/

rm -fr selfdrive/modeld/

# Mark as prebuilt release
touch prebuilt

# include source commit hash and build date in commit
GIT_HASH=$(git --git-dir=$SOURCE_DIR/.git rev-parse HEAD)
DATETIME=$(date '+%Y-%m-%dT%H:%M:%S')
DP_VERSION=$(cat $SOURCE_DIR/common/version.h | awk -F\" '{print $2}')

# Add built files to git
git add -f .
git commit --amend -m "
version: dragonpilot $TYPE version for EON/C2
date: $DATETIME
commit: $GIT_HASH
"

if [ ! -z "$PUSH" ]; then
  echo "[-] pushing T=$SECONDS"
  git push -f origin $DEVEL_BRANCH
fi

echo "[-] done T=$SECONDS"
