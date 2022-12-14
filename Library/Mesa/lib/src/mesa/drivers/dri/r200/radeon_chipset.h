#ifndef _RADEON_CHIPSET_H
#define _RADEON_CHIPSET_H
/* Including xf86PciInfo.h introduces a bunch of errors...
 */

/* General chip classes:
 * r100 includes R100, RV100, RV200, RS100, RS200, RS250.
 * r200 includes R200, RV250, RV280, RS300.
 * r300 includes R300, RV350, RV370.
 * (RS* denotes IGP)
 */
#define PCI_CHIP_RV380_3150		0x3150
#define PCI_CHIP_RV380_3152		0x3152
#define PCI_CHIP_RV380_3154		0x3154
#define PCI_CHIP_RV380_3E50		0x3E50
#define PCI_CHIP_RV380_3E54		0x3E54
#define PCI_CHIP_RS100_4136		0x4136
#define PCI_CHIP_RS200_4137		0x4137
#define PCI_CHIP_R300_AD		0x4144
#define PCI_CHIP_R300_AE		0x4145
#define PCI_CHIP_R300_AF		0x4146
#define PCI_CHIP_R300_AG		0x4147
#define PCI_CHIP_R350_AH                0x4148
#define PCI_CHIP_R350_AI                0x4149
#define PCI_CHIP_R350_AJ                0x414A
#define PCI_CHIP_R350_AK                0x414B
#define PCI_CHIP_RV350_AP               0x4150
#define PCI_CHIP_RV350_AQ               0x4151
#define PCI_CHIP_RV350_AR               0x4152
#define PCI_CHIP_RV350_AS               0x4153
#define PCI_CHIP_RV350_AT               0x4154
#define PCI_CHIP_RV350_AU		0x4155
#define PCI_CHIP_RV350_AV               0x4156
#define PCI_CHIP_RS250_4237		0x4237
#define PCI_CHIP_R200_BB		0x4242
#define PCI_CHIP_R200_BC		0x4243
#define PCI_CHIP_RS100_4336		0x4336
#define PCI_CHIP_RS200_4337		0x4337
#define PCI_CHIP_RS250_4437		0x4437
#define PCI_CHIP_RV250_If		0x4966
#define PCI_CHIP_RV250_Ig		0x4967
#define PCI_CHIP_R420_JH		0x4A48
#define PCI_CHIP_R420_JI		0x4A49
#define PCI_CHIP_R420_JJ		0x4A4A
#define PCI_CHIP_R420_JK		0x4A4B
#define PCI_CHIP_R420_JL		0x4A4C
#define PCI_CHIP_R420_JM		0x4A4D
#define PCI_CHIP_R420_JN		0x4A4E
#define PCI_CHIP_R420_JO		0x4A4F
#define PCI_CHIP_R420_JP		0x4A50
#define PCI_CHIP_R420_JT		0x4A54
#define PCI_CHIP_R481_4B49		0x4B49
#define PCI_CHIP_R481_4B4A		0x4B4A
#define PCI_CHIP_R481_4B4B		0x4B4B
#define PCI_CHIP_R481_4B4C		0x4B4C
#define PCI_CHIP_RADEON_LW		0x4C57
#define PCI_CHIP_RADEON_LX		0x4C58
#define PCI_CHIP_RADEON_LY		0x4C59
#define PCI_CHIP_RADEON_LZ		0x4C5A
#define PCI_CHIP_RV250_Ld		0x4C64
#define PCI_CHIP_RV250_Lf		0x4C66
#define PCI_CHIP_RV250_Lg		0x4C67
#define PCI_CHIP_R300_ND		0x4E44
#define PCI_CHIP_R300_NE		0x4E45
#define PCI_CHIP_R300_NF		0x4E46
#define PCI_CHIP_R300_NG		0x4E47
#define PCI_CHIP_R350_NH                0x4E48
#define PCI_CHIP_R350_NI                0x4E49  
#define PCI_CHIP_R360_NJ                0x4E4A  
#define PCI_CHIP_R350_NK                0x4E4B  
#define PCI_CHIP_RV350_NP               0x4E50
#define PCI_CHIP_RV350_NQ               0x4E51
#define PCI_CHIP_RV350_NR               0x4E52
#define PCI_CHIP_RV350_NS               0x4E53
#define PCI_CHIP_RV350_NT               0x4E54
#define PCI_CHIP_RV350_NV               0x4E56
#define PCI_CHIP_RADEON_QD		0x5144
#define PCI_CHIP_RADEON_QE		0x5145
#define PCI_CHIP_RADEON_QF		0x5146
#define PCI_CHIP_RADEON_QG		0x5147
#define PCI_CHIP_R200_QH		0x5148
#define PCI_CHIP_R200_QL		0x514C
#define PCI_CHIP_R200_QM		0x514D
#define PCI_CHIP_RV200_QW		0x5157
#define PCI_CHIP_RV200_QX		0x5158
#define PCI_CHIP_RADEON_QY		0x5159
#define PCI_CHIP_RADEON_QZ		0x515A
#define PCI_CHIP_RN50_515E		0x515E
#define PCI_CHIP_RV370_5460		0x5460
#define PCI_CHIP_RV370_5462		0x5462
#define PCI_CHIP_RV370_5464		0x5464
#define PCI_CHIP_R423_UH		0x5548
#define PCI_CHIP_R423_UI		0x5549
#define PCI_CHIP_R423_UJ		0x554A
#define PCI_CHIP_R423_UK		0x554B
#define PCI_CHIP_R430_554C		0x554C
#define PCI_CHIP_R430_554D		0x554D
#define PCI_CHIP_R430_554E		0x554E
#define PCI_CHIP_R430_554F		0x554F
#define PCI_CHIP_R423_5550		0x5550
#define PCI_CHIP_R423_UQ		0x5551
#define PCI_CHIP_R423_UR		0x5552
#define PCI_CHIP_R423_UT		0x5554
#define PCI_CHIP_RV410_564A		0x564A
#define PCI_CHIP_RV410_564B		0x564B
#define PCI_CHIP_RV410_564F		0x564F
#define PCI_CHIP_RV410_5652		0x5652
#define PCI_CHIP_RV410_5653		0x5653
#define PCI_CHIP_RV410_5657		0x5657
#define PCI_CHIP_RS300_5834		0x5834
#define PCI_CHIP_RS300_5835		0x5835
#define PCI_CHIP_RS480_5954		0x5954
#define PCI_CHIP_RS480_5955		0x5955
#define PCI_CHIP_RV280_5960		0x5960
#define PCI_CHIP_RV280_5961		0x5961
#define PCI_CHIP_RV280_5962		0x5962
#define PCI_CHIP_RV280_5964		0x5964
#define PCI_CHIP_RV280_5965		0x5965
#define PCI_CHIP_RN50_5969		0x5969
#define PCI_CHIP_RS482_5974		0x5974
#define PCI_CHIP_RS482_5975		0x5975
#define PCI_CHIP_RS400_5A41		0x5A41
#define PCI_CHIP_RS400_5A42		0x5A42
#define PCI_CHIP_RC410_5A61		0x5A61
#define PCI_CHIP_RC410_5A62		0x5A62
#define PCI_CHIP_RV370_5B60		0x5B60
#define PCI_CHIP_RV370_5B62		0x5B62
#define PCI_CHIP_RV370_5B63		0x5B63
#define PCI_CHIP_RV370_5B64		0x5B64
#define PCI_CHIP_RV370_5B65		0x5B65
#define PCI_CHIP_RV280_5C61		0x5C61
#define PCI_CHIP_RV280_5C63		0x5C63
#define PCI_CHIP_R430_5D48		0x5D48
#define PCI_CHIP_R430_5D49		0x5D49
#define PCI_CHIP_R430_5D4A		0x5D4A
#define PCI_CHIP_R480_5D4C		0x5D4C
#define PCI_CHIP_R480_5D4D		0x5D4D
#define PCI_CHIP_R480_5D4E		0x5D4E
#define PCI_CHIP_R480_5D4F		0x5D4F
#define PCI_CHIP_R480_5D50		0x5D50
#define PCI_CHIP_R480_5D52		0x5D52
#define PCI_CHIP_R423_5D57		0x5D57
#define PCI_CHIP_RV410_5E48		0x5E48
#define PCI_CHIP_RV410_5E4A		0x5E4A
#define PCI_CHIP_RV410_5E4B		0x5E4B
#define PCI_CHIP_RV410_5E4C		0x5E4C
#define PCI_CHIP_RV410_5E4D		0x5E4D
#define PCI_CHIP_RV410_5E4F		0x5E4F

