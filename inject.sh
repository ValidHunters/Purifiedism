path="$1"

# cleanup
rm SS14.Reloader.so
rm -r Matsuoka/bin
rm -rf $path/SS14.Reloader.so
rm -rf /tmp/Matsuoka.dll

# Build
clang++ -shared -fPIC -fshort-wchar -ldl -Wall -pedantic main.cc -o SS14.Reloader.so
dotnet build ./Matsuoka/Matsuoka.csproj

# Copy
cp SS14.Reloader.so $path
cp ./Matsuoka/bin/Debug/net8.0/Matsuoka.dll /tmp/Matsuoka.dll

# Load
LD_PRELOAD=$path/SS14.Reloader.so DOTNET_ROOT=$(realpath $path/dotnet) COREHOST_TRACE=1 COREHOST_TRACE_VERBOSITY=3 $path/SS14.Launcher
