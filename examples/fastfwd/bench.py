"""All 24 original known frames; independent four-state forwarding oracle is preserved."""

from example_fastfwd.fastfwd import Fastfwd, LaneInput, Channel
from pycircuit import bits, log, rule, struct, system


@struct
class Frame:
    lane0_valid: bits[1]
    lane0_data: bits[128]
    lane0_control: bits[5]
    lane1_valid: bits[1]
    lane1_data: bits[128]
    lane1_control: bits[5]
    lane2_valid: bits[1]
    lane2_data: bits[128]
    lane2_control: bits[5]
    lane3_valid: bits[1]
    lane3_data: bits[128]
    lane3_control: bits[5]
    engine0_valid: bits[1]
    engine0_data: bits[128]
    engine1_valid: bits[1]
    engine1_data: bits[128]
    engine2_valid: bits[1]
    engine2_data: bits[128]
    engine3_valid: bits[1]
    engine3_data: bits[128]


@rule
def stimulus(phase: bits[16]) -> Frame:
    lane0_valid: bits[1] = 0
    lane0_data: bits[128] = 0
    lane0_control: bits[5] = 0
    lane1_valid: bits[1] = 0
    lane1_data: bits[128] = 0
    lane1_control: bits[5] = 0
    lane2_valid: bits[1] = 0
    lane2_data: bits[128] = 0
    lane2_control: bits[5] = 0
    lane3_valid: bits[1] = 0
    lane3_data: bits[128] = 0
    lane3_control: bits[5] = 0
    engine0_valid: bits[1] = 0
    engine0_data: bits[128] = 0
    engine1_valid: bits[1] = 0
    engine1_data: bits[128] = 0
    engine2_valid: bits[1] = 0
    engine2_data: bits[128] = 0
    engine3_valid: bits[1] = 0
    engine3_data: bits[128] = 0
    if phase == 0:
        lane0_valid = 0
        lane0_data = 0
        lane0_control = 0
        lane1_valid = 1
        lane1_data = 0
        lane1_control = 11
        lane2_valid = 0
        lane2_data = 0
        lane2_control = 22
        lane3_valid = 1
        lane3_data = 0
        lane3_control = 1
        engine0_valid = 0
        engine0_data = 0
        engine1_valid = 1
        engine1_data = 0
        engine2_valid = 0
        engine2_data = 0
        engine3_valid = 1
        engine3_data = 0
    if phase == 1:
        lane0_valid = 1
        lane0_data = 18446744073709551615
        lane0_control = 7
        lane1_valid = 0
        lane1_data = 18446744073709551615
        lane1_control = 18
        lane2_valid = 1
        lane2_data = 18446744073709551615
        lane2_control = 29
        lane3_valid = 0
        lane3_data = 18446744073709551615
        lane3_control = 8
        engine0_valid = 1
        engine0_data = 18446744073709551615
        engine1_valid = 0
        engine1_data = 18446744073709551615
        engine2_valid = 1
        engine2_data = 18446744073709551615
        engine3_valid = 0
        engine3_data = 18446744073709551615
    if phase == 2:
        lane0_valid = 0
        lane0_data = 340282366920938463444927863358058659840
        lane0_control = 14
        lane1_valid = 1
        lane1_data = 340282366920938463444927863358058659840
        lane1_control = 25
        lane2_valid = 0
        lane2_data = 340282366920938463444927863358058659840
        lane2_control = 4
        lane3_valid = 1
        lane3_data = 340282366920938463444927863358058659840
        lane3_control = 15
        engine0_valid = 0
        engine0_data = 340282366920938463444927863358058659840
        engine1_valid = 1
        engine1_data = 340282366920938463444927863358058659840
        engine2_valid = 0
        engine2_data = 340282366920938463444927863358058659840
        engine3_valid = 1
        engine3_data = 340282366920938463444927863358058659840
    if phase == 3:
        lane0_valid = 1
        lane0_data = 18446744073709551616
        lane0_control = 21
        lane1_valid = 0
        lane1_data = 9444732965739290427392
        lane1_control = 0
        lane2_valid = 1
        lane2_data = 4835703278458516698824704
        lane2_control = 11
        lane3_valid = 0
        lane3_data = 2475880078570760549798248448
        lane3_control = 22
        engine0_valid = 1
        engine0_data = 1267650600228229401496703205376
        engine1_valid = 0
        engine1_data = 649037107316853453566312041152512
        engine2_valid = 1
        engine2_data = 332306998946228968225951765070086144
        engine3_valid = 0
        engine3_data = 170141183460469231731687303715884105728
    if phase == 4:
        lane0_valid = 0
        lane0_data = 1
        lane0_control = 28
        lane1_valid = 1
        lane1_data = 512
        lane1_control = 7
        lane2_valid = 0
        lane2_data = 262144
        lane2_control = 18
        lane3_valid = 1
        lane3_data = 134217728
        lane3_control = 29
        engine0_valid = 0
        engine0_data = 68719476736
        engine1_valid = 1
        engine1_data = 35184372088832
        engine2_valid = 0
        engine2_data = 18014398509481984
        engine3_valid = 1
        engine3_data = 9223372036854775808
    if phase == 5:
        lane0_valid = 1
        lane0_data = 113427455640312821160607117168492587690
        lane0_control = 3
        lane1_valid = 0
        lane1_data = 113427455640312821142160373094783036075
        lane1_control = 14
        lane2_valid = 1
        lane2_data = 113427455640312821197500605315911690920
        lane2_control = 25
        lane3_valid = 0
        lane3_data = 113427455640312821179053861242202139305
        lane3_control = 4
        engine0_valid = 1
        engine0_data = 113427455640312821086820140873654381230
        engine1_valid = 0
        engine1_data = 113427455640312821068373396799944829615
        engine2_valid = 1
        engine2_data = 113427455640312821123713629021073484460
        engine3_valid = 0
        engine3_data = 113427455640312821105266884947363932845
    if phase == 6:
        lane0_valid = 0
        lane0_data = 43681385781482968842077549096336084457
        lane0_control = 10
        lane1_valid = 1
        lane1_data = 63572973556190682225861964428563107048
        lane1_control = 21
        lane2_valid = 0
        lane2_data = 83422778293245425929337827418937348075
        lane2_control = 0
        lane3_valid = 1
        lane3_data = 18534384500567929690092302333723070186
        lane3_control = 11
        engine0_valid = 0
        engine0_data = 38384189237622673393570408327818496493
        engine1_valid = 1
        engine1_data = 143016337171398163356663589925597137132
        engine2_valid = 0
        engine2_data = 162866141908452907060139452915971378159
        engine3_valid = 1
        engine3_data = 97977748115775410820893927830757100270
    if phase == 7:
        lane0_valid = 1
        lane0_data = 167299589389480145040578349771822714344
        lane0_control = 17
        lane1_valid = 0
        lane1_data = 152631249273780045691970293397931609321
        lane1_control = 28
        lane2_valid = 1
        lane2_data = 132604214137287313245225432718724681706
        lane2_control = 7
        lane3_valid = 0
        lane3_data = 107592660218157293156200631303091572459
        lane3_control = 18
        engine0_valid = 1
        engine0_data = 87565625081664560709458013627605830124
        engine1_valid = 0
        engine1_data = 72567253394797622074315071661324290285
        engine2_valid = 1
        engine2_data = 52540218258304889627570210982117362670
        engine3_valid = 0
        engine3_data = 27528664339174869538545409566484253423
    if phase == 8:
        lane0_valid = 1
        lane0_data = 285600881014337657747463922274907442663
        lane0_control = 24
        lane1_valid = 0
        lane1_data = 289541732839626380582615676588932123878
        lane1_control = 3
        lane2_valid = 0
        lane2_data = 266856241711563816353169713633155469285
        lane2_control = 14
        lane3_valid = 0
        lane3_data = 334890647497377907330517918281137442532
        lane3_control = 25
        engine0_valid = 0
        engine0_data = 312205156369315343101074198329081973219
        engine1_valid = 0
        engine1_data = 209477736960643956817386502228288653538
        engine2_valid = 0
        engine2_data = 186792245832581392587940539272511998945
        engine3_valid = 0
        engine3_data = 254826651618395483565288743920493972192
    if phase == 9:
        lane0_valid = 0
        lane0_data = 68936717701396370482590115518625861094
        lane0_control = 31
        lane1_valid = 1
        lane1_data = 80852937501394588518271224055503441127
        lane1_control = 10
        lane2_valid = 0
        lane2_data = 60825902364901856071526363359116644324
        lane2_control = 21
        lane3_valid = 0
        lane3_data = 41131260428911499400329813889766706917
        lane3_control = 0
        engine0_valid = 0
        engine0_data = 21104225292418766953587196197101095394
        engine1_valid = 0
        engine1_data = 160296301116602069501498896928501319907
        engine2_valid = 0
        engine2_data = 140269265980109337054754036232114523104
        engine3_valid = 0
        engine3_data = 120574624044118980383557486762764585697
    if phase == 10:
        lane0_valid = 0
        lane0_data = 187238009326253883189475687952991112677
        lane0_control = 6
        lane1_valid = 0
        lane1_data = 175228125202123615475994781248813452516
        lane1_control = 17
        lane2_valid = 1
        lane2_data = 195077929939178359179470644239187693543
        lane2_control = 28
        lane3_valid = 0
        lane3_data = 215260127876735478806068771011915468518
        lane3_control = 7
        engine0_valid = 0
        engine0_data = 235109932613790222509546876971651156449
        engine1_valid = 0
        engine1_data = 265305312783610423442452910604054087904
        engine2_valid = 0
        engine2_data = 285155117520665167145928773594428328931
        engine3_valid = 0
        engine3_data = 305337315458222286772526900367156103906
    if phase == 11:
        lane0_valid = 0
        lane0_data = 310856212934251059387976488628477742564
        lane0_control = 13
        lane1_valid = 0
        lane1_data = 306821696784830286875024936147152981221
        lane1_control = 24
        lane2_valid = 0
        lane2_data = 329329957513454862361201901396917080038
        lane2_control = 3
        lane3_valid = 1
        lane3_data = 261783107729207534339255274052312944359
        lane3_control = 14
        engine0_valid = 0
        engine0_data = 284291368457832109825434482271438490080
        engine1_valid = 0
        engine1_data = 216123876939568536126565305304266754273
        engine2_valid = 0
        engine2_data = 238632137668193111612742270554030853090
        engine3_valid = 0
        engine3_data = 171085287883945783590795643209426717411
    if phase == 12:
        lane0_valid = 0
        lane0_data = 88875137638170108631487453631074782691
        lane0_control = 20
        lane1_valid = 0
        lane1_data = 103449813429738158302295711837665807586
        lane1_control = 31
        lane2_valid = 0
        lane2_data = 123299618166792902005771574810860179425
        lane2_control = 10
        lane3_valid = 0
        lane3_data = 148798728087489685050197953529871126240
        lane3_control = 21
        engine0_valid = 1
        engine0_data = 168648532824544428753676059541146421735
        engine1_valid = 0
        engine1_data = 12751993584476407701410033618815731942
        engine2_valid = 0
        engine2_data = 32601798321531151404885896592010103781
        engine3_valid = 0
        engine3_data = 58100908242227934449312275311021050596
    if phase == 13:
        lane0_valid = 0
        lane0_data = 212493341246167284829988254306561412578
        lane0_control = 27
        lane1_valid = 0
        lane1_data = 192508089147327521768404040807034309859
        lane1_control = 6
        lane2_valid = 0
        lane2_data = 172481054010834789321659180110647513056
        lane2_control = 17
        lane3_valid = 0
        lane3_data = 237857003805079048516306282499239628513
        lane3_control = 28
        engine0_valid = 0
        engine0_data = 217829968668586316069563664840933755366
        engine1_valid = 1
        engine1_data = 282585276728814329882436122786311096551
        engine2_valid = 0
        engine2_data = 262558241592321597435691262089924299748
        engine3_valid = 0
        engine3_data = 327934191386565856630338364478516415205
    if phase == 14:
        lane0_valid = 0
        lane0_data = 330794632871024797536873826740926664161
        lane0_control = 2
        lane1_valid = 0
        lane1_data = 329418572713173856659049423929315347680
        lane1_control = 13
        lane2_valid = 0
        lane2_data = 306733081585111292429603460990718562275
        lane2_control = 24
        lane3_valid = 0
        lane3_data = 284379983657551104123279761834475310818
        lane3_control = 3
        engine0_valid = 0
        engine0_data = 261694492529488539893836041899599710693
        engine1_valid = 0
        engine1_data = 238720752867912106058163745710465272036
        engine2_valid = 1
        engine2_data = 216035261739849541828717782771868486631
        engine3_valid = 0
        engine3_data = 193682163812289353522394083615625235174
    if phase == 15:
        lane0_valid = 0
        lane0_data = 114130469558083510272000019984645082592
        lane0_control = 9
        lane1_valid = 0
        lane1_data = 120729777374942064594704971395886664929
        lane1_control = 20
        lane2_valid = 0
        lane2_data = 100702742238449332147960110716679737314
        lane2_control = 31
        lane3_valid = 0
        lane3_data = 160761780049553927924778961158988680931
        lane3_control = 10
        engine0_valid = 0
        engine0_data = 140734744913061195478036343483502938596
        engine1_valid = 0
        engine1_data = 40665781495959640977049749659279345893
        engine2_valid = 0
        engine2_data = 20638746359466908530304888980072418278
        engine3_valid = 1
        engine3_data = 80697784170571504307123739422381361895
    if phase == 16:
        lane0_valid = 0
        lane0_data = 232431761182941022388589782129024159231
        lane0_control = 16
        lane1_valid = 1
        lane1_data = 215104965075671091552428528657916153086
        lane1_control = 27
        lane2_valid = 1
        lane2_data = 234954769812725835846200201989816176637
        lane2_control = 6
        lane3_valid = 1
        lane3_data = 175383288003188002434487118492179418876
        lane3_control = 17
        engine0_valid = 1
        engine0_data = 195233092740242746728261034827800627707
        engine1_valid = 1
        engine1_data = 305182152657157899518886658013156788474
        engine2_valid = 1
        engine2_data = 325031957394212643812658331345056812025
        engine3_valid = 1
        engine3_data = 265460475584674810400945247847420054264
    if phase == 17:
        lane0_valid = 1
        lane0_data = 15767597869999735123715975372742577662
        lane0_control = 23
        lane1_valid = 0
        lane1_data = 6416169737439299488084076124487470335
        lane1_control = 2
        lane2_valid = 1
        lane2_data = 28924430466063875564556851715777351676
        lane2_control = 13
        lane3_valid = 1
        lane3_data = 51765084395190826235986317816692788989
        lane3_control = 24
        engine0_valid = 1
        engine0_data = 74273345123815402312461336411703855610
        engine1_valid = 1
        engine1_data = 85859533352646780471311748997485349115
        engine2_valid = 1
        engine2_data = 108367794081271356547784524588775230456
        engine3_valid = 1
        engine3_data = 131208448010398307219213990689690667769
    if phase == 18:
        lane0_valid = 1
        lane0_data = 134068889494857247830601547807107829245
        lane0_control = 30
        lane1_valid = 1
        lane1_data = 143326653303285634378729459246768508156
        lane1_control = 9
        lane2_valid = 0
        lane2_data = 163176458040340378672501132595848400895
        lane2_control = 20
        lane3_valid = 1
        lane3_data = 98288064247662881842959797151928471294
        lane3_control = 31
        engine0_valid = 1
        engine0_data = 118137868984717626136733713470369810937
        engine1_valid = 1
        engine1_data = 63262657424303210613500284886125037816
        engine2_valid = 1
        engine2_data = 83112462161357954907271958235204930555
        engine3_valid = 1
        engine3_data = 18224068368680458077730622791285000954
    if phase == 19:
        lane0_valid = 1
        lane0_data = 257687093102854424029102348482594459132
        lane0_control = 5
        lane1_valid = 1
        lane1_data = 274920224885992305777759614145108036861
        lane1_control = 16
        lane2_valid = 1
        lane2_data = 297428485614616881854232389753577787390
        lane2_control = 27
        lane3_valid = 0
        lane3_data = 314952227560604169107833603908210052863
        lane3_control = 6
        engine0_valid = 1
        engine0_data = 337460488289228745184308622486041250296
        engine1_valid = 1
        engine1_data = 184222405040730555029299983302221809913
        engine2_valid = 1
        engine2_data = 206730665769355131105772758910691560442
        engine3_valid = 1
        engine3_data = 224254407715342418359373973065323825915
    if phase == 20:
        lane0_valid = 1
        lane0_data = 35706017806773473272613313485191499259
        lane0_control = 12
        lane1_valid = 1
        lane1_data = 29013045665782869272108563906649836794
        lane1_control = 23
        lane2_valid = 1
        lane2_data = 6327554537720305632958411309578833913
        lane2_control = 2
        lane3_valid = 1
        lane3_data = 74361960323534396020010805598855155448
        lane3_control = 13
        engine0_valid = 0
        engine0_data = 51676469195471832380862896039865076223
        engine1_valid = 1
        engine1_data = 108456409280990350402910189403683866878
        engine2_valid = 1
        engine2_data = 85770918152927786763760036806612863997
        engine3_valid = 1
        engine3_data = 153805323938741877150812431095889185532
    if phase == 21:
        lane0_valid = 1
        lane0_data = 159324221414770649471114114160678129146
        lane0_control = 19
        lane1_valid = 1
        lane1_data = 160606617248489540671138718804989365499
        lane1_control = 30
        lane2_valid = 1
        lane2_data = 140579582111996808814689668467308220408
        lane2_control = 9
        lane3_valid = 1
        lane3_data = 120884940176006451553197308639252631289
        lane3_control = 20
        engine0_valid = 1
        engine0_data = 100857905039513719696750501339652409854
        engine1_valid = 0
        engine1_data = 80542621369507117053483497068382046463
        engine2_valid = 1
        engine2_data = 60515586233014385197034446730700901372
        engine3_valid = 1
        engine3_data = 40820944297024027935542086902645312253
    if phase == 22:
        lane0_valid = 1
        lane0_data = 277625513039628162177999686595043380729
        lane0_control = 26
        lane1_valid = 1
        lane1_data = 297517100814335875561784101927270403320
        lane1_control = 5
        lane2_valid = 1
        lane2_data = 274831609686273311922633949347379269627
        lane2_control = 16
        lane3_valid = 1
        lane3_data = 337549103488947738891858091690372419322
        lane3_control = 27
        engine0_valid = 1
        engine0_data = 314863612360885175252710182114202470909
        engine1_valid = 1
        engine1_data = 206819280969074124960898423708420327676
        engine2_valid = 0
        engine2_data = 184133789841011561321748271128529193983
        engine3_valid = 1
        engine3_data = 246851283643685988290972413471522343678
    if phase == 23:
        lane0_valid = 1
        lane0_data = 60961349726686874913125879838761799160
        lane0_control = 1
        lane1_valid = 1
        lane1_data = 46293009610986775564517823464870694137
        lane1_control = 12
        lane2_valid = 1
        lane2_data = 68801270339611351640990599073340444666
        lane2_control = 23
        lane3_valid = 1
        lane3_data = 1254420555364023028748161370030657275
        lane3_control = 2
        engine0_valid = 1
        engine0_data = 23762681283988599105223179982221593084
        engine1_valid = 1
        engine1_data = 136370197192473583678549905444147480829
        engine2_valid = 1
        engine2_data = 158878457921098159755022681052617231358
        engine3_valid = 0
        engine3_data = 91331608136850831142780243349307443967
    return Frame(
        lane0_valid=lane0_valid,
        lane0_data=lane0_data,
        lane0_control=lane0_control,
        lane1_valid=lane1_valid,
        lane1_data=lane1_data,
        lane1_control=lane1_control,
        lane2_valid=lane2_valid,
        lane2_data=lane2_data,
        lane2_control=lane2_control,
        lane3_valid=lane3_valid,
        lane3_data=lane3_data,
        lane3_control=lane3_control,
        engine0_valid=engine0_valid,
        engine0_data=engine0_data,
        engine1_valid=engine1_valid,
        engine1_data=engine1_data,
        engine2_valid=engine2_valid,
        engine2_data=engine2_data,
        engine3_valid=engine3_valid,
        engine3_data=engine3_data,
    )


