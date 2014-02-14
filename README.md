=====================
RK Logo Tool by Lolet
=====================

This tool is easy way to replace not only kernel logo picture, but also charging resources on your Rockchip device. All you need it's a kernel.img dumped from firmware or device. Should work on every RK device using CLUT format. It also can dump original pictures from image to PPM format. To edit it use IrfanView or GIMP. Don't forget to set color limit to 224 colors.

Usage
-------------------------
```
RKLogoTool kernel.img output_dir - extract pictures from kernel image
RKLogoTool 0xaddress.ppm kernel.img - replace picture in kernel image
```

Note
-------------------------
After you finish you need to update CRC of kernel image using mkkrnlimg.exe tool. This will be done automatically in next version of tool.