#define PCI_CHIP_R520_7100              0x7100
#define PCI_CHIP_R520_7101              0x7101
#define PCI_CHIP_R520_7102              0x7102
#define PCI_CHIP_R520_7103              0x7103
#define PCI_CHIP_R520_7104              0x7104
#define PCI_CHIP_R520_7105              0x7105
#define PCI_CHIP_R520_7106              0x7106
#define PCI_CHIP_R520_7108              0x7108
#define PCI_CHIP_R520_7109              0x7109
#define PCI_CHIP_R520_710A              0x710A
#define PCI_CHIP_R520_710B              0x710B
#define PCI_CHIP_R520_710C              0x710C
#define PCI_CHIP_R520_710E              0x710E
#define PCI_CHIP_R520_710F              0x710F
#define PCI_CHIP_RV515_7140             0x7140
#define PCI_CHIP_RV515_7141             0x7141
#define PCI_CHIP_RV515_7142             0x7142
#define PCI_CHIP_RV515_7143             0x7143
#define PCI_CHIP_RV515_7144             0x7144
#define PCI_CHIP_RV515_7145             0x7145
#define PCI_CHIP_RV515_7146             0x7146
#define PCI_CHIP_RV515_7147             0x7147
#define PCI_CHIP_RV515_7149             0x7149
#define PCI_CHIP_RV515_714A             0x714A
#define PCI_CHIP_RV515_714B             0x714B
#define PCI_CHIP_RV515_714C             0x714C
#define PCI_CHIP_RV515_714D             0x714D
#define PCI_CHIP_RV515_714E             0x714E
#define PCI_CHIP_RV515_714F             0x714F
#define PCI_CHIP_RV515_7151             0x7151
#define PCI_CHIP_RV515_7152             0x7152
#define PCI_CHIP_RV515_7153             0x7153
#define PCI_CHIP_RV515_715E             0x715E
#define PCI_CHIP_RV515_715F             0x715F
#define PCI_CHIP_RV515_7180             0x7180
#define PCI_CHIP_RV515_7181             0x7181
#define PCI_CHIP_RV515_7183             0x7183
#define PCI_CHIP_RV515_7186             0x7186
#define PCI_CHIP_RV515_7187             0x7187
#define PCI_CHIP_RV515_7188             0x7188
#define PCI_CHIP_RV515_718A             0x718A
#define PCI_CHIP_RV515_718B             0x718B
#define PCI_CHIP_RV515_718C             0x718C
#define PCI_CHIP_RV515_718D             0x718D
#define PCI_CHIP_RV515_718F             0x718F
#define PCI_CHIP_RV515_7193             0x7193
#define PCI_CHIP_RV515_7196             0x7196
#define PCI_CHIP_RV515_719B             0x719B
#define PCI_CHIP_RV515_719F             0x719F
#define PCI_CHIP_RV530_71C0             0x71C0
#define PCI_CHIP_RV530_71C1             0x71C1
#define PCI_CHIP_RV530_71C2             0x71C2
#define PCI_CHIP_RV530_71C3             0x71C3
#define PCI_CHIP_RV530_71C4             0x71C4
#define PCI_CHIP_RV530_71C5             0x71C5
#define PCI_CHIP_RV530_71C6             0x71C6
#define PCI_CHIP_RV530_71C7             0x71C7
#define PCI_CHIP_RV530_71CD             0x71CD
#define PCI_CHIP_RV530_71CE             0x71CE
#define PCI_CHIP_RV530_71D2             0x71D2
#define PCI_CHIP_RV530_71D4             0x71D4
#define PCI_CHIP_RV530_71D5             0x71D5
#define PCI_CHIP_RV530_71D6             0x71D6
#define PCI_CHIP_RV530_71DA             0x71DA
#define PCI_CHIP_RV530_71DE             0x71DE
#define PCI_CHIP_RV515_7200             0x7200
#define PCI_CHIP_RV515_7210             0x7210
#define PCI_CHIP_RV515_7211             0x7211
#define PCI_CHIP_R580_7240              0x7240
#define PCI_CHIP_R580_7243              0x7243
#define PCI_CHIP_R580_7244              0x7244
#define PCI_CHIP_R580_7245              0x7245
#define PCI_CHIP_R580_7246              0x7246
#define PCI_CHIP_R580_7247              0x7247
#define PCI_CHIP_R580_7248              0x7248
#define PCI_CHIP_R580_7249              0x7249
#define PCI_CHIP_R580_724A              0x724A
#define PCI_CHIP_R580_724B              0x724B
#define PCI_CHIP_R580_724C              0x724C
#define PCI_CHIP_R580_724D              0x724D
#define PCI_CHIP_R580_724E              0x724E
#define PCI_CHIP_R580_724F              0x724F
#define PCI_CHIP_RV570_7280             0x7280
#define PCI_CHIP_RV560_7281             0x7281
#define PCI_CHIP_RV560_7283             0x7283
#define PCI_CHIP_R580_7284              0x7284
#define PCI_CHIP_RV560_7287             0x7287
#define PCI_CHIP_RV570_7288             0x7288
#define PCI_CHIP_RV570_7289             0x7289
#define PCI_CHIP_RV570_728B             0x728B
#define PCI_CHIP_RV570_728C             0x728C
#define PCI_CHIP_RV560_7290             0x7290
#define PCI_CHIP_RV560_7291             0x7291
#define PCI_CHIP_RV560_7293             0x7293
#define PCI_CHIP_RV560_7297             0x7297

