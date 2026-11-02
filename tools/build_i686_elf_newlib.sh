#!/usr/bin/env bash
set -euo pipefail

BINUTILS_VERSION=2.42
GCC_VERSION=13.2.0
NEWLIB_VERSION=4.4.0.20231231

ROOT="${HOME}/.cache/mini-os-toolchain"
SOURCE_DIR="${ROOT}/src"
BUILD_DIR="${ROOT}/build"
PREFIX="${PREFIX:-${HOME}/.local/opt/i686-elf}"
JOBS="${JOBS:-1}"

mkdir -p "$SOURCE_DIR" "$BUILD_DIR" "$PREFIX"
export PATH="${PREFIX}/bin:${PATH}"

fetch_archive() {
  local url="$1"
  local archive="$2"
  local checksum="$3"

  if [[ -f "$archive" ]] &&
     printf '%s  %s\n' "$checksum" "$archive" | sha256sum --check --status; then
    return
  fi

  rm -f "${archive}.part"
  curl --fail --location --retry 3 "$url" --output "${archive}.part"
  printf '%s  %s\n' "$checksum" "${archive}.part" | sha256sum --check
  mv "${archive}.part" "$archive"
}

fetch_archive \
  "https://ftp.gnu.org/gnu/binutils/binutils-${BINUTILS_VERSION}.tar.xz" \
  "${SOURCE_DIR}/binutils-${BINUTILS_VERSION}.tar.xz" \
  f6e4d41fd5fc778b06b7891457b3620da5ecea1006c6a4a41ae998109f85a800
fetch_archive \
  "https://ftp.gnu.org/gnu/gcc/gcc-${GCC_VERSION}/gcc-${GCC_VERSION}.tar.xz" \
  "${SOURCE_DIR}/gcc-${GCC_VERSION}.tar.xz" \
  e275e76442a6067341a27f04c5c6b83d8613144004c0413528863dc6b5c743da
fetch_archive \
  "https://sourceware.org/pub/newlib/newlib-${NEWLIB_VERSION}.tar.gz" \
  "${SOURCE_DIR}/newlib-${NEWLIB_VERSION}.tar.gz" \
  0c166a39e1bf0951dfafcd68949fe0e4b6d3658081d6282f39aeefc6310f2f13

for source in \
  "binutils-${BINUTILS_VERSION}" \
  "gcc-${GCC_VERSION}" \
  "newlib-${NEWLIB_VERSION}"; do
  if [[ ! -d "${SOURCE_DIR}/${source}" ]]; then
    tar -xf "${SOURCE_DIR}/${source}".tar.* -C "$SOURCE_DIR"
  fi
done

BINUTILS_SOURCE="${SOURCE_DIR}/binutils-${BINUTILS_VERSION}"
GCC_SOURCE="${SOURCE_DIR}/gcc-${GCC_VERSION}"
NEWLIB_SOURCE="${SOURCE_DIR}/newlib-${NEWLIB_VERSION}"
COMMON_CONFIGURE=(
  --target=i686-elf
  "--prefix=${PREFIX}"
  --disable-nls
  --disable-shared
  --disable-threads
  --disable-multilib
  --disable-libssp
  --disable-libquadmath
  --disable-libgomp
)

mkdir -p "${BUILD_DIR}/binutils"
cd "${BUILD_DIR}/binutils"
"${BINUTILS_SOURCE}/configure" \
  --target=i686-elf \
  "--prefix=${PREFIX}" \
  --with-sysroot \
  --disable-nls \
  --disable-werror
make -j"$JOBS"
make install

mkdir -p "${BUILD_DIR}/gcc-bootstrap"
cd "${BUILD_DIR}/gcc-bootstrap"
"${GCC_SOURCE}/configure" \
  "${COMMON_CONFIGURE[@]}" \
  --enable-languages=c \
  --without-headers \
  --with-newlib
make -j"$JOBS" all-gcc all-target-libgcc
make install-gcc install-target-libgcc

mkdir -p "${BUILD_DIR}/newlib"
cd "${BUILD_DIR}/newlib"
rm -f "${BUILD_DIR}/newlib/i686-elf/newlib/config.cache"
"${NEWLIB_SOURCE}/configure" \
  --target=i686-elf \
  "--prefix=${PREFIX}" \
  --disable-nls \
  --disable-newlib-supplied-syscalls \
  --enable-newlib-io-c99-formats \
  --enable-newlib-io-long-long
make -j"$JOBS"
make install

mkdir -p "${BUILD_DIR}/gcc-final"
cd "${BUILD_DIR}/gcc-final"
"${GCC_SOURCE}/configure" \
  "${COMMON_CONFIGURE[@]}" \
  --enable-languages=c \
  --with-newlib
make -j"$JOBS" all-gcc all-target-libgcc
make install-gcc install-target-libgcc

printf '\nInstalled i686-elf GCC/Newlib toolchain at %s\n' "$PREFIX"
printf 'Add it to PATH with: export PATH="%s/bin:$PATH"\n' "$PREFIX"
