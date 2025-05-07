************************** MPEG-D USAC 音频编码器 - 第二版 2.3 ***************************
*                                                                                          *
*                    MPEG-D USAC 音频参考软件 - 中文说明文档                               *
*                                                                                          *
*                                        2024-01-17                                        *
*                                                                                          *
********************************************************************************************


(1) 软件包内容
============================================================================================
- 包含基于ANSI C/C++编写的MPEG-D USAC音频核心编解码器
- 支持Linux/Mac和Microsoft Visual Studio 2008/2012的构建文件


(2) 构建与运行要求
============================================================================================
- 必备软件包：
  . AFsp音频文件工具包（必须v9r0版本）
  . ISOBMFF参考软件（libisomediafile）
  . MPEG-D DRC参考软件
- Linux/Mac环境：
  . GNU make
  . gcc 4.4.5或更高版本
- Windows环境：
  . Visual Studio 2012
  . Windows SDK


(2.1) 安装AFsp音频工具包
============================================================================================
(a) 安装步骤：
1. 下载地址：http://www.mmsp.ece.mcgill.ca/Documents/Downloads/AFsp/
2. 解压AFsp-v9r0.tar.gz至./tools/AFsp/目录
3. 文件结构应包含：
   - Linux/Mac构建文件：./tools/AFsp/Makefile
   - Windows解决方案：./tools/AFsp/MSVC/MSVC.sln

(b) 自动构建：
软件构建时将自动编译libtsplite库


(2.2) 安装ISOBMFF媒体文件库
============================================================================================
(a) 安装步骤：
1. 从GitHub下载：https://github.com/MPEGGroup/isobmff/archive/master.zip
2. 解压libisomediafile至./tools/IsoLib/
3. 文件结构应包含：
   - Linux构建文件：./tools/IsoLib/libisomediafile/linux/libisomediafile/Makefile
   - VS2012解决方案：./tools/IsoLib/libisomediafile/w32/libisomediafile/VS2012/libisomedia.sln

(b) 自动构建：
MPEG-4 FF参考软件将在所有平台上与MPEG-D USAC音频参考软件一起自动构建


(2.3) 安装MPEG-D DRC参考软件
============================================================================================
(a) 安装步骤：
1. 从w17642 - ISO/IEC 23003-4:2015/AMD 2的DRC参考软件中复制/MPEG_D_DRC_refsoft文件夹
2. 将文件夹放入：./mpegD_drc/
3. 确保以下文件位置正确：
   - Windows编译脚本：./mpegD_drc/MPEG_D_DRC_refsoft/scripts/compile.bat
   - Linux编译脚本：./mpegD_drc/MPEG_D_DRC_refsoft/scripts/compile.sh

(b) 自动构建：
MPEG-D DRC参考软件将在所有平台上与MPEG-D USAC音频参考软件一起自动构建


(3) MPEG-D USAC音频参考软件构建指南
============================================================================================
(a) Linux/Mac系统：
1. 按照上述说明安装AFsp和MPEG-D DRC参考软件
2. 如需设置访问权限
3. 进入scripts目录：cd ./scripts
4. 执行编译脚本：./compile.sh
5. 检查executables_*目录中是否生成了所有6个文件

(b) Windows系统：
1. 按照上述说明安装AFsp和MPEG-D DRC参考软件
2. 确保环境变量VS110COMNTOOLS已正确设置
3. 进入scripts目录：cd scripts
4. 执行编译脚本：compile.bat
5. 检查executables_*目录中是否生成了所有6个文件

(c) 注意事项：
- 成功构建将显示"Build successful."并返回代码0
- 失败构建将显示"Build failed."并返回代码1
- 软件目录路径不能包含空格
- 查看compile.bat/compile.sh可了解单个模块的编译方法

(d) 高级构建选项：
- Linux/Mac：
  ./compile.sh Clean      # 清理所有中间文件、库和可执行文件
  ./compile.sh Release    # 发布模式构建，启用编译器优化
  ./compile.sh c89        # C89兼容模式构建
  ./compile.sh co         # 检出所需软件（如libisomediafile）
  ./compile.sh co GIT     # 使用GIT检出依赖

- Windows：
  compile.bat x64         # 64位Windows系统构建
  compile.bat Release     # 发布模式构建
  compile.bat VS2008      # 使用VS2008编译器构建
  compile.bat co          # 检出所需软件
  compile.bat co GIT      # 使用GIT检出依赖

(e) 编译器版本兼容性：
- Linux/Mac：
  . 支持gcc 3.3.5或更高版本
  . 使用./compile.sh c89启用兼容模式
- Windows：
  . 支持Visual Studio 2008（需要VS90COMNTOOLS环境变量）
  . 使用compile.bat VS2008启用兼容模式