#define PCI_CHIP_RS350_7834		0x7834
#define PCI_CHIP_RS350_7835		0x7835
#define PCI_CHIP_RS690_791E             0x791E
#define PCI_CHIP_RS690_791F             0x791F
#define PCI_CHIP_RS600_793F             0x793F
#define PCI_CHIP_RS600_7941             0x7941
#define PCI_CHIP_RS600_7942             0x7942
#define PCI_CHIP_RS740_796C             0x796C
#define PCI_CHIP_RS740_796D             0x796D
#define PCI_CHIP_RS740_796E             0x796E
#define PCI_CHIP_RS740_796F             0x796F

#define PCI_CHIP_R600_9400              0x9400
#define PCI_CHIP_R600_9401              0x9401
#define PCI_CHIP_R600_9402              0x9402
#define PCI_CHIP_R600_9403              0x9403
#define PCI_CHIP_R600_9405              0x9405
#define PCI_CHIP_R600_940A              0x940A
#define PCI_CHIP_R600_940B              0x940B
#define PCI_CHIP_R600_940F              0x940F

#define PCI_CHIP_RV610_94C0             0x94C0
#define PCI_CHIP_RV610_94C1             0x94C1
#define PCI_CHIP_RV610_94C3             0x94C3
#define PCI_CHIP_RV610_94C4             0x94C4
#define PCI_CHIP_RV610_94C5             0x94C5
#define PCI_CHIP_RV610_94C6             0x94C6
#define PCI_CHIP_RV610_94C7             0x94C7
#define PCI_CHIP_RV610_94C8             0x94C8
#define PCI_CHIP_RV610_94C9             0x94C9
#define PCI_CHIP_RV610_94CB             0x94CB
#define PCI_CHIP_RV610_94CC             0x94CC
#define PCI_CHIP_RV610_94CD             0x94CD

