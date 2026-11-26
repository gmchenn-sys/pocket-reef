# 烧录说明

仅适用于 ESP32-S3 N16R8 + ST7789 SPI 240×240，GPIO 接线见 README。

## 方式 A：浏览器

电脑 Chrome / Edge 打开 https://gmchenn-sys.github.io/pocket-reef/ 。插入数据线，点击安装并选择串口。ESP Web Tools 会校验芯片家族，但不能替你识别屏幕型号、接线或 PSRAM 规格。固件使用固定的 16MB Flash / 8MB OPI PSRAM 配置。

首次安装可清空旧数据。更新 Pocket Reef 时，取消清空数据选项，四段固件写入不会覆盖 NVS 区域。

## 方式 B：下载后使用 Python

从 Releases 下载并解压 `pocket-reef-v30-firmware.zip`，进入解压后的目录：

```sh
python3 -m pip install "esptool==5.3.0"
python3 scripts/flash.py --port /dev/cu.usbmodemXXXX
```

Windows 可使用 `py -m pip install "esptool==5.3.0"`，然后 `py scripts/flash.py --port COM3`。串口名称用实际设备替换。脚本先校验下载文件 SHA-256，再按 ESP32-S3 的四个偏移写入，不会主动擦除 NVS。若需要清空旧归属，再额外使用 `--erase`；此选项会清空所有旧固件数据。

`--dry-run` 只打印要执行的命令，不接触开发板。连接失败时关闭占用串口的监视器；必要时按住 BOOT → 短按 RST → 等 2 秒松开 BOOT，再试一次。

## 方式 C：工厂合并镜像

也可下载 `pocket-reef-v30-factory.bin`，用 esptool 写入 **0x0**：

```sh
python3 -m esptool --chip esp32s3 --port PORT write-flash 0x0 pocket-reef-v30-factory.bin
```

**合并镜像会覆盖中间的数据区域，包括 NVS，适合新板/全新初始化。保留现有生物归属请使用方式 A 或 B。** `aquarium.bin` 只是应用分段，不能当作工厂镜像写入 0x0。

## 首次启动

手机连接 PocketReef / Reef-258614，访问 http://192.168.4.1/ 。热点是本地连接，没有互联网；让手机保留这个 Wi-Fi，页面保持前台。网页与固件不需要家庭 Wi-Fi 密码。

浏览器烧录使用 [ESP Web Tools](https://esphome.github.io/esp-web-tools/)。网页安装依赖在线加载工具，固件里的鱼缸界面则完全本地运行。
