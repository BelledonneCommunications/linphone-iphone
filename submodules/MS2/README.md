## Simple example of MediaStreamer 2 usage

This Xcode project is a simple local (not using the network) example of use of the MediaStreamer2 API. **This project only works on a real device with a camera onboard: running it on the simulator will not work**.

It requires first that you compile the liblinphone SDK into the `submodules/build` directory. Please read theroot directory README for instructions on how to build this SDK.

After that, simply open the MS2.xcodeproj file with Xcode and press the Run button on the top.

You will be presented with a munimal UI that lets you start a streaming session, and switch rapidly between a static image and the live image coming from the camera sensor of the device.

