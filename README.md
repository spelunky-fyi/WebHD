# Web HD

Coming Soon!

## Building

Requires CMake and Visual Studio (MSVC).

```bash
git clone --recurse-submodules https://github.com/spelunky-fyi/WebHD.git
cd WebHD
cmake -A Win32 -S . -B build
cmake --build build --config Release
```

The built DLL will be at `build/webhd/Release/webhd.dll`.

## Releasing

Update the `VERSION` file and push to `main`. The GitHub Actions workflow will build and create a release automatically.