(4) 快速入门：使用usacDec解码
============================================================================================
构建完成后，进入executables_*目录，不带参数运行usacDec[.exe]可查看所有可用命令行参数。

基本用法需要至少两个参数：输入文件(mp4)和输出文件，例如：
- Linux/Mac: ./usacDec -if <input>.mp4 -of <output>.wav
- Windows:   usacDec.exe -if <input>.mp4 -of <output>.wav


(4.1) 解码示例
============================================================================================
(a) 获取MPEG-D USAC音频一致性测试流：
1. 从ISO提供的URN下载MPEG-D USAC音频一致性测试流：
   http://standards.iso.org/iso-iec/23003/-3/ed-2/en
2. 将文件放入相应目录：
   - *.mp4文件放入./MPEGD_USAC_mp4目录
   - *.wav文件放入./MPEGD_USAC_ref目录

(b) 解码MPEG-D USAC音频一致性测试流：
1. 进入executables_*目录
2. 按照(4)中的说明调用usacDec[.exe]

(c) 比较MPEG-D USAC音频一致性测试流：
可以使用一致性工具ssnrcd或AFsp工具compAudio进行验证。

AFsp音频工具位置：
- Linux/Mac: ./tools/AFsp/bin
- Windows:   ./tools/AFsp/MSVC/bin

使用compAudio比较两个音频文件：
- Linux/Mac: ./compAudio <reference>.wav <MPEG-D_USAC_decoded>.wav
- Windows:   compAudio.exe <reference>.wav <MPEG-D_USAC_decoded>.wav

使用ssnrcd比较两个音频文件：
./ssnrcd <reference>.wav <MPEG-D_USAC_decoded>.wav


(5) 工具概述
============================================================================================
参考软件模块使用以下辅助工具：

- AFsp库（如上所述）

- libisomediafile（库）
  . 用于读写MP4文件的库

- ssnrcd（命令行）
  . 根据ISO/IEC 13818-4、ISO/IEC 14496-4、ISO/IEC 23003-3、ISO/IEC 23008-9进行音频一致性验证
  . 详见./tools/ssnrcd/readme.txt

- wavCutterCmdl（命令行）
  . 使用wavIO库处理wave和qmf文件
  . 用于剪切wave文件（移除/添加开始时的延迟和结束时的刷新样本）

- wavIO（库）
  . 用于读写PCM数据或qmf数据到wave文件的库


(6) 模块概述
============================================================================================
参考软件基于以下独立模块：

- mpegD_usac（命令行）
  . 基于USAC参考软件的核心编码器和解码器实现

- mpegD_drc（命令行/库）
  . 用于解码DRC序列并将其应用于音频信号的DRC模块


(7) MPEG-D USAC编码器参考实现
============================================================================================
编码器参考实现位于：
/mpegD_usac/usacEncDec/win32/usacEnc

注意：编码器与解码器共享源代码。因此这里给出了相应的Visual Studio解决方案路径。
请查看compile.sh了解编码器软件的构建信息。

编码器支持三种不同的操作模式：
- 仅支持频域编码的操作模式（类似AAC）：
  使用命令行参数'-usac_fd'启用
- 仅支持LPC域编码的操作模式：
  使用命令行参数'-usac_td'启用
- 在频域和LPC域编码模式之间切换的操作模式：
  使用命令行参数'-usac_switched'启用

此外，需要设置比特率。通过'-br'命令行开关设置，单位为比特每秒(bps)。

对于立体声输入和比特率<64kbps的情况，MPEG Surround立体声编码会自动激活。
对于立体声输入和比特率>=64kbps的情况，会激活离散立体声编码。
通过命令行开关'-ipd'激活MPEG Surround中的相位编码。
通过命令行开关'-mps_res'激活统一立体声编码。
通过命令行开关'-tsd applause_classifier.dat'激活掌声编码。
通过命令行开关'-usac_tw'激活时间扭曲滤波器组。

通过指定适当的DRC配置文件和DRC增益曲线，使用命令行开关'-drcConfigFile'和'-drcGainFile'激活动态范围控制(DRC)编码。

示例：
- usacEnc -if all_mono.wav -of output_switched.mp4 -usac_switched -br 32000 
  -> 切换操作模式
  -> 比特率：32kbps
  -> 输出文件：output_switched.mp4
  -> 输入文件：all_mono.wav
  -> 不使用MPEG Surround

