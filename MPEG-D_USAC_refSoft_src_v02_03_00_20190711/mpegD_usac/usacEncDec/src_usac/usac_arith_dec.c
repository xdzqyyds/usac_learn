/*******************************************************************************
This software module was originally developed by

Fraunhofer IIS, VoiceAge Corp.


Initial author:
Guillaume Fuchs      (Fraunhofer IIS)

and edited by:
2009-01-14 FhG IIS: support reduced arithmetic coder tables

-

in the course of development of ISO/IEC 23003 for reference purposes and its
performance may not have been optimized. This software module is an
implementation of one or more tools as specified by ISO/IEC 23003. ISO/IEC gives
You a royalty-free, worldwide, non-exclusive, copyright license to copy,
distribute, and make derivative works of this software module or modifications
thereof for use in implementations of ISO/IEC 23003 in products that satisfy
conformance criteria (if any). Those intending to use this software module in
products are advised that its use may infringe existing patents. ISO/IEC have no
liability for use of this software module or modifications thereof. Copyright is
not released for products that do not conform to audiovisual and image-coding
related ITU Recommendations and/or ISO/IEC International Standards.

Assurance that the originally developed software module can be used (1) in
ISO/IEC 23003 once ISO/IEC 23003 has been adopted; and (2) to develop ISO/IEC
23003:s
Fraunhofer IIS, VoiceAge Corp. grant(s) ISO/IEC all
rights necessary to include the originally developed software module or
modifications thereof in ISO/IEC 23003 and to permit ISO/IEC to offer You a
royalty-free, worldwide, non-exclusive, copyright license to copy, distribute,
and make derivative works for use in implementations of ISO/IEC 23003 in
products that satisfy conformance criteria (if any), and to the extent that such
originally developed software module or portions of it are included in ISO/IEC
23003. To the extent that Fraunhofer IIS, VoiceAge Corp.
own(s) patent rights that would be required to make, use, or sell the
originally developed software module or portions thereof included in ISO/IEC
23003 in a conforming product, Fraunhofer IIS, VoiceAge Corp. 
will assure the ISO/IEC that it is (they are) willing to negotiate
licenses under reasonable and non-discriminatory terms and conditions with
applicants throughout the world. ISO/IEC gives You a free license to this
software module or modifications thereof for the sole purpose of developing
ISO/IEC 23003.

Fraunhofer IIS, VoiceAge Corp. retain full right to
modify and use the code for its (their) own purpose, assign or donate the code
to a third party and to inhibit third parties from using the code for products
that do not conform to MPEG-related ITU Recommendations and/or ISO/IEC
International Standards. This copyright notice must be included in all copies or
derivative works.

Copyright (c) ISO/IEC 2008.
*******************************************************************************/
#include <math.h>
#include <string.h>
#include "common_m4a.h"
#include "buffers.h"
#include "allHandles.h"
#include "bitstream.h"
#include "usac_port.h"
#include "usac_arith_dec.h"
#include "common_m4a.h"
#include "usac_allVariables.h"

/* local defines */
#ifndef int64
#ifdef _MSC_VER
#define int64 signed __int64
#else
#define int64   long long
#endif
#endif
#define iq_table_size  1024
#define exp_table_size 128
#define num_codes	17
#define cbits 		16
#define stat_bits	14

#define ari_q4	(((long)1<<cbits)-1)
#define ari_q1	(ari_q4/4+1)
#define ari_q2	(2*ari_q1)
#define ari_q3	(3*ari_q1)

static const int A_thres = 3;
static const int B_thres = 3;
static const int Val_esc = 16;

#include "usac_arith_tables.h" /* needs defined stat_bits */



/* local types*/
typedef struct {
  int	low,high,vobf;
} Tastat;

/* local variabels */
static Tqi2 q_unknown={0,0,0,0};

