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
### 创建新仓库
确定了解码器和编码器的相关参数，创建了新仓库以便管理与开发：
[ISO-IEC-USAC GitHub 仓库](https://github.com/xdzqyyds/ISO-IEC-USAC.git)


2025.5.12
### 编码器在VS2022中的编译构建
- 先编译成功，提供必要的依赖文件。
- 在VS2022中进行，忽略版本升级。
- 将调试中的工作目录改为 ```$(OutDir)```,代表可执行文件下的路径,并放入音频文件
- 当前已经通过修改编码端`vcproj`等一系列文件进行了默认配置，但`ISO-IEC-USAC GitHub` 仓库中没有修改

```
    宏名	                   含义
$(ProjectDir)	    项目文件所在的目录
$(SolutionDir)	    解决方案（.sln 文件）所在的目录
$(OutDir)	        编译生成的输出文件目录（如 Debug/Release）
$(TargetPath)	    最终生成的可执行文件（.exe）的完整路径
$(Configuration)	当前构建配置（如 Debug 或 Release）
```
- 正常配置命令行参数，进行调试。

### 时间测量

在开启`SBR` ,使用`MPS`处理双声道音频时，采样点数为`1024`，平均每帧的编码时长为`6.5ms`
```
encode per frame time: 6.50 ms
```
### 码率限制

最低码率为`17 kbps`

### 编码器封装
```
LIBXAACENC_API typedef struct {
    signed int bitrate;            // 编码码率(推荐16000)
    signed int ui_pcm_wd_sz;       // pcm格式(pcm_16le)
    signed int ui_samp_freq;       // 采样率
    signed int ui_num_chan;        // 通道数量
    signed int sbr_flag;           // 是否使用SBR(0-不使用, 1-使用)
    signed int mps_flag;           // 是否使用MPS(0-不使用, 1-使用)
    signed int Super_frame_mode;   // 超级帧格式（400ms或200ms）
} encode_para;


encode_obj* xheaace_create(encode_para* encode_para);

LIBXAACENC_API signed int xheaace_encode_frame(encode_obj* pv_encode_obj,  const unsigned char* raw_pcm_frame);

LIBXAACENC_API signed int xheaace_delete(encode_obj* pv_encode_obj);

```
2025.5.13
### 封装过程中需要处理的位置

#### 关注`audioFile->fileAfsp`   `fileNumSample = encNumSample`
```
audioFile = AudioOpenRead(audioFileName,&numChannel,&fSample,
                          &fileNumSample);
其中
`numChannel` 和 `fSample` 可以通过参数进行配置
`audioFile`则需要在后面着重考虑
`fileNumSample`表示采样点数，需要着重考虑
`file->fileAfsp = af`这句话要仔细考虑

af = AFopenRead(fileName,&ns,&nc,&fs,
    AUdebugLevel?stdout:(FILE*)NULL);
af的值来自上面的函数

```

#### 考虑设置一个固定的输出文件 `encoded.mp4`
```
 outfile = StreamFileOpenWrite(bitFileName, FILETYPE_AUTO, NULL);

 留意输出文件的设置
```

#### 函数内部并未用到`audioFile->fileAfsp`，该函数应该可以忽略
```
  AudioSeek(audioFile,
            startSample+delayNumSample-startupNumFrame*frameNumSample);
重点关注该函数，可能用于从音频文件中读取音频数据
```

#### `该函数在循环编码中起到关键作用，考虑新建音频采集接口函数代替，用于读取每一帧,其中sampleBuf为读取到的音频数据`
```
numSample = AudioReadData(audioFile,sampleBufRS,frameNumSample*downSampleRatio/upSampleRatio);

该函数从输入的音频文件中读取数据，并将读取的数据以每通道分离的二维 float 数组 data[channel][sample] 的形式输出
```

#### 关键函数，用于编码一帧，将结果输出到`au_buffers[track_count_total]`中
```
err = frameData.enc_mode->EncFrame(encSampleBuf,
                                   &au_buffers[track_count_total],
                                   requestIPF,
                                   &usacSyncState,
                                   *frameData.enc_data,
                                   sbrenable ? ((upSampleRatio > 1) ? tmpBufUS : tmpBufRS) : NULL);

关键函数，用于编码一帧，其输入参数有待考量
```

#### 考虑对`au`组装为超级帧
```
au->numBits         = BsBufferNumBit(au_buffers[i]);
au->data            = BsBufferGetDataBegin(au_buffers[i]);
关键函数，au表示编码后的比特流大小和指针

```

#### 考虑保留文件写入功能，在主动停止音频采集时触发写入磁盘操作
```
StreamPutAccessUnit(outprog, i, au);将比特率写入outprog
StreamFileSetEditlistValues;将元数据写入outprog
if (StreamFileClose(outfile))
  CommonExit(1,"Encode: error closing bit stream file");写入磁盘文件

```


2025.5.14
### 编码器封装
- 编码器封装完成，但是目前缺少超级帧的实现部分
由于我在现有项目中导入超级帧模块后报错很多，`因此考虑将超级帧部分单独封装为一个动态库，供编解码器调用`

`考虑在git上新建一个仓库，用来负责超级帧部分的动态库打包`

`已经完成了封装，现在先提交pr，然后再在本地新建一个项目用于测试`