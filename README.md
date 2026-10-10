# 椒苗智护

赛题二作品：GD32VF103 RISC-V + OpenHarmony LiteOS-M，C 语言。育苗棚单区域根区灌溉、通风和补光。

## 工程入口

- `main.c` 为唯一应用入口，`app/` 为新增业务代码。
- `liteos_m/`、`gd32vf103/`、`startup_gd32vf103.S`、`intexc_gd32vf103.S` 为用户提供的官方鸿蒙教程底座。
- 原 21185 的 `Firmware/`、`Hardware/`、`SYSTEM/`、`start.S`、`entry.S`、`init.c`、`handlers.c` 和原链接脚本保留作参考，**当前 Makefile 不编译它们**。两个 HAL 接口不同，不要混合头文件或一起编译启动程序。
- `tests/` 为纯 C 主机验证，不在 MCU 运行，不向传感采集注入预设输入。

## 编译

原平台编译器默认 `riscv64-unknown-elf-`，直接 `make`。
现代 xPack GNU RISC-V 工具链可以使用：

```sh
make PREFIX=riscv-none-elf- CORE_ARCH_ABI="-march=rv32imac_zicsr_zifencei -mabi=ilp32"
```

GNU make、工具链 `bin` 目录需在 PATH；有些编译器默认 ISA 版本不同，显式 zicsr/zifencei 只是声明 CSR 和 fence 指令，不引入浮点硬件需求。输出为 `build/main.elf`、`.hex`、`.bin`、`.map`。
现代编译器检查日志在交付包 `验证/`；官方平台必须另行编译并运行，不把本地构建成功当作平台成功。

## 接线及标定

详见包内接线表和原理图。PA0 DHT11，PA1土壤AO，PA2光照AO，PA3水位AO；PB0水泵、PA6风扇、PA7补光；PB12/13/14按键。TFT：PB6/PB7/PB5/PB4/PA15。USART0 PA9/10 115200 8N1。
输入0～3.3V；输出高电平开启。按键接GND，内部上拉低有效。
`app/board_port.h` 中调整 ADC 端点。百分比是相对归一化值，不是未经标定的体积含水率、lux 或水量。
首次运行可使用三个电位器验证ADC；土壤默认干4095/湿0，光照与水位0/4095。
实物水泵、风扇、补光需要功率驱动和共地，GPIO不直接带电机。

## 操作

- PB12短按：自动/手动，切换立即停泵并清除手动请求。
- PB13短按：育苗/移栽参数；长按1.5秒：FUZZY自适应/BASE阈值对照。
- PB14短按：仅手动模式开关泵；长按1.5秒：确认锁存故障。
- 防抖40ms，短按在释放时执行。故障确认不能绕过缺水、传感器无效或小时预算。

## 验证状态

主机策略测试覆盖启动、灌溉、缺水、过期、异常值、迟调度、人工操作、回绕与随机不变量。自建12分钟环境模型用于比较脉冲策略，结果不是官方仿真/真实栽培数据。
正式完成条件：平台编译、完整链路演示、故障注入、资源运行观测和录屏通过。当前版本的实际状态见交付包 `验证/构建结果.json` 及 `平台验收记录.md`。

