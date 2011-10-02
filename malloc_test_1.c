/**
 * @file malloc_test.c
 *
 * @author Dario Sneidermanis
 */

#include <stdio.h>
#include "malloc.h"


#define SIZE       50
#define RUNS       5000
#define MAX_ALLOC  1000

static int rand ( void ) {

    static int num[] = {

        1804289383, 846930886,  1681692777, 1714636915, 1957747793, 424238335,
        719885386,  1649760492, 596516649,  1189641421, 1025202362, 1350490027,
        783368690,  1102520059, 2044897763, 1967513926, 1365180540, 1540383426,
        304089172,  1303455736, 35005211,   521595368,  294702567,  1726956429,
        336465782,  861021530,  278722862,  233665123,  2145174067, 468703135,
        1101513929, 1801979802, 1315634022, 635723058,  1369133069, 1125898167,
        1059961393, 2089018456, 628175011,  1656478042, 1131176229, 1653377373,
        859484421,  1914544919, 608413784,  756898537,  1734575198, 1973594324,
        149798315,  2038664370, 1129566413, 184803526,  412776091,  1424268980,
        1911759956, 749241873,  137806862,  42999170,   982906996,  135497281,
        511702305,  2084420925, 1937477084, 1827336327, 572660336,  1159126505,
        805750846,  1632621729, 1100661313, 1433925857, 1141616124, 84353895,
        939819582,  2001100545, 1998898814, 1548233367, 610515434,  1585990364,
        1374344043, 760313750,  1477171087, 356426808,  945117276,  1889947178,
        1780695788, 709393584,  491705403,  1918502651, 752392754,  1474612399,
        2053999932, 1264095060, 1411549676, 1843993368, 943947739,  1984210012,
        855636226,  1749698586, 1469348094, 1956297539, 1036140795, 463480570,
        2040651434, 1975960378, 317097467,  1892066601, 1376710097, 927612902,
        1330573317, 603570492,  1687926652, 660260756,  959997301,  485560280,
        402724286,  593209441,  1194953865, 894429689,  364228444,  1947346619,
        221558440,  270744729,  1063958031, 1633108117, 2114738097, 2007905771,
        1469834481, 822890675,  1610120709, 791698927,  631704567,  498777856,
        1255179497, 524872353,  327254586,  1572276965, 269455306,  1703964683,
        352406219,  1600028624, 160051528,  2040332871, 112805732,  1120048829,
        378409503,  515530019,  1713258270, 1573363368, 1409959708, 2077486715,
        1373226340, 1631518149, 200747796,  289700723,  1117142618, 168002245,
        150122846,  439493451,  990892921,  1760243555, 1231192379, 1622597488,
        111537764,  338888228,  2147469841, 438792350,  1911165193, 269441500,
        2142757034, 116087764,  1869470124, 155324914,  8936987,    1982275856,
        1275373743, 387346491,  350322227,  841148365,  1960709859, 1760281936,
        771151432,  1186452551, 1244316437, 971899228,  1476153275, 213975407,
        1139901474, 1626276121, 653468858,  2130794395, 1239036029, 1884661237,
        1605908235, 1350573793, 76065818,   1605894428, 1789366143, 1987231011,
        1875335928, 1784639529, 2103318776, 1597322404, 1939964443, 2112255763,
        1432114613, 1067854538, 352118606,  1782436840, 1909002904, 165344818,
        1395235128, 532670688,  1351797369, 492067917,  1504569917, 680466996,
        706043324,  496987743,  159259470,  1359512183, 480298490,  1398295499,
        1096689772, 2086206725, 601385644,  1172755590, 1544617505, 243268139,
        1012502954, 1272469786, 2027907669, 968338082,  722308542,  1820388464,
        933110197,  6939507,    740759355,  1285228804, 1789376348, 502278611,
        1450573622, 1037127828, 1034949299, 654887343,  1529195746, 392035568,
        1335354340, 87755422,   889023311,  1494613810, 1447267605, 1369321801,
        745425661,  396473730,  1308044878, 1346811305, 1569229320, 705178736,
        1590079444, 434248626,  1977648522, 1470503465, 1402586708, 552473416,
        1143408282, 188213258,  559412924,  1884167637, 1473442062, 201305624,
        238962600,  776532036,  1238433452, 1273911899, 1431419379, 620145550,
        1665947468, 619290071,  707900973,  407487131,  2113903881, 7684930,
        1776808933, 711845894,  404158660,  937370163,  2058657199, 1973387981,
        1642548899, 1501252996, 260152959,  1472713773, 824272813,  1662739668,
        2025187190, 1967681095, 1850952926, 437116466,  1704365084, 1176911340,
        638422090,  1943327684, 1953443376, 1876855542, 1069755936, 1237379107,
        349517445,  588219756,  1856669179, 1057418418, 995706887,  1823089412,
        1065103348, 625032172,  387451659,  1469262009, 1562402336, 298625210,
        1295166342, 1057467587, 1799878206, 1555319301, 382697713,  476667372,
        1070575321, 260401255,  296864819,  774044599,  697517721,  2001229904,
        1950955939, 1335939811, 1797073940, 1756915667, 1065311705, 719346228,
        846811127,  1414829150, 1307565984, 555996658,  324763920,  155789224,
        231602422,  1389867269, 780821396,  619054081,  711645630,  195740084,
        917679292,  2006811972, 1253207672, 570073850,  1414647625, 1635905385,
        1046741222, 337739299,  1896306640, 1343606042, 1111783898, 446340713,
        1197352298, 915256190,  1782280524, 846942590,  524688209,  700108581,
        1566288819, 1371499336, 2114937732, 726371155,  1927495994, 292218004,
        882160379,  11614769,   1682085273, 1662981776, 630668850,  246247255,
        1858721860, 1548348142, 105575579,  964445884,  2118421993, 1520223205,
        452867621,  1017679567, 1857962504, 201690613,  213801961,  822262754,
        648031326,  1411154259, 1737518944, 282828202,  110613202,  114723506,
        982936784,  1676902021, 1486222842, 950390868,  255789528,  1266235189,
        1242608872, 1137949908, 1277849958, 777210498,  653448036,  1908518808,
        1023457753, 364686248,  1309383303, 1129033333, 1329132133, 1280321648,
        501772890,  1781999754, 150517567,  212251746,  1983690368, 364319529,
        1034514500, 484238046,  1775473788, 624549797,  767066249,  1886086990,
        739273303,  1750003033, 1415505363, 78012497,   552910253,  1671294892,
        1344247686, 1795519125, 661761152,  474613996,  425245975,  1315209188,
        235649157,  1448703729, 1679895436, 1545032460, 430253414,  861543921,
        677870460,  932026304,  496060028,  828388027,  1144278050, 332266748,
        1192707556, 31308902,   816504794,  820697697,  655858699,  1583571043,
        559301039,  1395132002, 1186090428, 1974806403, 1473144500, 1739000681,
        1498617647, 669908538,  1387036159, 12895151,   1144522535, 1812282134,
        1328104339, 1380171692, 1113502215, 860516127,  777720504,  1543755629,
        1722060049, 1455590964, 328298285,  70636429,   136495343,  1472576335,
        402903177,  1329202900, 1503885238, 1219407971, 2416949,    12260289,
        655495367,  561717988,  1407392292, 1841585795, 389040743,  733053144,
        1433102829, 1887658390, 1402961682, 672655340,  1900553541, 400000569,
        337453826,  1081174232, 1780172261, 1450956042, 1941690360, 410409117,
        847228023,  1516266761, 1866000081, 1175526309, 1586903190, 2002495425,
        500618996,  1989806367, 1184214677, 2004504234, 1061730690, 1186631626,
        2016764524, 1717226057, 1748349614, 1276673168, 1411328205, 2137390358,
        2009726312, 696947386,  1877565100, 1265204346, 1369602726, 1630634994,
        1665204916, 1707056552, 564325578,  1297893529, 1010528946, 358532290,
        1708302647, 1857756970, 1874799051, 1426819080, 885799631,  1314218593,
        1281830857, 1386418627, 1156541312, 318561886,  1243439214, 70788355,
        1505193512, 1112720090, 1788014412, 1106059479, 241909610,  1051858969,
        1095966189, 104152274,  1748806355, 826047641,  1369356620, 970925433,
        309198987,  887077888,  530498338,  873524566,  37487770,   1541027284,
        1232056856, 1745790417, 1251300606, 959372260,  1025125849, 2137100237,
        126107205,  159473059,  1376035217, 1282648518, 478034945,  471990783,
        1353436873, 1983228458, 1584710873, 993967637,  941804289,  1826620483,
        2045826607, 2037770478, 1930772757, 1647149314, 716334471,  1152645729,
        470591100,  1025533459, 2039723618, 1001089438, 1899058025, 2077211388,
        394633074,  983631233,  1675518157, 1645933681, 1943003493, 553160358
    };

    static size_t pos = 0;
    static int last = 0;

    last *= 31;
    last += pos < sizeof(num)/sizeof(int) ? num[pos++] : num[pos = 0];
    if ( last < 0 )
        last = -(last+1);

    return last;
}