#define PCI_CHIP_RV630_9580             0x9580
#define PCI_CHIP_RV630_9581             0x9581
#define PCI_CHIP_RV630_9583             0x9583
#define PCI_CHIP_RV630_9586             0x9586
#define PCI_CHIP_RV630_9587             0x9587
#define PCI_CHIP_RV630_9588             0x9588
#define PCI_CHIP_RV630_9589             0x9589
#define PCI_CHIP_RV630_958A             0x958A
#define PCI_CHIP_RV630_958B             0x958B
#define PCI_CHIP_RV630_958C             0x958C
#define PCI_CHIP_RV630_958D             0x958D
#define PCI_CHIP_RV630_958E             0x958E
#define PCI_CHIP_RV630_958F             0x958F

#define PCI_CHIP_RV670_9500             0x9500
#define PCI_CHIP_RV670_9501             0x9501
#define PCI_CHIP_RV670_9504             0x9504
#define PCI_CHIP_RV670_9505             0x9505
#define PCI_CHIP_RV670_9506             0x9506
#define PCI_CHIP_RV670_9507             0x9507
#define PCI_CHIP_RV670_9508             0x9508
#define PCI_CHIP_RV670_9509             0x9509
#define PCI_CHIP_RV670_950F             0x950F
#define PCI_CHIP_RV670_9511             0x9511
#define PCI_CHIP_RV670_9515             0x9515
#define PCI_CHIP_RV670_9517             0x9517
#define PCI_CHIP_RV670_9519             0x9519

