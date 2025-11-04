# Test-Pubsub
Smaller isolation test to investigate some asio-grpc behaviours, particualry with handling stopped signals

# Build
Assuming you have conan:
```
conan install --build missing
cmake --preset=[conan-default|conan-debug|conan-release]
```