- usacEnc -if example__DRC_noSbr_FD_input.wav -of output_applyDrc.mp4 -usac_fd -sbrRatioIndex 0
  -br 64000 -drcConfigFile example__DRC_noSbr_FD_config.xml -drcGainFile example__DRC_noSbr_FD_gains.dat
  -> 频域操作模式
  -> 比特率：64kbps
  -> 禁用SBR
  -> 输出文件：output_applyDrc.mp4
  -> 输入文件：example__DRC_noSbr_FD_input.wav
  -> 激活DRC编码
  -> drcToolEncoder输入文件：example__DRC_noSbr_FD_config.xml
                                example__DRC_noSbr_FD_gains.dat

- usacEnc -if all_mono.wav -of output_switched.mp4 -usac_switched -br 32000 -sbrRatioIndex 2 
  -> 切换操作模式
  -> 比特率：32kbps
  -> 8:3 SBR系统，核心编码器帧长度为768
  -> 输出文件：output_switched.mp4
  -> 输入文件：all_mono.wav
  -> 不使用MPEG Surround

- usacEnc -if all_mono.wav -of output_switched.mp4 -usac_switched -br 16000 -sbrRatioIndex 1
  -> 切换操作模式
  -> 比特率：16kbps
  -> 4:1 SBR系统，核心编码器帧长度为1024
  -> 输出文件：output_switched.mp4
  -> 输入文件：all_mono.wav
  -> 不使用MPEG Surround

- usacEnc -if all_mono.wav -of output_lpcDomain.mp4 -usac_td -br 30000
  -> LPC域操作模式
  -> 比特率：30kbps
  -> 输出文件：output_lpcDomain.mp4
  -> 输入文件：all_mono.wav
  -> 不使用MPEG Surround

- usacEnc -if all_stereo.wav -of output_freqDomain.mp4 -usac_fd -br 32000
  -> 频域操作模式
  -> 比特率：32kbps
  -> 输出文件：output_freqDomain.mp4
  -> 输入文件：all_stereo.wav
  -> 自动激活MPEG Surround

- usacEnc -if all_stereo.wav -of output_freqDomain.mp4 -usac_fd -usac_tw -br 64000
  -> 频域操作模式
  -> 比特率：64kbps
  -> 输出文件：output_freqDomain.mp4
  -> 输入文件：all_stereo.wav
  -> 激活离散立体声编码
  -> 激活时间扭曲滤波器组

- usacEnc -if all_stereo.wav -of output_freqDomain.mp4 -usac_fd -ipd -br 32000
  -> 频域操作模式
  -> 比特率：32kbps
  -> 输出文件：output_freqDomain.mp4
  -> 输入文件：all_stereo.wav
  -> 自动激活MPEG Surround
  -> 激活MPEG Surround中的相位编码

- usacEnc -if all_stereo -of output_uniStereo.mp4 -usac_fd -br 64000 -ipd -mps_res
  -> 频域操作模式
  -> 比特率：64kbps
  -> 输出文件：output_uniStereo.mp4
  -> 输入文件：all_stereo.wav
  -> 激活统一立体声编码

- usacEnc -if all_applause.wav -of output_applCoding.mp4 -usac_fd -br 32000 -tsd all_applause_TsdData_32s.dat
  -> 频域操作模式
  -> 比特率：32kbps
  -> 输出文件：output_applCoding.mp4
  -> 输入文件：all_applause.wav
  -> 激活掌声编码
  -> 掌声分类器：all_applause_TsdData_32s.dat

注意：编码器不包含音频输入的自动重采样功能，即编码项的采样率将与输入的采样率相同。
用户应选择合理的采样率/比特率组合，特别是在较低比特率下操作编码器时。
LPC域操作模式不支持的采样率/比特率组合将输出错误消息：
"EncUsacInit: no td coding mode found for the user defined parameters"。

为了向编码器提供具有适当采样率的wav文件，可能需要使用外部工具进行重采样步骤。
免费的重采样工具"ResampAudio"专为重采样音频wav文件而设计，可用于此目的。
ResampAudio是AFsp包的一部分，构建USAC参考软件也需要该包。
请参阅AFsp软件包随附的文档，了解如何在您的平台上构建ResampAudio。


(8) MPEG-D USAC解码器参考实现
============================================================================================
解码器参考实现位于：
/mpegD_usac/usacEncDec/win32/usacDec

注意：解码器与编码器共享源代码。因此这里给出了相应的Visual Studio解决方案路径。
请查看compile.sh了解解码器软件的构建信息。


(9) 技术支持
============================================================================================
如需MPEG-D USAC参考软件的构建和运行技术支持，请联系：
christian.neukam@iis.fraunhofer.de
christian.ertel@iis-extern.fraunhofer.de
mpeg-h-3da-maintenance@iis.fraunhofer.de


(10) 修订历史
============================================================================================
详见changes.txt文件获取完整的更改列表


(c) Fraunhofer IIS, VoiceAge Corp., 2008-10-08 - 2018-04-11
