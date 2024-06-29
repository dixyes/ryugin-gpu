# 龙吟显卡

## 准备

你得有个wave，可以参考[chdilo/BadAppleOSC](https://github.com/chdilo/BadAppleOSC) [yeonzi/badappe_oscilloscope](https://github.com/yeonzi/badappe_oscilloscope)

得有个vga的显卡，一个跑着linux的硬件

## 使用

**不建议在玩这个的时候接显示器，如果它去世了，我只能深表遗憾。**

### 阴间EDID

加载edid并强制开启vga：

将yinjian.bin打包到initramfs的/lib/firmware/edid/yinjian.bin，可以参考你的发行版所用的init的操作，参考[archwiki](https://wiki.archlinux.org/title/Kernel_mode_setting)

使用以下内核参数，根据你的引导方式修改

```
drm.edid_firmware=edid/yinjian.bin video=VGA-1:800x600e
```

说明：

`drm.edid_firmware=edid/yinjian.bin`：加载/lib/firmware/edid/yinjian.bin到所有接口上
`video=VGA-1:800x600e`：配置`VGA-1`接口为800x600的模式，`e`强制开启

### 跑它

```bash
cc playvideo.c -o playvideo -g -Ofast
sudo ./playvideo badapple\ matlab.wav
```

## License

```text
Copyright (C) 2024 Yun Dou <dixyes@gmail.com>

        DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE 
                    Version 2, December 2004 

 Copyright (C) 2004 Sam Hocevar <sam@hocevar.net> 

 Everyone is permitted to copy and distribute verbatim or modified 
 copies of this license document, and changing it is allowed as long 
 as the name is changed. 

            DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE 
   TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION 

  0. You just DO WHAT THE FUCK YOU WANT TO.
```