#define PCI_CHIP_RV620_95C0             0x95C0
#define PCI_CHIP_RV620_95C2             0x95C2
#define PCI_CHIP_RV620_95C4             0x95C4
#define PCI_CHIP_RV620_95C5             0x95C5
#define PCI_CHIP_RV620_95C6             0x95C6
#define PCI_CHIP_RV620_95C7             0x95C7
#define PCI_CHIP_RV620_95C9             0x95C9
#define PCI_CHIP_RV620_95CC             0x95CC
#define PCI_CHIP_RV620_95CD             0x95CD
#define PCI_CHIP_RV620_95CE             0x95CE
#define PCI_CHIP_RV620_95CF             0x95CF

#define PCI_CHIP_RV635_9590             0x9590
#define PCI_CHIP_RV635_9591             0x9591
#define PCI_CHIP_RV635_9593             0x9593
#define PCI_CHIP_RV635_9595             0x9595
#define PCI_CHIP_RV635_9596             0x9596
#define PCI_CHIP_RV635_9597             0x9597
#define PCI_CHIP_RV635_9598             0x9598
#define PCI_CHIP_RV635_9599             0x9599
#define PCI_CHIP_RV635_959B             0x959B

#define PCI_CHIP_RS780_9610             0x9610
#define PCI_CHIP_RS780_9611             0x9611
#define PCI_CHIP_RS780_9612             0x9612
#define PCI_CHIP_RS780_9613             0x9613
#define PCI_CHIP_RS780_9614             0x9614
#define PCI_CHIP_RS780_9615             0x9615
#define PCI_CHIP_RS780_9616             0x9616

#define PCI_CHIP_RS880_9710             0x9710
#define PCI_CHIP_RS880_9711             0x9711
#define PCI_CHIP_RS880_9712             0x9712
#define PCI_CHIP_RS880_9713             0x9713
#define PCI_CHIP_RS880_9714             0x9714
#define PCI_CHIP_RS880_9715             0x9715

#define PCI_CHIP_RV770_9440             0x9440
#define PCI_CHIP_RV770_9441             0x9441
#define PCI_CHIP_RV770_9442             0x9442
#define PCI_CHIP_RV770_9443             0x9443
#define PCI_CHIP_RV770_9444             0x9444
#define PCI_CHIP_RV770_9446             0x9446
#define PCI_CHIP_RV770_944A             0x944A
#define PCI_CHIP_RV770_944B             0x944B
#define PCI_CHIP_RV770_944C             0x944C
#define PCI_CHIP_RV770_944E             0x944E
#define PCI_CHIP_RV770_9450             0x9450
#define PCI_CHIP_RV770_9452             0x9452
#define PCI_CHIP_RV770_9456             0x9456
#define PCI_CHIP_RV770_945A             0x945A
#define PCI_CHIP_RV770_945B             0x945B
#define PCI_CHIP_RV790_9460             0x9460
#define PCI_CHIP_RV790_9462             0x9462
#define PCI_CHIP_RV770_946A             0x946A
#define PCI_CHIP_RV770_946B             0x946B
#define PCI_CHIP_RV770_947A             0x947A
#define PCI_CHIP_RV770_947B             0x947B

