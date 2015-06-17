//regenerate libdummy.a using:
// for arch in arm64 armv7 i386 x86_64; do 
//     clang -c dummy.c -o dummy-$arch.a -arch $arch
// done
// lipo -create -output libdummy.a dummy-*.a
void dummy_does_nothing() {}