const float iq_table [iq_table_size] =  /* pow (i, 4.0F / 3.0F) */
  {
    0.000000F, 1.000000F, 2.519842F, 4.326749F, 6.349605F, 8.549880F, 10.902724F, 13.390519F,
    16.000002F, 18.720757F, 21.544350F, 24.463783F, 27.473145F, 30.567354F, 33.741997F, 36.993187F,
    40.317478F, 43.711792F, 47.173351F, 50.699638F, 54.288361F, 57.937416F, 61.644875F, 65.408951F,
    69.227989F, 73.100456F, 77.024910F, 81.000008F, 85.024506F, 89.097198F, 93.216988F, 97.382812F,
    101.593681F, 105.848648F, 110.146820F, 114.487335F, 118.869400F, 123.292229F, 127.755081F, 132.257263F,
    136.798096F, 141.376923F, 145.993134F, 150.646133F, 155.335358F, 160.060226F, 164.820221F, 169.614853F,
    174.443604F, 179.306015F, 184.201599F, 189.129944F, 194.090607F, 199.083176F, 204.107239F, 209.162415F,
    214.248322F, 219.364594F, 224.510880F, 229.686829F, 234.892090F, 240.126373F, 245.389313F, 250.680649F,
    256.000031F, 261.347229F, 266.721893F, 272.123779F, 277.552582F, 283.008087F, 288.490021F, 293.998108F,
    299.532135F, 305.091827F, 310.676941F, 316.287292F, 321.922638F, 327.582764F, 333.267426F, 338.976440F,
    344.709625F, 350.466705F, 356.247559F, 362.051941F, 367.879669F, 373.730591F, 379.604492F, 385.501221F,
    391.420563F, 397.362396F, 403.326508F, 409.312744F, 415.320953F, 421.350983F, 427.402649F, 433.475830F,
    439.570343F, 445.686066F, 451.822845F, 457.980530F, 464.158966F, 470.358032F, 476.577606F, 482.817535F,
    489.077698F, 495.357971F, 501.658173F, 507.978241F, 514.318054F, 520.677429F, 527.056274F, 533.454529F,
    539.871948F, 546.308533F, 552.764160F, 559.238708F, 565.731995F, 572.243958F, 578.774536F, 585.323608F,
    591.890991F, 598.476685F, 605.080566F, 611.702454F, 618.342346F, 625.000122F, 631.675659F, 638.368896F,
    645.079712F, 651.808044F, 658.553711F, 665.316772F, 672.097046F, 678.894470F, 685.708923F, 692.540405F,
    699.388733F, 706.253845F, 713.135742F, 720.034241F, 726.949341F, 733.880859F, 740.828857F, 747.793152F,
    754.773682F, 761.770386F, 768.783203F, 775.812073F, 782.856873F, 789.917603F, 796.994080F, 804.086365F,
    811.194275F, 818.317810F, 825.456848F, 832.611389F, 839.781311F, 846.966614F, 854.167175F, 861.382935F,
    868.613831F, 875.859802F, 883.120789F, 890.396729F, 897.687561F, 904.993286F, 912.313721F, 919.648865F,
    926.998718F, 934.363159F, 941.742065F, 949.135559F, 956.543396F, 963.965637F, 971.402222F, 978.853027F,
    986.318054F, 993.797241F, 1001.290466F, 1008.797791F, 1016.319092F, 1023.854370F, 1031.403564F, 1038.966553F,
    1046.543213F, 1054.133789F, 1061.737915F, 1069.355835F, 1076.987183F, 1084.632202F, 1092.290649F, 1099.962646F,
    1107.647949F, 1115.346558F, 1123.058594F, 1130.783813F, 1138.522217F, 1146.273926F, 1154.038574F, 1161.816406F,
    1169.607300F, 1177.411255F, 1185.228027F, 1193.057739F, 1200.900391F, 1208.755859F, 1216.624023F, 1224.505005F,
    1232.398682F, 1240.304932F, 1248.223877F, 1256.155396F, 1264.099487F, 1272.056030F, 1280.025024F, 1288.006470F,
    1296.000244F, 1304.006470F, 1312.024902F, 1320.055664F, 1328.098633F, 1336.153809F, 1344.221191F, 1352.300659F,
    1360.392212F, 1368.495728F, 1376.611328F, 1384.738892F, 1392.878418F, 1401.029907F, 1409.193237F, 1417.368408F,
    1425.555298F, 1433.754028F, 1441.964478F, 1450.186646F, 1458.420532F, 1466.666016F, 1474.923096F, 1483.191772F,
    1491.471924F, 1499.763672F, 1508.066772F, 1516.381470F, 1524.707520F, 1533.044922F, 1541.393677F, 1549.753784F,
    1558.125122F, 1566.507812F, 1574.901611F, 1583.306763F, 1591.723022F, 1600.150391F, 1608.588867F, 1617.038452F,
    1625.499023F, 1633.970703F, 1642.453369F, 1650.946899F, 1659.451538F, 1667.966919F, 1676.493286F, 1685.030518F,
    1693.578491F, 1702.137329F, 1710.706909F, 1719.287231F, 1727.878296F, 1736.480103F, 1745.092529F, 1753.715576F,
    1762.349243F, 1770.993408F, 1779.648315F, 1788.313599F, 1796.989502F, 1805.675903F, 1814.372681F, 1823.079834F,
    1831.797485F, 1840.525635F, 1849.263916F, 1858.012695F, 1866.771729F, 1875.541016F, 1884.320679F, 1893.110474F,
    1901.910522F, 1910.720703F, 1919.541138F, 1928.371704F, 1937.212402F, 1946.063110F, 1954.923950F, 1963.794922F,
    1972.675781F, 1981.566650F, 1990.467651F, 1999.378540F, 2008.299316F, 2017.229980F, 2026.170654F, 2035.121216F,
    2044.081543F, 2053.051758F, 2062.031738F, 2071.021484F, 2080.020996F, 2089.030273F, 2098.049316F, 2107.078125F,
    2116.116455F, 2125.164551F, 2134.222168F, 2143.289551F, 2152.366455F, 2161.452881F, 2170.549072F, 2179.654541F,
    2188.769775F, 2197.894287F, 2207.028320F, 2216.171875F, 2225.324951F, 2234.487305F, 2243.659180F, 2252.840576F,
    2262.031006F, 2271.230957F, 2280.440186F, 2289.658691F, 2298.886475F, 2308.123779F, 2317.370117F, 2326.625732F,
    2335.890381F, 2345.164551F, 2354.447510F, 2363.739990F, 2373.041504F, 2382.352051F, 2391.671875F, 2401.000488F,
    2410.338379F, 2419.685303F, 2429.041260F, 2438.406250F, 2447.780273F, 2457.163086F, 2466.555176F, 2475.956055F,
    2485.365723F, 2494.784424F, 2504.212158F, 2513.648682F, 2523.093994F, 2532.548340F, 2542.011230F, 2551.483154F,
    2560.963867F, 2570.453125F, 2579.951416F, 2589.458496F, 2598.974121F, 2608.498535F, 2618.031494F, 2627.573486F,
    2637.123779F, 2646.682861F, 2656.250732F, 2665.827148F, 2675.412109F, 2685.005615F, 2694.607910F, 2704.218506F,
    2713.837891F, 2723.465576F, 2733.102051F, 2742.746826F, 2752.400146F, 2762.061768F, 2771.732178F, 2781.410889F,
    2791.097900F, 2800.793457F, 2810.497314F, 2820.209717F, 2829.930176F, 2839.659424F, 2849.396729F, 2859.142334F,
    2868.896240F, 2878.658691F, 2888.429199F, 2898.208008F, 2907.995117F, 2917.790527F, 2927.594238F, 2937.406006F,
    2947.225830F, 2957.054199F, 2966.890381F, 2976.734863F, 2986.587646F, 2996.448486F, 3006.317383F, 3016.194336F,
    3026.079346F, 3035.972656F, 3045.873779F, 3055.783203F, 3065.700439F, 3075.625977F, 3085.559326F, 3095.500732F,
    3105.449951F, 3115.407471F, 3125.372803F, 3135.345947F, 3145.327148F, 3155.316162F, 3165.313232F, 3175.318359F,
    3185.331055F, 3195.351807F, 3205.380371F, 3215.416748F, 3225.460938F, 3235.513184F, 3245.572998F, 3255.640625F,
    3265.716064F, 3275.799316F, 3285.890381F, 3295.989258F, 3306.095703F, 3316.209961F, 3326.332031F, 3336.461670F,
    3346.598877F, 3356.744141F, 3366.896729F, 3377.057129F, 3387.225098F, 3397.400879F, 3407.583984F, 3417.774902F,
    3427.973633F, 3438.179688F, 3448.393311F, 3458.614502F, 3468.843262F, 3479.079590F, 3489.323486F, 3499.574951F,
    3509.833984F, 3520.100342F, 3530.374268F, 3540.655518F, 3550.944580F, 3561.240723F, 3571.544678F, 3581.855713F,
    3592.174316F, 3602.500488F, 3612.833984F, 3623.174805F, 3633.522949F, 3643.878662F, 3654.241455F, 3664.611816F,
    3674.989502F, 3685.374512F, 3695.766846F, 3706.166504F, 3716.573242F, 3726.987549F, 3737.409180F, 3747.837891F,
    3758.273926F, 3768.717041F, 3779.167725F, 3789.625488F, 3800.090332F, 3810.562500F, 3821.041992F, 3831.528564F,
    3842.022217F, 3852.523193F, 3863.031250F, 3873.546387F, 3884.068848F, 3894.598145F, 3905.134766F, 3915.678711F,
    3926.229492F, 3936.787354F, 3947.352295F, 3957.924561F, 3968.503662F, 3979.089844F, 3989.683105F, 4000.283447F,
    4010.890625F, 4021.504883F, 4032.126221F, 4042.754639F, 4053.389893F, 4064.032227F, 4074.681641F, 4085.337891F,
    4096.000977F, 4106.671387F, 4117.348145F, 4128.032227F, 4138.723145F, 4149.420898F, 4160.125488F, 4170.837402F,
    4181.555664F, 4192.281250F, 4203.013672F, 4213.752441F, 4224.498535F, 4235.251465F, 4246.010742F, 4256.777344F,
    4267.550293F, 4278.330566F, 4289.117188F, 4299.911133F, 4310.711426F, 4321.518555F, 4332.332520F, 4343.153320F,
    4353.980469F, 4364.814941F, 4375.655762F, 4386.503418F, 4397.357422F, 4408.218750F, 4419.086426F, 4429.960938F,
    4440.841797F, 4451.729492F, 4462.624023F, 4473.524902F, 4484.432617F, 4495.347168F, 4506.268066F, 4517.195801F,
    4528.129883F, 4539.070801F, 4550.018066F, 4560.972168F, 4571.932617F, 4582.899902F, 4593.873535F, 4604.854004F,
    4615.840820F, 4626.833984F, 4637.833984F, 4648.840332F, 4659.853516F, 4670.872559F, 4681.898926F, 4692.931152F,
    4703.970215F, 4715.015625F, 4726.067383F, 4737.125977F, 4748.190430F, 4759.261719F, 4770.339844F, 4781.423828F,
    4792.514160F, 4803.611328F, 4814.714844F, 4825.824707F, 4836.940918F, 4848.063477F, 4859.192383F, 4870.327637F,
    4881.469238F, 4892.617676F, 4903.771973F, 4914.932617F, 4926.100098F, 4937.273438F, 4948.453125F, 4959.639160F,
    4970.831543F, 4982.030273F, 4993.235352F, 5004.446777F, 5015.664062F, 5026.888184F, 5038.118164F, 5049.354492F,
    5060.597168F, 5071.846191F, 5083.101074F, 5094.362793F, 5105.630371F, 5116.904297F, 5128.184082F, 5139.470215F,
    5150.762695F, 5162.061523F, 5173.366211F, 5184.677246F, 5195.994629F, 5207.317871F, 5218.646973F, 5229.982910F,
    5241.324707F, 5252.672363F, 5264.026855F, 5275.386719F, 5286.752930F, 5298.125488F, 5309.503906F, 5320.888672F,
    5332.279297F, 5343.676270F, 5355.079102F, 5366.488281F, 5377.902832F, 5389.324219F, 5400.751465F, 5412.184570F,
    5423.623535F, 5435.068848F, 5446.520020F, 5457.977539F, 5469.440918F, 5480.910156F, 5492.385742F, 5503.866699F,
    5515.354004F, 5526.847656F, 5538.346680F, 5549.852051F, 5561.363281F, 5572.880371F, 5584.403809F, 5595.932617F,
    5607.467773F, 5619.008789F, 5630.555664F, 5642.108398F, 5653.666992F, 5665.231934F, 5676.802246F, 5688.378906F,
    5699.960938F, 5711.549316F, 5723.143555F, 5734.743652F, 5746.349121F, 5757.960938F, 5769.578613F, 5781.202148F,
    5792.831543F, 5804.466309F, 5816.107422F, 5827.754395F, 5839.406738F, 5851.064941F, 5862.729492F, 5874.399414F,
    5886.075195F, 5897.756836F, 5909.444336F, 5921.137695F, 5932.836426F, 5944.541016F, 5956.251465F, 5967.967773F,
    5979.689941F, 5991.417480F, 6003.151367F, 6014.890625F, 6026.635254F, 6038.386230F, 6050.142578F, 6061.904785F,
    6073.672363F, 6085.445801F, 6097.225098F, 6109.010254F, 6120.800781F, 6132.597168F, 6144.398926F, 6156.206543F,
    6168.020020F, 6179.838867F, 6191.663574F, 6203.493652F, 6215.329590F, 6227.171387F, 6239.018555F, 6250.871094F,
    6262.729492F, 6274.593750F, 6286.463379F, 6298.338379F, 6310.219238F, 6322.105957F, 6333.998047F, 6345.895508F,
    6357.798828F, 6369.707520F, 6381.621582F, 6393.541504F, 6405.467285F, 6417.398438F, 6429.334961F, 6441.276855F,
    6453.224609F, 6465.177734F, 6477.136230F, 6489.100586F, 6501.070312F, 6513.045410F, 6525.026367F, 6537.012695F,
    6549.004395F, 6561.001953F, 6573.004395F, 6585.012695F, 6597.026367F, 6609.045410F, 6621.070312F, 6633.100098F,
    6645.135742F, 6657.176758F, 6669.223145F, 6681.275391F, 6693.332520F, 6705.395508F, 6717.463379F, 6729.537109F,
    6741.616211F, 6753.700684F, 6765.790527F, 6777.885742F, 6789.986328F, 6802.092773F, 6814.204102F, 6826.320801F,
    6838.442871F, 6850.570801F, 6862.703613F, 6874.841797F, 6886.985840F, 6899.134766F, 6911.289062F, 6923.448730F,
    6935.613770F, 6947.784180F, 6959.959961F, 6972.141113F, 6984.327637F, 6996.519043F, 7008.716309F, 7020.918457F,
    7033.125977F, 7045.338867F, 7057.557129F, 7069.780762F, 7082.009766F, 7094.243652F, 7106.482910F, 7118.727539F,
    7130.977539F, 7143.232910F, 7155.493164F, 7167.758789F, 7180.029785F, 7192.306152F, 7204.587402F, 7216.874023F,
    7229.166016F, 7241.462891F, 7253.765625F, 7266.073242F, 7278.385742F, 7290.703613F, 7303.026855F, 7315.355469F,
    7327.688965F, 7340.027832F, 7352.371582F, 7364.720703F, 7377.075195F, 7389.434570F, 7401.799316F, 7414.168945F,
    7426.543945F, 7438.924316F, 7451.309570F, 7463.700195F, 7476.095703F, 7488.496582F, 7500.902344F, 7513.313477F,
    7525.729492F, 7538.150391F, 7550.577148F, 7563.008301F, 7575.445312F, 7587.886719F, 7600.333496F, 7612.785645F,
    7625.242676F, 7637.704590F, 7650.171875F, 7662.644043F, 7675.121582F, 7687.604004F, 7700.091309F, 7712.583984F,
    7725.081543F, 7737.583984F, 7750.091797F, 7762.604492F, 7775.122559F, 7787.645020F, 7800.172852F, 7812.706055F,
    7825.244141F, 7837.787109F, 7850.334961F, 7862.887695F, 7875.445801F, 7888.008789F, 7900.577148F, 7913.149902F,
    7925.728027F, 7938.311035F, 7950.898926F, 7963.492188F, 7976.089844F, 7988.692871F, 8001.300781F, 8013.913574F,
    8026.531738F, 8039.154297F, 8051.782227F, 8064.415039F, 8077.052734F, 8089.695312F, 8102.342773F, 8114.995117F,
    8127.652832F, 8140.314941F, 8152.982422F, 8165.654297F, 8178.331543F, 8191.013672F, 8203.700195F, 8216.392578F,
    8229.088867F, 8241.791016F, 8254.497070F, 8267.208984F, 8279.924805F, 8292.646484F, 8305.373047F, 8318.103516F,
    8330.839844F, 8343.580078F, 8356.326172F, 8369.076172F, 8381.831055F, 8394.591797F, 8407.356445F, 8420.126953F,
    8432.901367F, 8445.680664F, 8458.464844F, 8471.253906F, 8484.048828F, 8496.847656F, 8509.651367F, 8522.458984F,
    8535.272461F, 8548.090820F, 8560.914062F, 8573.741211F, 8586.574219F, 8599.411133F, 8612.253906F, 8625.100586F,
    8637.952148F, 8650.808594F, 8663.669922F, 8676.536133F, 8689.407227F, 8702.282227F, 8715.163086F, 8728.047852F,
    8740.937500F, 8753.832031F, 8766.731445F, 8779.635742F, 8792.544922F, 8805.458008F, 8818.376953F, 8831.299805F,
    8844.227539F, 8857.160156F, 8870.097656F, 8883.039062F, 8895.985352F, 8908.937500F, 8921.893555F, 8934.854492F,
    8947.819336F, 8960.790039F, 8973.764648F, 8986.744141F, 8999.728516F, 9012.717773F, 9025.710938F, 9038.709961F,
    9051.712891F, 9064.719727F, 9077.732422F, 9090.750000F, 9103.771484F, 9116.797852F, 9129.828125F, 9142.864258F,
    9155.904297F, 9168.949219F, 9181.999023F, 9195.052734F, 9208.112305F, 9221.175781F, 9234.243164F, 9247.316406F,
    9260.393555F, 9273.475586F, 9286.562500F, 9299.653320F, 9312.749023F, 9325.849609F, 9338.954102F, 9352.064453F,
    9365.178711F, 9378.296875F, 9391.420898F, 9404.548828F, 9417.680664F, 9430.818359F, 9443.959961F, 9457.106445F,
    9470.256836F, 9483.412109F, 9496.572266F, 9509.737305F, 9522.906250F, 9536.080078F, 9549.257812F, 9562.440430F,
    9575.627930F, 9588.819336F, 9602.016602F, 9615.216797F, 9628.422852F, 9641.632812F, 9654.846680F, 9668.066406F,
    9681.289062F, 9694.517578F, 9707.750000F, 9720.987305F, 9734.228516F, 9747.474609F, 9760.725586F, 9773.980469F,
    9787.240234F, 9800.503906F, 9813.772461F, 9827.045898F, 9840.323242F, 9853.605469F, 9866.891602F, 9880.182617F,
    9893.478516F, 9906.778320F, 9920.083008F, 9933.391602F, 9946.705078F, 9960.022461F, 9973.344727F, 9986.671875F,
    10000.002930F, 10013.337891F, 10026.678711F, 10040.022461F, 10053.372070F, 10066.724609F, 10080.083008F, 10093.445312F,
    10106.811523F, 10120.182617F, 10133.557617F, 10146.937500F, 10160.322266F, 10173.710938F, 10187.103516F, 10200.500977F,
    10213.903320F, 10227.309570F, 10240.719727F, 10254.134766F, 10267.554688F, 10280.978516F, 10294.406250F, 10307.838867F
  };

