# Linux Build Notes

by [utaysi](https://github.com/utaysi)

## Engine
- Unreal Engine 5.4 built from source at `/opt/unreal-engine-5.4.0/`
- Linux only supports Vulkan (no OpenGL backend)

## Build Command
```bash
/opt/unreal-engine-5.4.0/Engine/Build/BatchFiles/Linux/Build.sh \
  TBRaymarchProjectEditor Linux Development \
  -Project="/path/to/TBRaymarchProject.uproject"
```

## DCMTK 3.6.8 — Rebuilding Static Libraries for Linux

The plugin bundles DCMTK as static libraries. The Windows `.lib` files are
checked in; the Linux `.a` files must be built with specific flags to be
ABI-compatible with UE5's toolchain.

### Prerequisites
- System `clang` and `clang++` (not UE5's bundled clang — see below)
- `libc++` and `libc++abi` development packages (`libc++-dev`, `libc++abi-dev` or equivalent)
- DCMTK 3.6.8 source

### Why not use UE5's bundled clang as a cross-compiler?
DCMTK's cmake build uses `try_run()` for arithmetic type detection (`arith.h`
generation). Cross-compiling with UE5's toolchain breaks these runtime checks
because the executables target UE5's CentOS 7 sysroot, which may not run on the
host. Using the system clang with `-stdlib=libc++` produces ABI-compatible
objects without the cross-compilation issues.

### CMake Configure
```bash
cmake <dcmtk-3.6.8-source> \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_SHARED_LIBS=OFF \
  -DDCMTK_ENABLE_PRIVATE_TAGS=OFF \
  -DDCMTK_WITH_ZLIB=OFF -DDCMTK_WITH_PNG=OFF -DDCMTK_WITH_TIFF=OFF \
  -DDCMTK_WITH_XML=OFF -DDCMTK_WITH_ICONV=OFF -DDCMTK_WITH_ICU=OFF \
  -DDCMTK_WITH_OPENSSL=OFF -DDCMTK_WITH_SNDFILE=OFF -DDCMTK_WITH_WRAP=OFF \
  -DDCMTK_WITH_DOXYGEN=OFF -DDCMTK_WITH_OPENJPEG=OFF \
  -DDCMTK_ENABLE_CHARSET_CONVERSION=OFF \
  -DBUILD_APPS=OFF \
  -DCMAKE_CXX_STANDARD=17 \
  -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
  -DCMAKE_CXX_FLAGS="-fPIC -stdlib=libc++" \
  -DCMAKE_C_FLAGS="-fPIC -std=gnu17" \
  -DCMAKE_EXE_LINKER_FLAGS="-stdlib=libc++ -lc++abi" \
  -DHAVE_STRLCPY=FALSE \
  -DHAVE_STRLCAT=FALSE
```

Key flags explained:
- **`-stdlib=libc++`**: UE5 uses libc++, not libstdc++. Mixing ABIs causes linker errors.
- **`-DHAVE_STRLCPY=FALSE -DHAVE_STRLCAT=FALSE`**: Newer glibc provides these, but
  UE5's CentOS 7 sysroot does not. Force DCMTK to use its internal fallbacks.
- **`-DDCMTK_ENABLE_CHARSET_CONVERSION=OFF`**: Disables iconv dependency. The generated
  `osconfig_linux.h` must then have `DCMTK_ENABLE_CHARSET_CONVERSION` manually set to
  `DCMTK_CHARSET_CONVERSION_OFICONV` to avoid an `-Wundef` error.

### Post-build Steps

1. Copy these `.a` files to `Plugins/.../ThirdParty/dcmtk/lib/Linux/`:
   ```
   libdcmjpls.a  libdcmjpeg.a  libdcmimage.a  libdcmimgle.a
   libdcmdata.a  liboflog.a    libofstd.a     liboficonv.a
   libdcmtkcharls.a  libijg8.a  libijg12.a  libijg16.a
   ```

2. Copy generated headers to `ThirdParty/dcmtk/include/dcmtk/config/`:
   - `osconfig.h` → `osconfig_linux.h`
   - `arith.h` → `arith_linux.h`

3. Patch `osconfig_linux.h`: add or change:
   ```c
   #define DCMTK_ENABLE_CHARSET_CONVERSION DCMTK_CHARSET_CONVERSION_OFICONV
   ```

4. The dispatcher headers (`osconfig.h`, `arith.h`) already `#include` the
   `*_linux.h` or `*_win64.h` variant based on platform — no changes needed there.

### glibc C23 Compatibility Shim (`libisoc23_compat.a`)

On glibc 2.38+, when `_DEFAULT_SOURCE` is defined (which DCMTK sets internally),
the C library redirects standard functions to C23 versions:
- `sscanf` → `__isoc23_sscanf`
- `strtol` → `__isoc23_strtol`

These symbols are baked into the DCMTK `.a` files at compile time. UE5's CentOS 7
sysroot doesn't have them, causing unresolved symbol errors at link time.

**Fix**: A small shim library that forwards these back to the standard versions:

```c
// isoc23_compat.c
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

int __isoc23_sscanf(const char *str, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int ret = vsscanf(str, fmt, ap);
    va_end(ap);
    return ret;
}

long __isoc23_strtol(const char *nptr, char **endptr, int base) {
    return strtol(nptr, endptr, base);
}
```

Build **without** `_DEFAULT_SOURCE` to avoid the same redirect:
```bash
clang -std=c17 -fPIC -c isoc23_compat.c -o isoc23_compat.o
ar rcs libisoc23_compat.a isoc23_compat.o
```

Copy `libisoc23_compat.a` to `ThirdParty/dcmtk/lib/Linux/`. It is already
referenced in `VolumeTextureToolkit.Build.cs`.

## Vulkan Shader Parameter API

UE 5.3 deprecated the per-call `SetShaderValue(RHICmdList, ShaderRHI, ...)`
and `SetTextureParameter(RHICmdList, ShaderRHI, ...)` APIs. On Vulkan, these
cause null `FRHITexture*` crashes in `RHISetShaderTexture`.

Use the batched API instead:
```cpp
FRHIBatchedShaderParameters& Params = RHICmdList.GetScratchShaderParameters();
SetShaderValue(Params, MyParam, Value);
SetTextureParameter(Params, MyTexture, TextureRHI);
SetSamplerParameter(Params, MySampler, SamplerStateRHI);
SetUAVParameter(Params, MyUAV, UAVRef);
RHICmdList.SetBatchedShaderParameters(ShaderRHI, Params);
```

Do **not** unbind resources by setting them to `nullptr` through the batched API
— Vulkan will dereference the null pointer. Resource transitions
(`RHICmdList.Transition()`) handle state management on Vulkan/D3D12.