#define PCI_CHIP_RV730_9480             0x9480
#define PCI_CHIP_RV730_9487             0x9487
#define PCI_CHIP_RV730_9488             0x9488
#define PCI_CHIP_RV730_9489             0x9489
#define PCI_CHIP_RV730_948F             0x948F
#define PCI_CHIP_RV730_9490             0x9490
#define PCI_CHIP_RV730_9491             0x9491
#define PCI_CHIP_RV730_9495             0x9495
#define PCI_CHIP_RV730_9498             0x9498
#define PCI_CHIP_RV730_949C             0x949C
#define PCI_CHIP_RV730_949E             0x949E
#define PCI_CHIP_RV730_949F             0x949F

#define PCI_CHIP_RV710_9540             0x9540
#define PCI_CHIP_RV710_9541             0x9541
#define PCI_CHIP_RV710_9542             0x9542
#define PCI_CHIP_RV710_954E             0x954E
#define PCI_CHIP_RV710_954F             0x954F
#define PCI_CHIP_RV710_9552             0x9552
#define PCI_CHIP_RV710_9553             0x9553
#define PCI_CHIP_RV710_9555             0x9555
#define PCI_CHIP_RV710_9557             0x9557

#define PCI_CHIP_RV740_94A0             0x94A0
#define PCI_CHIP_RV740_94A1             0x94A1
#define PCI_CHIP_RV740_94A3             0x94A3
#define PCI_CHIP_RV740_94B1             0x94B1
#define PCI_CHIP_RV740_94B3             0x94B3
#define PCI_CHIP_RV740_94B4             0x94B4
#define PCI_CHIP_RV740_94B5             0x94B5
#define PCI_CHIP_RV740_94B9             0x94B9

enum {
   CHIP_FAMILY_R100,
   CHIP_FAMILY_RV100,
   CHIP_FAMILY_RS100,
   CHIP_FAMILY_RV200,
   CHIP_FAMILY_RS200,
   CHIP_FAMILY_R200,
   CHIP_FAMILY_RV250,
   CHIP_FAMILY_RS300,
   CHIP_FAMILY_RV280,
   CHIP_FAMILY_R300,
   CHIP_FAMILY_R350,
   CHIP_FAMILY_RV350,
   CHIP_FAMILY_RV380,
   CHIP_FAMILY_R420,
   CHIP_FAMILY_RV410,
   CHIP_FAMILY_RS400,
   CHIP_FAMILY_RS600,
   CHIP_FAMILY_RS690,
   CHIP_FAMILY_RS740,
   CHIP_FAMILY_RV515,
   CHIP_FAMILY_R520,
   CHIP_FAMILY_RV530,
   CHIP_FAMILY_R580,
   CHIP_FAMILY_RV560,
   CHIP_FAMILY_RV570,
   CHIP_FAMILY_R600,
   CHIP_FAMILY_RV610,
   CHIP_FAMILY_RV630,
   CHIP_FAMILY_RV670,
   CHIP_FAMILY_RV620,
   CHIP_FAMILY_RV635,
   CHIP_FAMILY_RS780,
   CHIP_FAMILY_RS880,
   CHIP_FAMILY_RV770,
   CHIP_FAMILY_RV730,
   CHIP_FAMILY_RV710,
   CHIP_FAMILY_RV740,
   CHIP_FAMILY_LAST
};

/* General classes of Radeons, as described above the device ID section */
#define RADEON_CLASS_R100		(0 << 0)
#define RADEON_CLASS_R200		(1 << 0)
#define RADEON_CLASS_R300		(2 << 0)
#define RADEON_CLASS_R600		(3 << 0)
#define RADEON_CLASS_MASK		(3 << 0)

#define RADEON_CHIPSET_TCL		(1 << 2)	/* tcl support - any radeon */
#define RADEON_CHIPSET_BROKEN_STENCIL	(1 << 3)	/* r100 stencil bug */
#define R200_CHIPSET_YCBCR_BROKEN	(1 << 4)	/* r200 ycbcr bug */

#endif /* _RADEON_CHIPSET_H */