static const float exp_table [exp_table_size] =  /* pow (2.0, 0.25 * i) */
  {
    1.0F, 1.189207F, 1.414214F, 1.681793F, 2.0F, 2.378414F, 2.828427F, 3.363586F,
    4.0F, 4.756828F, 5.656854F, 6.727171F, 8.0F, 9.513657F, 11.313708F, 13.454343F,
    16.0F, 19.027313F, 22.627417F, 26.908686F, 32.0F, 38.054626F, 45.254833F, 53.817371F,
    64.0F, 76.109253F, 90.509666F, 107.634743F, 128.0F, 152.218506F, 181.019333F, 215.269485F,
    256.0F, 304.437012F, 362.038666F, 430.538971F, 512.0F, 608.874023F, 724.077332F, 861.077942F,
    1024.0F, 1217.748047F, 1448.154663F, 1722.155884F, 2048.0F, 2435.496094F, 2896.309326F, 3444.311768F,
    4096.0F, 4870.992188F, 5792.618652F, 6888.623535F, 8192.0F, 9741.984375F, 11585.237305F, 13777.247070F,
    16384.0F, 19483.968750F, 23170.474609F, 27554.494141F, 32768.0F, 38967.9375F, 46340.949219F, 55108.988281F,
    65536.0F, 77935.8750F, 92681.898438F, 110217.976563F, 131072.0F, 155871.75F, 185363.796875F, 220435.953125F,
    262144.0F, 311743.50F, 370727.593750F, 440871.906250F, 524288.0F, 623487.0F, 741455.187500F, 881743.812500F,
    1048576.0F, 1246974.0F, 1482910.375000F, 1763487.625000F, 2097152.0F, 2493948.0F, 2965820.750000F, 3526975.25F,
    4194304.0F, 4987896.0F, 5931641.50F, 7053950.50F, 8388608.0F, 9975792.0F, 11863283.0F, 14107901.0F,
    16777216.0F, 19951584.0F, 23726566.0F, 28215802.0F, 33554432.0F, 39903168.0F, 47453132.0F, 56431604.0F,
    67108864.0F, 79806336.0F, 94906264.0F, 112863208.0F, 134217728.0F, 159612672.0F, 189812528.0F, 225726416.0F,
    268435456.0F, 319225344.0F, 379625056.0F, 451452832.0F, 536870912.0F, 638450688.0F, 759250112.0F, 902905664.0F,
    1073741824.0F, 1276901376.0F, 1518500224.0F, 1805811328.0F, 2147483648.0F, 2553802752.0F, 3037000448.0F, 3611622656.0F,
  };

