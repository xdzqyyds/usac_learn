2025.5.7

### 思考编解码流程
```
.wav（PCM）音频

    ↓  按帧读取（1024 采样）
    ↓  帧时长 = 采样点数 / 采样率 = 1024 / 44100 ≈ 23.22ms
    ↓  帧字节 = 采样点数 * 采样深度 * 通道数 = 1024 * 2 * 2  = 4096 字节

原始帧（固定时长 & 固定字节大小）

    ↓  编码器编码

编码帧（"固定时长" & 不定字节大小）

    ↓  超帧时长 = 400ms
    ↓  超帧字节 = 编码码率 * 超帧时长 = 64kbps * 0.4s / 8 = 4000 字节
    ↓  累计编码帧直到 ≈ 4000 字节

封装超级帧（带偏移索引）
    ↓  存储或网络发送
```
#### 编码帧的时长是否固定？

持续时长应该是原始帧或解码帧播放的持续时间，在编码帧这里衡量时长似乎没有意义，或者说编码帧的时长是逻辑意义上的，而组成编码帧的字节是物理意义上的。

换句话说，在处理所有音频文件中，始终都是对字节来进行处理，时间是逻辑上的估量，因此不能通过读取一段一段的固定时长来表示一帧一帧的编码数据。


2025.5.9
#### 测试libusac编解码器的所有功能

```
AOT : 42 - USAC
USAC Codec Mode : Switched Mode
Harmonic SBR : 0                  无效
High quality esbr : 0             无法使用
Complex Prediction Flag : 0       无效
TNS Flag : 0                      无效
Core-coder framelength index : 3  固定
Noise Filling Flag : 0            无效
Use ADTS Flag : 0                 固定
Use TNS Flag : 0                  无效
Bitrate : 64000 bps
Frame Length : 1024
Sampling Frequency : 44100 Hz
************************************************************************************************
```



#### libtsplite 构建失败
原因：环境变量配置出现疏漏
解决：VS110COMNTOOLS环境变量的最后要加上\ ，这样可以防止相对路径拼接出错
```
E:\VS2012\Common7\Tools\ 

```
#### core Encoder 构建失败
原因：在 \tools\IsoLib\libisomediafile\w32\libisomediafile\VS2012\libisomedia.vcxproj
        \tools\IsoLib\libisomediafile\w32\libisomediafile\VS2012\libisomedia.vcxproj.filters
     中缺少部分.c文件的引入
     
解决：分别添加下面内容即可

```
    <ClCompile Include="..\..\..\src\BoxedMetadataSampleEntry.c" />
    <ClCompile Include="..\..\..\src\MetadataKeyTableBox.c" />
    <ClCompile Include="..\..\..\src\MetadataKeyDeclarationBox.c" />
    <ClCompile Include="..\..\..\src\MetadataLocaleBox.c" />
    <ClCompile Include="..\..\..\src\MetadataSetupBox.c" />
    <ClCompile Include="..\..\..\src\MetadataKeyBox.c" />
    <ClCompile Include="..\..\..\src\VVCConfigAtom.c" />
    <ClCompile Include="..\..\..\src\VVCNALUConfigAtom.c" />
    <ClCompile Include="..\..\..\src\MP4MebxTrackReader.c" />
    <ClCompile Include="..\..\..\src\GroupsListBox.c" />
    <ClCompile Include="..\..\..\src\EntityToGroupBox.c" />
    <ClCompile Include="..\..\..\src\SegmentTypeAtom.c" />
    <ClCompile Include="..\..\..\src\SegmentIndexAtom.c" />
    <ClCompile Include="..\..\..\src\SubsegmentIndexAtom.c" />
    <ClCompile Include="..\..\..\src\ProducerReferenceTimeAtom.c" />
    <ClCompile Include="..\..\..\src\VolumetricVisualMediaHeaderAtom.c" />
```


