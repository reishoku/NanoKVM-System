# NanoKVM-System
This repository contains parts of NanoKVM compiled using MaixCDK, including kvm_system and kvm_vision
The repository contains the kvm_system source code and update packages from NanoKVM PCIe/Full/Lite.

note: This repository is only a backup of the MaixCDK part of the NanoKVM project. Updates may occur more frequently than in the main NanoKVM repository. Please submit issues or pull requests in the main NanoKVM repository.

## If you want to compile kvm_system yourself, please follow the steps below: 

1. Clone the MaixCDK repository: `git clone https://github.com/sipeed/MaixCDK.git`
2. Improve the Compilation Environment, refer to [here](https://github.com/sipeed/MaixCDK/tree/main/docs/doc_zh#%E5%BF%AB%E9%80%9F%E5%BC%80%E5%A7%8B)
3. Copy `./kvm_system` to `~/MaixCDK` and open it in terminal
4. Execute `maixcdk build` and Select `maixcam` to complete the compilation
5. Replace `/kvmapp/kvm_system/kvm_system` in the NanoKVM system with `dist/kvm_system_release/kvm_system`
6. Execute: `/etc/init.d/S95nanokvm restart` on NanoKVM

## Update record

### 1/17 2025
+ First release

### 2/18 2025
+ Add the kvm_vision section of code
+ Add the kvm_mmf section of code
+ Remove the mandatory UID authentication for the mmf section
+ (Beta) HDMI may fail to automatically obtain the correct physical resolution in rare cases, leading to issues retrieving images from the VI subsystem. The following modifications will be made:
    + Added HDMI auto-detection mode. By running echo 1 > /etc/kvm/hdmi_mode, the system will automatically probe through the resolution list one by one
    + Added manual mode. By running echo 2 > /etc/kvm/hdmi_mode, you can switch to this mode. You can manually set the current HDMI physical resolution by executing echo 1920 > /kvmapp/kvm/width and echo 1080 > /kvmapp/kvm/height
    + It is recommended to enable logging in manual mode by setting level = debug in server.yaml (/etc/kvm/server.yaml -> logger). This will provide warnings if the current resolution width or height is too large or too small
    + In auto-detection mode, if a suitable resolution is not found, the system will automatically switch to manual mode
    + When in either of the above modes and a resolution change is in progress, calling the read API will return -4 (Resolution modification in progress, please try again later)
    + When the /etc/kvm/hdmi_mode file is missing, empty, or set to 0, the system will revert to normal mode (retrieving HDMI resolution from I2C)
    + While manually modifying the resolution, some excessively small values may cause initialization errors in VI, leading to program hangs. A minimum value restriction has been added to prevent this issue
+ Rewrote the new app initialization and new image functions
+ Remove the mandatory DNS configuration modification; apply it only during new image creation
+ Tailscale automatically modifies the resolv.conf file. In the previous version, this was disabled in the code; it will be disabled in S98 starting from the next version
+ Remove the mandatory override of S98tailscaled for the new app, keeping only a copy in /kvmapp/system/init.d/
+ Remove S50ssdpd; NanoKVM currently does not require network discovery. Keep only a copy in /kvmapp/system/init.d/

note: To compile kvm_vision_test, you need to replace the components/vision/ directory under MaixCDK, and move components/kvm/ and components/kvm_mmf/ into MaixCDK/components. However, there might still be some component modifications that haven't been uploaded, so I'll continue to look for them.

note: Regarding the jpg_stream in the kvmapp directory: In the earlier versions of the app (1.0.x), the transmission of streams relied on an application named jpg_stream. To ensure that users with app versions lower than 1.1.x can upgrade normally, this file was included in the upgrade package.

note: Regarding kvmapp/kvm_system/kvm_stream: In the earlier versions of the app (2.0.x), the video data stream transmission relied on an application named kvm_stream. During this period, a rather unwise judgment criterion was used for production testing: it checked whether the kvm_stream file existed in kvmapp/kvm_system. To ensure that all production testing image cards from this period could function normally, an empty file was added to the installation package. Once we confirm that all the image cards from this batch have been consumed, I will remove the empty kvm_stream file from this location.

note: In applications with an app version greater than 2.1.0, we use a server written in Go to complete video stream transmission. However, Go has difficulty accessing data from the chip's MIPI CSI -> VI -> VPSS -> VENC path. Therefore, I developed the kvm_vision application using MaixCDK, which allows the Go side to conveniently obtain encoded images through the generated libkvm.so dynamic library. The libkvm.so handles all the details but relies on an upstream encryption library. After version 2.2.0, we abandoned the design of the encryption library and replaced it with libkvm_mmf.so. All related code has been open-sourced in the components/ directory.