/* local functions */
static long decode2(unsigned char *bbuf,long bp,Tqi2 q[],Tqi2 *qn,long n, long nt);
static long encode2(unsigned char *bbuf,long bp,Tqi2 q[],Tqi2 *qn,long n,long nt,int use_stop);
static void map_context(int size,int previousSize,Tqi2 arithQ[]);
static long get_state(Tqi2 q[], long *s, int bin);
static void init_state(Tqi2 q[], long *s);
static unsigned long get_pk(unsigned long s);
static long ari_start_decoding(unsigned char *buf,long bp,Tastat *s);
static void ari_start_encoding(Tastat *s);
static long ari_decode(unsigned char *buf,long bp,int *res,Tastat *s,unsigned short const *cum_freq,long cfl);
static long ari_encode(unsigned char *buf,long bp,Tastat *s,long symbol,unsigned short const *cum_freq);
static long ari_done_encoding(unsigned char *buf,long bp,Tastat *s);
static float esc_iquant(int q, float noiseLevel , int withNoise, unsigned int *nfSeed);
static float NoiseLevel(int noiseLevel);
static long mul_sbc(long r,long c);
static void ari_copy_states(Tastat *source, Tastat *dest);
#ifdef UNIFIED_RANDOMSIGN
static float randomSign(unsigned int *seed);
#else
static unsigned int nfRand(unsigned int *seed);
#endif


static void applyScaleFactorsAndNf(int bNoiseFillingActive,
                                   int blockSize,
                                   Info* info,
                                   float* coef,
                                   int* quant,
                                   int noise_level,
                                   short* factors,
                                   byte max_sfb,
                                   unsigned int* nfSeed);




/* function definitions*/
/*-----------------------------------------------------------------------------
  functionname: acSpecFrame
  description:  AAC arithmetic decoding+inverse Q+noise filling+scaling
  input params:
  returns:
  ------------------------------------------------------------------------------*/
