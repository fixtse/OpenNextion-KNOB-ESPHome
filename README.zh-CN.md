# OpenNextion Knob by fixtSE

由 [fixtSE](https://fixtse.com/) 打造。

**轻轻一转，掌控日常智能家居。**

将 ONX2424G013 变成小巧的 Home Assistant 控制器。通过圆形屏幕和一个旋钮，
调节房间环境、启动日常场景，随时查看家中状态。

[开始使用](docs/setup.md) · [下载固件](https://github.com/fixtse/esp-ota/releases) · [English](README.md)

## 掌控你的家

| 功能 | 你可以做什么 |
| --- | --- |
| 灯光 | 开关、调节亮度和颜色，精细调整色温。 |
| 温控 | 调节目标温度、切换 HVAC 模式、分别调整温度范围上下限，以及设备支持的预设、风速、摆风和湿度。 |
| 风扇 | 调节转速、开关电源，以及设备支持的预设、摇头和转向。 |
| 媒体 | 调节音量、播放或暂停、切换曲目，选择播放列表或电台；支持经典播放器和全屏专辑封面，进度环延伸到屏幕边缘。 |
| 开关与窗帘 | 控制开关、插座和窗帘位置。 |
| 辅助实体与日常操作 | 调节数值、确认选项、激活场景、触发按钮、运行脚本或自动化。 |
| 状态一览 | 查看传感器、天气，以及带动态进度环的计时器倒计时。 |

可用设置会根据 Home Assistant 实体的能力自动调整。

## 按你的习惯设置

- **最多二十个可配置位置。** 按名称、实体或房间搜索，重新排序或复制控制项。
- **直观的旋钮操作。** 旋转调节、短按执行、长按返回；支持的页面可按住并旋转来选择其他设置。
- **沿用你的 Home Assistant 图标。** 动态下载实体所选的 MDI 图标，下载不可用时使用内置默认图标。
- **清晰的操作反馈。** 立即显示请求值，并提示 Home Assistant 确认、连接状态或操作超时。
- **无需管理设备令牌。** 通过 ESPHome 集成控制 Home Assistant，在浏览器中登录并选择实体。
- **内置配置网页。** 控制列表在实体名称前显示图标；支持实时预览、主题、未保存修改恢复，以及配置备份与还原。
- **适合日常的屏幕。** 设置屏幕方向、亮度、旋钮灵敏度、日期和时间格式、媒体样式、进度环颜色、待机天气和睡眠时间。
- **轻松维护。** 支持 Wi-Fi 配网、实体状态自动恢复、无需重启的手动重试和无线固件更新。

## 更多日常功能

- **播放列表与电台：** 在网页中添加常用播放操作，在旋钮列表中选择后自动返回播放器。每个旋转刻度立即更新选择，不再等待滚动动画。[使用指南（英文）](docs/web-configuration.md#playlists-and-stations)
- **计时器不占满屏幕：** 可隐藏计时器的普通页面，活动计时器仍自动弹出；隐藏项仍计入二十个配置位置。[计时器指南（英文）](docs/web-configuration.md#timers-without-clutter)
- **提醒、图片与会议：** 通过 Home Assistant 显示带可选图片的消息，自动将 HTML 换行和实体转换为纯文本。图片下载失败时保留文字，新消息会取消旧图片请求；按下关闭、旋转阅读长消息。[提醒指南（英文）](docs/display-notifications.md)
- **自定义旋转操作：** 分别设置顺时针、逆时针操作，以及可选的按住旋转操作；支持显示反馈值。电脑控制需要已有的 Home Assistant 连接。[操作指南（英文）](docs/gesture-controls.md)
- **待机播放信息：** 选择时钟或正在播放的媒体，停止播放时回到时钟。[显示设置（英文）](docs/web-configuration.md#display-and-idle-screen)
- **按下后的返回值：** 在 **Press action** 中启用 **Show the returned value after pressing**，显示脚本返回的 `value` 和可选 `unit`。自动化可通过设备的反馈操作发送结果。此选项与旋转反馈独立，短暂弹窗不会拦截旋转操作。[示例（英文）](docs/gesture-controls.md#press-action-results)
- **清晰的脚本与自动化页面：** 居中图标，显示运行／空闲或启用／禁用状态，也可显示已配置的反馈值。

## 发送通知

图片和纯文字都使用同一个 **show_message** 操作：

```yaml
action: esphome.onx2424g013_dbbc54_show_message
data:
  title: 提醒
  message: 洗衣已完成。<br/>请取出衣物。
  image_url: ""
  timeout_seconds: 60
```

将设备前缀替换为你的设备名称。`image_url` 为空表示纯文字，也可填写旋钮能够访问的
HTTP 或 HTTPS 图片地址。四个字段均需填写；`timeout_seconds: 0` 表示等待按下关闭，
也可用 **dismiss_message** 远程关闭。图片布局保留至少三行文字空间，下载有大小和
时间限制。[图片要求和更多示例（英文）](docs/display-notifications.md)

## 开始使用

1. 在兼容的 **ONX2424G013** 上[安装固件](docs/setup.md)，并连接 Wi-Fi。
2. 在 Home Assistant 的 ESPHome 集成中添加设备，启用 **Allow the device to perform Home Assistant actions**。
3. 打开设备的 **Visit** 链接或 `http://<旋钮 IP>/`，登录 Home Assistant、选择实体并保存配置。

[控制指南](docs/knob-controls.md) · [配置与备份](docs/web-configuration.md)