2025.5.11
### 解码器参数
```

    -if <s>                    输入 MP4 文件的路径（input file）
    -of <s>                    输出 WAV 文件的路径（output file

[可选参数]:

    -cfg <s>                   二进制配置文件的路径（configuration file）
    -bitdepth <i>              解码器输出的量化位深，单位为 bit（位深）
    -use24bitChain             将内部处理链缩减为 24 位精度
                               <默认：无>，默认使用 32 位处理链
    -drcInterface <s>          DRC（动态范围控制）解码接口文件路径
    -targetLoudnessLevel <f>   解码输出的目标响度级别，单位为 LKFS
                               注意：若指定了 -drcInterface 且其内部定义了 targetLoudness 参数，则优先使用该接口文件中的设置
                               <默认：未设置>
    -drcEffectTypeRequest <i>  请求的 DRC 效果类型，使用十六进制格式
                               注意：若 -drcInterface 接口文件中定义drcEffectTypeRequest 参数，则优先使用该设置
                               <默认：未设置>
    -nodrc                     禁用动态范围控制（DRC）处理
    -nogc                      禁用垃圾回收器（GC）
    -cpo <i>                   一致性输出类型（Conformance Point Output）
                               0：输出完整的 USAC 解码器结果
                               <默认：0>
    -deblvl <0,1>              激活调试级别：0（默认），1
    -v                         启用详细输出等级 1（verbose level 1）
    -vv                        启用详细输出等级 2（verbose level 2）

[格式说明]:

    <s>                        字符串类型参数
    <i>                        整数类型参数
    <f>                        浮点类型参数

```

### 编码器参数
```

    -h                         打印帮助信息  
    -xh                        打印扩展帮助信息  
    -if <s>                    输入 WAV 音频文件的路径（input file）  
    -of <s>                    输出 MP4 文件的路径（output file）  
    -br <i>                    设置比特率（单位：kbit/s）

[可选参数]：

    -numChannelOut <i>         设置输出音频的声道数（0 表示与输入文件相同）（默认值：0）  
    -fSampleOut <i>            设置码流头部中的采样率（单位：Hz，0 表示与输入文件相同）（默认值：0）  
    -drcConfigFile <s>         指定包含 DRC（动态范围控制）配置的 XML 文件路径  
    -drcGainFile <s>           指定包含 DRC 增益数据的文件路径  
    -bw <i>                    设置音频带宽（单位：Hz）  
    -streamID <ui>             设置流标识符（Stream ID）  
    -drc                       启用 DRC 编码功能  
    -cfg                       指定二进制配置文件的路径  
    -usac_switched             启用 USAC 切换编码（默认）  
    -usac_fd                   启用 USAC 频域编码（Frequency Domain）  
    -usac_td                   启用 USAC 时域编码（Temporal Domain）  
    -noSbr                     禁用 SBR（谱包络复制）工具  
    -sbrRatioIndex <i>         设置 SBR 比率索引，可选值为 0、1、2、3（默认）  
    -mps_res                   启用 MPEG Surround 残差功能（需要启用 SBR）  
    -hSBR                      启用 SBR 的谐波拼接处理（harmonic patching）  
    -ms <0,1,2>                设置中-侧立体声（mid/side stereo）掩码方式：0、1（默认）、2  
    -wshape                    使用 WS_DOLBY 窗函数代替默认的 WS_FHG 窗函数  
    -deblvl <0,1>              设置调试级别：0（默认），1  
    -v                         启用详细输出等级 1（verbose level 1）  
    -vv                        启用详细输出等级 2（verbose level 2）  
    -nogc                      禁用垃圾回收器（GC）

使用 -xh 查看扩展帮助！

[格式说明]：

    <s>     字符串类型参数（string）  
    <i>     整数类型参数（integer）  
    <ui>    无符号整数类型参数（unsigned integer）  
    <f>     浮点类型参数（float）  

```



- Linux/Mac环境：
  . GNU make
  . gcc 4.4.5或更高版本
- Windows环境：
  . Visual Studio 2012
  . Windows SDK

1. 确保环境变量```VS110COMNTOOLS```已正确设置   ```E:\VS2012\Common7\Tools\```
2. 进入scripts目录：```cd scripts```
3. 执行编译脚本：```compile.bat```
4. 检查executables_*目录中是否生成了所有6个文件