void acSpecFrame(HANDLE_USAC_DECODER      hDec,
                 Info*                    info,
                 float*                   coef,
                 int                      max_spec_coefficients,
                 int                      nlong,
                 int                      noise_level,
                 short*                   factors,
                 int                      arithSize,
                 int                     *arithPreviousSize,
                 Tqi2                     arithQ[],
                 HANDLE_BUFFER            hVm,
                 byte                     max_sfb,
                 int                      reset,
                 unsigned int             *nfSeed,
                 int                      bUseNoiseFilling)
{
  int                 i;
  int                 ind = 0;
  int                 bitsRead = 0;
  /*int                 reset;*/
  HANDLE_BSBITBUFFER  h_arith_bb = GetRemainingBufferBits (hVm);
  unsigned char*      arith_bb = BsBufferGetDataBegin ( h_arith_bb );
  int                 quant[1024];
  int sbk;
#ifdef NOISEFILLING_NO_BUGFIX
  int sfb, group;
  float fac;
#endif
  float noiseLevel_float = 0.f;
  float noiseLevelTmp = 0.f;
  int groupWin = 0;
  int groupSbkOffset = 0;
  int offset = 0;
  int sfbOffset = 0;
  const int nsbk = info->nsbk;
  Tqi2  arithQTmp[(1024/2)+4];
  int isNoise[8][MAXBANDS] = {{0}};


  /*
   * Arithmetic decoding
   */
  
  /* Initialization*/
  intclr(quant, 1024);

  if(reset){
    for(i=0;i<(1024/2)+4;i++){
      arithQ[i]=q_unknown;
      arithQTmp[i] = q_unknown;
    }    
  }
  else { /*Mapping of the context*/
    map_context(arithSize, *arithPreviousSize, arithQ);
  }

  *arithPreviousSize = arithSize;
  
  /*Arithmetic decoding*/
  if(max_spec_coefficients > 0){
    
    for(sbk=0; sbk<nsbk;  sbk++){
      bitsRead = decode2(arith_bb, bitsRead, arithQ+2, &arithQTmp[sbk*arithSize/2], max_spec_coefficients/2, arithSize/2);

      /* copy data from arithQ to quant */
      for(i=0; i<max_spec_coefficients/2; i++){
        arithQ[i+2] =  arithQTmp[sbk*arithSize/2+i];
        quant[sbk*arithSize+2*i + 0] = arithQTmp[sbk*arithSize/2+i].a;
        quant[sbk*arithSize+2*i + 1] = arithQTmp[sbk*arithSize/2+i].b;
      }
      for(; i<arithSize/2; i++){
        quant[sbk*arithSize+2*i + 0] = 0;
        quant[sbk*arithSize+2*i + 1] = 0;
      }
    }
  }
#ifndef ARITH_DEC_RESET_CONTEXT_NOFIX
  else {
    for(i = 0; i < (arithSize / 2); i++){
      arithQ[i+2].c = 1;
    }
  }
#endif

  SkipBits(hVm, bitsRead);

  BsFreeBuffer(h_arith_bb);


#ifdef NOISEFILLING_NO_BUGFIX
  /*
   * Inverse Quantization-Noise Filling and Scaling
   */
  if(hDec->bUseNoiseFilling){
    int g;
    noiseLevel_float = NoiseLevel(noise_level>>5);

    /* check if noise-filling should be inserted */
    for(g=0; g<info->num_groups; g++){
      for ( sfb = 0; sfb < (int)max_sfb; sfb++) {
        isNoise[g][sfb] = 1;
      }
    }

    offset = 0;
    sbk = 0;
    for(g=0; g<info->num_groups; g++){
      int gg;
      for(gg=0; gg<info->group_len[g]; gg++){
        for ( sfb = 0; sfb < (int)max_sfb; sfb++) {
          int startLine =0;

          if (sfb>0) {
            startLine = info->sbk_sfb_top[sbk][sfb-1];
          }
          else {
            startLine = 0;
          }

          for (ind = startLine; ind < info->sbk_sfb_top[sbk][sfb]; ind++) {
            if(quant[offset + ind] != 0) {
              isNoise[g][sfb] = 0;
              break;
            }
          }
        }
        offset += info->bins_per_sbk[sbk];
        sbk++;
      }
    }
  }

  /* rescaling */
  group = 0;
  groupSbkOffset = 0;
  offset = 0;
  

#ifdef BUGFIX_MAX_SFB
  memset(coef, 0, sizeof(float) * 2048);
#endif
  for (sbk=0; sbk<nsbk; sbk++) {

    if(info->group_len[group] + groupSbkOffset < sbk+1){
      groupSbkOffset += info->group_len[group];
      group++;
      groupWin = 0;
    }

    for ( sfb = 0; sfb < (int)max_sfb; sfb++) {
      int startLine =0;


      if (sfb>0) {
        startLine = info->sbk_sfb_top[sbk][sfb-1];
      }
      else {
        startLine = 0;
      }


      fac = factors[sfbOffset + sfb]-SF_OFFSET;

      if(hDec->bUseNoiseFilling){
        int   noiseOffset = (hDec->blockSize == 768)?
          (info->islong?120:15):
          (info->islong?160:20);
        noiseLevelTmp = noiseLevel_float;

        if(startLine < noiseOffset){
          noiseLevelTmp = 0.f;
        }
        if( isNoise[group][sfb] ) {
          fac += (((int)(noise_level & 0x1f))-16);
        }
      }
      fac = (float) pow(2.0F, 0.25F * fac);
      
      for (ind = startLine; ind < info->sbk_sfb_top[sbk][sfb]; ind++) {
        coef[offset + ind] = esc_iquant(quant[offset + ind],noiseLevelTmp,hDec->bUseNoiseFilling, nfSeed)*fac;
      }
    }
    sfbOffset += info->sfb_per_sbk[sbk];
#ifdef BUGFIX_MAX_SFB
    offset += info->bins_per_sbk[sbk];
#else
    for ( ; ind < info->bins_per_sbk[sbk] ; ind++) {
      coef[offset + ind] = 0.0F;
    }
    offset += ind;
#endif
    groupWin++;
  }


#else /* #ifdef NOISEFILLING_NO_BUGFIX */

  applyScaleFactorsAndNf(bUseNoiseFilling,
                         hDec->blockSize,
                         info,
                         coef,
                         quant,
                         noise_level,
                         factors,
                         max_sfb,
                         nfSeed);



#endif /* NOISEFILLING_NO_BUGFIX */
  return ;
}


#ifndef NOISEFILLING_NO_BUGFIX
static void applyScaleFactorsAndNf(int bNoiseFillingActive,
                                   int blockSize,
                                   Info* info,
                                   float* coef,
                                   int* quant,
                                   int noise_level,
                                   short* factors,
                                   byte max_sfb,
                                   unsigned int* nfSeed){

  int grp = 0, grpwin = 0, win_tot = 0, sfb = 0;
  float noiseLevel = 0.f;
  int noiseOffset = 0;
  float fac = 0.f;

  if(bNoiseFillingActive){
    noiseLevel = NoiseLevel(noise_level>>5);    
    noiseOffset = (blockSize == 768)?
      (info->islong?120:15):
      (info->islong?160:20);
  }


  memset(coef, 0, sizeof(float) * 2048);
  

  for(grp = 0; grp < info->num_groups; grp++){
    int groupwin = 0;
    for(sfb = 0; sfb < (int)max_sfb; sfb++){
      int bUseNoiseFilling = 0;
      int bandIsNoise = 1; 

      int sfb_offset = win_tot * info->sfb_per_sbk[win_tot];
      fac = (float)(factors[sfb_offset + sfb] - SF_OFFSET);
      
      if(bNoiseFillingActive){
        for(groupwin = 0; groupwin < info->group_len[grp]; groupwin++){
          int win = groupwin + win_tot;
          int offset = win * info->bins_per_sbk[win];
          int start = (sfb == 0) ? 0 : info->sbk_sfb_top[win][sfb - 1];
          int idx = 0;
          for(idx = start; idx < info->sbk_sfb_top[win][sfb]; idx++){
            if(quant[offset + idx] != 0){
              bandIsNoise = 0;
              break;
            }
            if(!bandIsNoise) break;
          }
          
        }
      }

      for(groupwin = 0; groupwin < info->group_len[grp]; groupwin++){
        int idx = 0;
        int win = win_tot + groupwin;
        int start = (sfb == 0) ? 0 : info->sbk_sfb_top[win][sfb - 1];
        int offset = win * info->bins_per_sbk[win];

        if(bNoiseFillingActive){
          bUseNoiseFilling = (start >= noiseOffset) & bNoiseFillingActive;       
        
          /* apply only if all coefs are 0 */
          if(bandIsNoise && (groupwin == 0)){
            fac += (((int)(noise_level & 0x1f))-16);
          }
        }
        if(groupwin == 0){
          fac = (float)pow(2.f, 0.25f * fac);
        }

        for(idx = start; idx < info->sbk_sfb_top[win][sfb]; idx++){
          coef[offset + idx] = esc_iquant(quant[offset + idx], 
                                          noiseLevel,
                                          bUseNoiseFilling, 
                                          nfSeed);          

          coef[offset + idx] *= fac;
        }
       

        
      } /* loop over group windows */

    } /* loop over sfbs */

    win_tot += info->group_len[grp];

  } /* loop over groups */
  

  return;
}
#endif /* #ifndef NOISEFILLING_NO_BUGFIX */ 