@rule
def advance(phase):
    if phase < 23:
        phase = phase + 1


@system
def FastfwdSystem():  # noqa: N802
    phase: bits[16] = 0
    frame = stimulus(phase)
    lane0 = LaneInput(
        valid=frame.lane0_valid, data=frame.lane0_data, control=frame.lane0_control
    )
    lane1 = LaneInput(
        valid=frame.lane1_valid, data=frame.lane1_data, control=frame.lane1_control
    )
    lane2 = LaneInput(
        valid=frame.lane2_valid, data=frame.lane2_data, control=frame.lane2_control
    )
    lane3 = LaneInput(
        valid=frame.lane3_valid, data=frame.lane3_data, control=frame.lane3_control
    )
    engine0 = Channel(valid=frame.engine0_valid, data=frame.engine0_data)
    engine1 = Channel(valid=frame.engine1_valid, data=frame.engine1_data)
    engine2 = Channel(valid=frame.engine2_valid, data=frame.engine2_data)
    engine3 = Channel(valid=frame.engine3_valid, data=frame.engine3_data)
    dut = Fastfwd(lane0, lane1, lane2, lane3, engine0, engine1, engine2, engine3)

    @rule
    def check():
        assert dut.backpressure == 0, "stateless backpressure"
        assert dut.lane0.valid == frame.lane0_valid, "lane0 valid forwarding"
        assert dut.lane0.data == frame.lane0_data, "lane0 data forwarding"
        assert dut.engine0.valid == frame.engine0_valid, "engine0 valid forwarding"
        assert dut.engine0.data == frame.engine0_data, "engine0 data forwarding"
        assert dut.engine0.latency == 0, "engine0 latency default"
        assert dut.engine0.datapath_valid == 0, "engine0 datapath_valid default"
        assert dut.engine0.datapath_data == 0, "engine0 datapath_data default"
        assert dut.lane1.valid == frame.lane1_valid, "lane1 valid forwarding"
        assert dut.lane1.data == frame.lane1_data, "lane1 data forwarding"
        assert dut.engine1.valid == frame.engine1_valid, "engine1 valid forwarding"
        assert dut.engine1.data == frame.engine1_data, "engine1 data forwarding"
        assert dut.engine1.latency == 0, "engine1 latency default"
        assert dut.engine1.datapath_valid == 0, "engine1 datapath_valid default"
        assert dut.engine1.datapath_data == 0, "engine1 datapath_data default"
        assert dut.lane2.valid == frame.lane2_valid, "lane2 valid forwarding"
        assert dut.lane2.data == frame.lane2_data, "lane2 data forwarding"
        assert dut.engine2.valid == frame.engine2_valid, "engine2 valid forwarding"
        assert dut.engine2.data == frame.engine2_data, "engine2 data forwarding"
        assert dut.engine2.latency == 0, "engine2 latency default"
        assert dut.engine2.datapath_valid == 0, "engine2 datapath_valid default"
        assert dut.engine2.datapath_data == 0, "engine2 datapath_data default"
        assert dut.lane3.valid == frame.lane3_valid, "lane3 valid forwarding"
        assert dut.lane3.data == frame.lane3_data, "lane3 data forwarding"
        assert dut.engine3.valid == frame.engine3_valid, "engine3 valid forwarding"
        assert dut.engine3.data == frame.engine3_data, "engine3 data forwarding"
        assert dut.engine3.latency == 0, "engine3 latency default"
        assert dut.engine3.datapath_valid == 0, "engine3 datapath_valid default"
        assert dut.engine3.datapath_data == 0, "engine3 datapath_data default"
        log("info", "phase", phase)

    advance(phase)
    check()