#define MEM_SIZE (10 * 1024 * 1024)

char memory [ MEM_SIZE ];


int main( void ) {

    init_malloc( memory, MEM_SIZE );

    /* this should not be done :P */
    size_t free_memory = *(size_t*)get_malloc_context();

    int ** vector = malloc( SIZE * sizeof( int* ) );

    if ( vector == NULL ) {

        printf( "nothing\n" );
        return 1;
    }

    for ( int i = 0; i < SIZE; i++ )
        vector[i] = NULL;

    for ( int i = 0; i < RUNS; i++ ) {

        int j = rand() % SIZE, k;

        if ( vector[j] ) {

            printf( "\nfreeing vector[%d] = %p\n", j, (void*)vector[j] );

            free( vector[j] );

            vector[j] = NULL;

        } else {

            k = rand() % MAX_ALLOC;

            if ( ( vector[j] = malloc( (unsigned)k * sizeof(int) ) ) ) {

                printf( "allocated vector[%d] = %p (%d)\n", j,
                        (void*)vector[j], k );

                for( ; k > 0; k-- )
                    vector[j][k-1] = rand();

            } else {

                printf( "error while allocating in vector[%d]\n", j );
            }
        }
    }

    for ( int i = 0; i < SIZE; i++ )

        if ( vector[i] ) {

            printf( "freeing vector[%d] = %p\n", i, (void*)vector[i] );
            free( vector[i] );
            vector[i] = NULL;
        }

    free( vector );

    printf("\n");

    /* this should not be done :P */
    free_memory -= *(size_t*)get_malloc_context();

    if ( free_memory || check_malloc() )
        printf( "THERE WAS AN ERROR\n" );
    else
        printf( "SUCCESSFUL RUN!\n" );

    return 0;
}