/*-----------------------------------------------------------------------------
  functionname: aencSpecFrame
  description:  AAC arithmetic encoding
  input params:
  returns:
  ------------------------------------------------------------------------------*/
int aencSpecFrame(HANDLE_BSBITSTREAM       bs_data,
		  WINDOW_SEQUENCE          windowSequence,
		  int                      nlong,
		  int                      *quantSpectrum,
		  int                      max_spec_coefficients,
		  Tqi2                     arithQ[],
		  int                      *arithPreviousSize,
		  int                      reset){
  int i,w;
  int write_flag = (bs_data!=NULL);
  Tqi2 arithQTmp[(1024/2)+4];
  Tqi2 qn[(1024/2)+4];
  int size;
  int nWindows = (windowSequence == EIGHT_SHORT_SEQUENCE)?8:1;
  int resetFlag=0;
  int bitCount=0;
  int bitCountData=0;
  unsigned char arith_bb[6144] = {0};


  switch( windowSequence ) {
  case ONLY_LONG_SEQUENCE :
  case LONG_START_SEQUENCE :
  case STOP_START_SEQUENCE:
  case LONG_STOP_SEQUENCE :  
    size=nlong; 
    break;
  case EIGHT_SHORT_SEQUENCE : 
    size=nlong/8;
    break;
  default :
    CommonExit( 1, "aencSpecFrame: Unknown window type(1)" );
  }

  /* copy context to tmp buffer */
  memcpy(arithQTmp, arithQ, sizeof(arithQTmp));

  if(reset){
    resetFlag=1;
    for(i=0;i<(1024/2)+4;i++){
      arithQTmp[i] = q_unknown;
    }    
  }
  else { /*Mapping of the context*/
    memcpy(arithQTmp, arithQ, sizeof(arithQTmp));
    map_context(size, *arithPreviousSize, arithQTmp);
  }  

  /*Arithmetic encoding*/
  for(w=0; w<nWindows; w++){
    for(i=0; i<size; i+=2){
      qn[(w*size + i)/2].a = quantSpectrum[w*size + i+0];
      qn[(w*size + i)/2].b = quantSpectrum[w*size + i+1];
    }
    
    if(max_spec_coefficients/2 > size/2){
      CommonWarning("aencSpecFrame(): too many spectral coeffs");
      exit(1);
    }
    
    if(max_spec_coefficients > 0){
      bitCountData = encode2(arith_bb,bitCountData,arithQTmp+2,&qn[w*size/2],max_spec_coefficients/2,size/2,2);
    }
  } 

  /* write arithmetic coding data */
  for(i=0;i<bitCountData-7;i+=8){
    if(write_flag) BsPutBit(bs_data, arith_bb[i/8], 8);
    bitCount+=8;
  }
  /* take care of remaining bits... */
  if(write_flag) BsPutBit(bs_data, arith_bb[i/8] >> (8-(bitCountData%8)), bitCountData%8);
  bitCount+=bitCountData%8;
  

  /* Update context*/
  if(write_flag){
    *arithPreviousSize = size;
    memcpy(arithQ, arithQTmp, sizeof(arithQTmp));
  }

  return bitCount;
}


/*-----------------------------------------------------------------------------
  functionname: tcxArithDec
  description:  TCX arithmetic decoding to be called from dec_prm.c of AMR-WB+
  input params:
  returns:
  ------------------------------------------------------------------------------*/
int tcxArithDec(int                      tcx_size,
                int*                     quant,
                int                      *arithPreviousSize,
                Tqi2                     arithQ[],
                HANDLE_BUFFER            hVm,
                int 			 reset)
{
  Tqi2	              arithQTmp[(1024/2)+4];
  int                 bitsRead = 0;
  int                 i;
  HANDLE_BSBITBUFFER  h_arith_bb = GetRemainingBufferBits (hVm);
  unsigned char*      arith_bb = BsBufferGetDataBegin ( h_arith_bb );

  /*Initialization*/
  intclr(quant, tcx_size);

  if(reset){
    for(i=0;i<(1024/2)+4;i++) {
      arithQ[i]=q_unknown;
      arithQTmp[i]=q_unknown;      
    }
  }
  else {
    map_context(tcx_size,*arithPreviousSize,arithQ);
  }
  *arithPreviousSize = tcx_size;

  /*Decoding*/
  bitsRead = decode2(arith_bb, bitsRead,arithQ+2,arithQTmp,tcx_size/2,tcx_size/2);

  /* copy data from arithQ to quant */
  for(i=0; i<tcx_size/2; i++){
    quant[2*i + 0] = arithQTmp[i].a;
    quant[2*i + 1] = arithQTmp[i].b;
  }

  SkipBits(hVm, bitsRead);
  BsFreeBuffer(h_arith_bb);

  return(0);
}


/*-----------------------------------------------------------------------------
  functionname: tcxArithEnc
  description:  TCX arithmetic encoding
  input params:
  returns:
  ------------------------------------------------------------------------------*/
int tcxArithEnc(int      tcx_size,
                int      max_tcx_size,
		int      *quantSpectrum,
		Tqi2     arithQ[],
		int      update_flag,
		unsigned char bitBuffer[])
{
  int i;
  int bitCount = 0;
  Tqi2 qn[(1024/2)+4];
  Tqi2 arithQTmp[(1024/2)+4];

  /* copy context to tmp buffer */
  memcpy(arithQTmp, arithQ, sizeof(arithQTmp));

  /* map context */
  map_context(tcx_size, max_tcx_size, arithQTmp);
 
  /* Generate the quadruples */  
  for(i=0; i<tcx_size; i+=2){
    qn[i/2].a = quantSpectrum[i + 0];
    qn[i/2].b = quantSpectrum[i + 1];
  }
  
  /* Arithmetic encoding */
  bitCount = encode2(bitBuffer, bitCount, arithQTmp+2, &qn[0], tcx_size/2, tcx_size/2, 2);
    
  /*Re-mapping of the context*/
  if(update_flag){
    map_context(max_tcx_size, tcx_size, arithQTmp);
    memcpy(arithQ, arithQTmp, sizeof(arithQTmp));
  }
  
  return bitCount;
}

/*-----------------------------------------------------------------------------
  functionname: tcxArithreset
  description:  reset TCX arithmetic context
  input params:
  returns:
  ------------------------------------------------------------------------------*/
int tcxArithReset(Tqi2 arithQ[])
{
  int i;

  for(i=0;i<(1024/2)+4;i++)
    arithQ[i] = q_unknown;

  return 0;
}

static int esc_nb_offset[8] = {0,131072,262144,393216,524288,655360,786432,917504};

/* local function definitions*/
static long decode2(unsigned char *bbuf,long bp,Tqi2 q[],Tqi2 *qn,long n, long nt){
  Tastat as;
  long	 a, b;
  long	 s, i, l, lev, pki, esc_nb;
  int	 r;
  long   state_inc = 0;
  
  for (i=0; i<nt; i++){
    q[i].a=0;
    q[i].b=0;
    q[i].c_prev = q[i].c;
    q[i].c=1; 
  }
  
  bp = ari_start_decoding(bbuf, bp, &as);
  
  init_state(q,&state_inc);
  lev = 0;
  
  for (i=0; i<n; i++){
    
    s = get_state(q+i, &state_inc, i);
    
    for (lev=esc_nb=0;;){
      
      pki = get_pk(s+esc_nb_offset[esc_nb]);  
      bp  = ari_decode(bbuf, bp, &r, &as, ari_pk[pki], Val_esc+1);
      
      if(r < Val_esc){
        break;
      }      
      
      lev+=1;            
      esc_nb=lev;      

      if(esc_nb > 7){
        esc_nb=7;
      }
    }
    
    if(r==0){             
      if(esc_nb>0) break; 
      
      q[i].a=0;
      q[i].b=0;
      q[i].c=1;      
    }
    else{
      b = r>>2;      
      a = r&0x3;     
      
      for (l=0; l<lev; l++){
	

        int	pidx=(a==0)?1:((b==0)?0:2);
        bp=ari_decode(bbuf,bp,&r,&as,ari_pk_4[pidx],4);

        a=(a<<1)|( r    &1);  
        b=(b<<1)|((r>>1)&1);
      }
      q[i].a=a;
      q[i].b=b;
      q[i].c=a+b+1;    
      if(q[i].c>0xF){
        q[i].c=0xF;
      }
    }
  }
  
  bp-=cbits-2;
  
  for(i=0; i<nt; i++){
    
    if( q[i].a ) {    
      
      r = ((bbuf[bp>>3]>>(7-(bp&7)))&1);  
      bp++;
      if( !r ){
        q[i].a = -(q[i].a);
      }
    }
    
    if( q[i].b ) {
      
      r = ((bbuf[bp>>3]>>(7-(bp&7)))&1);
      bp++;
      if( !r ){
        q[i].b = -(q[i].b);
      }
    }
    qn[i].a = q[i].a;
    qn[i].b = q[i].b;     
    qn[i].c = q[i].c;     
  } 
  
  return bp;
}



static long encode2(unsigned char *bbuf,long bp,Tqi2 q[],Tqi2 *qn,long n,long nt,int use_stop)
{
  int		qs[32];
  Tastat	as,as_stop;
  
  long	a, b, a1 , b1;
  long	s, t, i, l, pki, lev1, esc_nb;
  uchar bbuf_temp[6144]={0};
  long  bp_start = bp;
  long  bp_stop  = bp;
  int   stop = 0;
  long  sopt;
  int	a2,b2;
  
  
  /*Start Encoding*/
  ari_start_encoding(&as);
  init_state(q, &sopt);
  
  /*Main Loop through the 2-tuples*/
  for (i=0; i<n; i++){
    
    /*STOP symbol detection*/
    if((use_stop==1 || use_stop==2) && (stop==0)){
      int j;
      
      stop=1;
      for (j=i; j<n; j++){
	if(qn[j].a!=0 || qn[j].b!=0){
	  stop=0;
	  break;
	}
      }

      if(stop){
    	s = get_state(q+i, &sopt, i);
    	t = s&0xFFFFF;
        
        pki=get_pk(t);
        
	if(use_stop==1){/* do the real coding */
	  /*send exc symbol*/
          bp=ari_encode(bbuf,bp,&as,Val_esc,ari_pk[pki]);
          pki=get_pk(t+(1<<17));
          bp=ari_encode(bbuf,bp,&as,0,ari_pk[pki]);
	  
	  break;
	}
	else{ /*simulate the coding*/
          bp_stop = bp;
          ari_copy_states(&as, &as_stop);
          
          /*send exc symbol*/
          bp_stop = ari_encode(bbuf_temp,bp_stop,&as_stop,Val_esc,ari_pk[pki]);
          
          /*send 0*/
          pki = get_pk(t+(1<<17));
          bp_stop = ari_encode(bbuf_temp,bp_stop,&as_stop,(0),ari_pk[pki]);
	}
	
      }
    }      
    s=get_state(q+i, &sopt, i);
    t=s&0xFFFFF;

    a=qn[i].a;
    b=qn[i].b;
    a1=abs(a);
    b1=abs(b);
    
    q[i].a=a;
    q[i].b=b;
    
    q[i].c=a1+b1+1;
    if(q[i].c>0xF){
      q[i].c=0xF; 
    }
    
    lev1 = 0;
    esc_nb=0;
    
    while((a1) > A_thres || (b1) > B_thres){
      pki = get_pk(t+(esc_nb<<17));
      
      bp = ari_encode(bbuf, bp, &as, Val_esc, ari_pk[pki]);
      
      qs[lev1++]=(a1&1)|((b1&1)<<1);
      (a1)>>=1;
      (b1)>>=1;
      esc_nb++;
      
      if(esc_nb>7){
        esc_nb=7;
      }
    }    
    
    pki=get_pk(t+(esc_nb<<17));

    bp = ari_encode(bbuf, bp, &as, ((a1)+((A_thres+1)*(b1))), ari_pk[pki]);
    
    a2 = a1;
    b2 = b1;
    
    
    for (l=lev1-1; l>=0; l--){         
      
      int pidx=(a2==0)?1:((b2==0)?0:2);
      bp = ari_encode(bbuf, bp, &as, qs[l], ari_pk_4[pidx]);   
      
      a2 = (a2<<1)|(qs[l]&1);
      b2 = (b2<<1)|((qs[l]>>1)&1);
      
    }
  }
  
  if(use_stop==2){
    bp=ari_done_encoding(bbuf,bp,&as);
    if(stop){
      bp_stop=ari_done_encoding(bbuf_temp,bp_stop,&as_stop);
      
      /*Redo coding according to the method selection*/
      if(bp_stop<bp){ 
        bp=encode2(bbuf,bp_start,q,qn,n,nt,1);
      } else {
        bp=encode2(bbuf,bp_start,q,qn,n,nt,0);
      }
    } else {
      bp=encode2(bbuf,bp_start,q,qn,n,nt,0);
    } 
  }
  else{
    bp=ari_done_encoding(bbuf,bp,&as);
    
    for (; i<nt; i++){
      q[i] = qn[i];
      q[i].c = 1;
    }
    
    for (i=0; i<n; i++){
        
      /*Coding of Sign*/
      if(q[i].a != 0) {
        if(q[i].a > 0) {
          bbuf[bp>>3]|=(128>>(bp&7));
          bp++;
        } else  {
          bbuf[bp>>3]&=~(128>>(bp&7));
          bp++;  
        } 
      }
      
      if(q[i].b != 0) {
        if(q[i].b > 0) {
          bbuf[bp>>3]|=(128>>(bp&7));
          bp++;
        } else  {
          bbuf[bp>>3]&=~(128>>(bp&7));
          bp++;  
        } 
      }
    }
    
    for (i=0; i<nt; i++){
      q[i].a = 0;
      q[i].b = 0;
      q[i].c_prev = q[i].c;
      q[i].c = 1;
    }
  }
  
  return bp;
}


static void map_context(int size,int previousSize,Tqi2 arithQ[])
{
  int i,k;
  float ratio;
  Tqi2 tmp[1024/2+4];
  
  for(i=2; i <(previousSize/2+4); i++)
    tmp[i] = arithQ[i];
  
  
  ratio = (float)(previousSize)/(float)(size);
  for(i=0; i<(size/2); i++){
    k = (int)((float)(i)*ratio);
    arithQ[2+i] = tmp[2+k];
  }

  arithQ[(size/2)+2] = tmp[(previousSize/2)+2];
  arithQ[(size/2)+3] = tmp[(previousSize/2)+3];
}

static void init_state(Tqi2 q[], long *s){
  
  *s=q[0].c_prev<<12;
  
  return;
}


static long get_state(Tqi2 q[], long *s, int bin)
{
  long s_tmp = *s;
  
  /*Build context state on 16 bits*/
  s_tmp = s_tmp>>4;                      /* Shift old 4 bits */
  s_tmp = s_tmp + (q[1].c_prev<<12);     /* add new 4 bits */
  s_tmp = (s_tmp&0xFFF0) + q[-1].c; /* replace last 4 bits */
  
  *s = s_tmp;
  
  if(bin > 3) {
    /*Cumulative amplitude below 2*/
    if( ( q[-1].c + q[-2].c + q[-3].c ) < 5){
      return(s_tmp+0x10000); 
    }
  }
  
  return(s_tmp);
}

static long ari_start_decoding(unsigned char *buf,long bp,Tastat *s)
{
  register long	val;
  register long	i;

  val = 0;
  for (i=1; i<=cbits; i++)
    {
      val = (val<<1) | ((buf[bp>>3]>>(7-(bp&7)))&1);
      bp++;
    }
  s->low = 0;
  s->high = ari_q4;
  s->vobf=val;

  return bp;
}

static void ari_start_encoding(Tastat *s)
{
  s->low  = 0;
  s->high = ari_q4;
  s->vobf = 0;
}

static long ari_decode(unsigned char *buf,
                       long bp,
                       int *res,
                       Tastat *s,
                       unsigned short const *cum_freq,
                       long cfl)
{
  register long	symbol;
  register long	low, high, range, value;
  register long	cum;
  register unsigned short const	*p;
  register unsigned short const *q;

  low = s->low;
  high = s->high;
  value = s->vobf;

  range = high-low+1;
  cum =((((int) (value-low+1))<<stat_bits)-((int) 1))/((int) range);

  p = cum_freq-1;

  do
    {
      q=p+(cfl>>1);
    
      if ( *q > cum ) { p=q; cfl++; }
      cfl>>=1;
    }
  while ( cfl>1 );

  symbol = p-cum_freq+1;

  if ( symbol ) high  = low + mul_sbc(range,cum_freq[symbol-1]) - 1;

  low  += mul_sbc(range,cum_freq[symbol]);

  for (;;)
    {
      if ( high<ari_q2 )
        {
        }
      else if ( low>=ari_q2 )
        {
          value -= ari_q2;
          low -= ari_q2;
          high -= ari_q2;
        }
      else if ( low>=ari_q1 && high<ari_q3 )
        {
          value -= ari_q1;
          low -= ari_q1;
          high -= ari_q1;
        }
      else break;

      low += low;
      high += high+1;

      value = (value<<1) | ((buf[bp>>3]>>(7-(bp&7)))&1);
      bp++;
    }

  s->low = low;
  s->high = high;
  s->vobf = value;

  *res=symbol;

  return bp;
}

static long ari_encode(unsigned char *buf,long bp,Tastat *s,long symbol,unsigned short const *cum_freq)
{
  register long low, high, range;
  register long	bits_to_follow;

  high=s->high;
  low =s->low;
  range = high-low+1;

  if ( symbol ) high  = low + mul_sbc(range,cum_freq[symbol-1]) - 1;
	
  low  += mul_sbc(range,cum_freq[symbol]);

  bits_to_follow = s->vobf;

  for (;;)
    {
      if ( high<ari_q2 )
        {
          buf[bp>>3]&=~(128>>(bp&7));
          bp++;
          while ( bits_to_follow )
            {
              buf[bp>>3]|=(128>>(bp&7));
              bp++;
              bits_to_follow--;
            }
        }
      else if ( low>=ari_q2 )
        {
          buf[bp>>3]|=(128>>(bp&7));
          bp++;
          while ( bits_to_follow )
            {
              buf[bp>>3]&=~(128>>(bp&7));
              bp++;
              bits_to_follow--;
            }
          low -= ari_q2;
          high -= ari_q2;				/* Subtract offset to top.  */
        }
      else if ( low>=ari_q1 && high<ari_q3 )		/* Output an opposite bit   */
        {						/* later if in middle half. */
          bits_to_follow += 1;
          low -= ari_q1;				/* Subtract offset to middle*/
          high -= ari_q1;
        }
      else break;					/* Otherwise exit loop.     */

      low += low;
      high += high+1;					/* Scale up code range.     */
    }

  s->low  = low;
  s->high = high; 
  s->vobf = bits_to_follow;
  
  return bp;
}


static long ari_done_encoding(unsigned char *buf,long bp,Tastat *s)
{
  register long	low, high;
  register long	bits_to_follow;

  low = s->low;
  high = s->high; 
  bits_to_follow = s->vobf+1;

  if ( low < ari_q1 )
    {
      buf[bp>>3]&=~(128>>(bp&7));
      bp++;
      while ( bits_to_follow )
        {
          buf[bp>>3]|=(128>>(bp&7));
          bp++;
          bits_to_follow--;
        }
    }
  else
    {
      buf[bp>>3]|=(128>>(bp&7));
      bp++;
      while ( bits_to_follow )
        {
          buf[bp>>3]&=~(128>>(bp&7));
          bp++;
          bits_to_follow--;
        }
    }

  s->low = low;
  s->high = high;
  s->vobf = bits_to_follow;

  return bp;
}


static long mul_sbc(long r,long c)
{
  return (((int) r)*((int) c))>>stat_bits;
}

/* Noise level*/
static float NoiseLevel(int noiseLevel)
{
  float noiseLevelFloat = 0.f;

  if (7<noiseLevel) {
    CommonWarning( "noiseLevel out of range, restricting");
    noiseLevel = 7;
  }

  if (0<noiseLevel) {
    noiseLevelFloat = (float)pow(2.f, (noiseLevel-14.f)/3.f );
  }
  
  return noiseLevelFloat;
}

#ifdef UNIFIED_RANDOMSIGN
static float randomSign(unsigned int *seed)
{
  float sign = 0.f;
  *seed = ((*seed) * 69069) + 5;
  if ( ((*seed) & 0x10000) > 0) {
    sign = -1.f;
  } else {
    sign = +1.f;
  }
  return sign;
}
#else
static unsigned int nfRand( unsigned int *seed )
{
  *seed = ((*seed) * 69069) + 5;

  return(unsigned int)(*seed);
}
#endif


static float esc_iquant(int q, float noiseLevel , int withNoise, unsigned int *nfSeed)
{
  
  if (withNoise) {
    if ( q == 0 ) {
    
#ifdef UNIFIED_RANDOMSIGN

      return randomSign(nfSeed) * noiseLevel;

#else
      float sign = 0.f;
      unsigned int tmp = nfRand(nfSeed)&0x10000;
      sign = (tmp>0)?(-1.f):(+1.f);
      return sign * noiseLevel;
#endif
    }
  }
  
  if (q > 0) {
    if (q >= 8192)
      {
        q = 8191;
      }
    if (q < iq_table_size) {
      return(iq_table[q]);
    }
    else {
      return((float)pow((float)q, 4.0f/3.0f));
    }
  }
  else {
    q = -q;
    if (q < iq_table_size) {
      return((-iq_table[q]));
    }
    else {
      return((float)-pow((float)q, 4.0f/3.0f));
    }
  }
}

static unsigned long get_pk(unsigned long s)
{
  unsigned long j;
  long  i, i_min, i_max;
  
  i_min=-1;
  i=i_min;
  i_max = (sizeof(ari_merged_hash)/sizeof(ari_merged_hash[0]))-1;
  while((i_max-i_min)>1){
    i=i_min+((i_max-i_min)/2);
    j=ari_merged_hash[i];
    if(s<(j>>8))
      i_max=i;
    else if(s>(j>>8))
      i_min=i;
    else
      return(j&0xFF);
  }
  
  return(ari_merged_ps[i_max]);
}


static void ari_copy_states(Tastat *source, Tastat *dest)
{
  dest->low  = source->low;
  dest->high = source->high;
  dest->vobf = source->vobf;
}
