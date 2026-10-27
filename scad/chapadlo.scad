// =====================================================================
//  CHÁPADLÁ PRE HALLOWEEN TEKVICU – kompletný mechanizmus
//  * 2 chápadlá, každé ovládané 2 servami cez lanká (4 lanká / chápadlo)
//  * "kocka": krabica so 4 servami na podstavcoch, kladky, vodiace bloky
//  * vymeniteľný medzikus (rovný alebo zahnutý) medzi krabicou a chápadlom
//  * lanká vedú v PTFE trubičkách 4x2 mm od servo bloku až do základne chápadla
//
//  Export STL: nastav "part" a daj Render (F6) -> Export STL
// =====================================================================

/* [Výber dielu] */
part = "assembly"; // [assembly, box, box_floor, drain_column, box_wall_port, box_wall_side, box_wall_side_board, lid, eye_deck, eye_roof, eye_pod, tip_stick, spacer, pulley, segments_plate, segment, base, tip, servo_bracket]
seg_index = 0;     // pre part = "segment"

/* [Chápadlo] */
N          = 12;   // počet segmentov
R0         = 14;   // max. polomer najväčšieho segmentu [mm]
H0         = 18;   // výška najväčšieho segmentu
G0         = 3;    // medzera medzi segmentmi pri najväčšom segmente
s_tip      = 0.55; // mierka posledného segmentu voči prvému
face_ratio = 0.80; // polomer dosadacej plôšky / R
cable_ratio= 0.70; // (len pre seg_style = "bead")
cable_edge = 2.2;  // vzdialenosť stredu otvoru na lanko od vonkajšieho okraja [mm]
bulge_pow  = 0.7;  // tvar "korálky" (menšie = bruchatejšie)
seg_style  = "cone"; // [cone, bead]  cone = valec so zrazenými kužeľmi a krčkom (ako vo videu), bead = korálka
neck_ratio = 0.50; // polomer krčku / R (spoj medzi segmentmi)
neck_wall  = 1.5;  // min. stena okolo chrbtice v krčku
a_top      = 30;   // sklon horného kužeľa od vodorovnej [st.]
a_bot      = 40;   // sklon spodného kužeľa (>= ~40 kvôli tlači bez podpier)
seg_facets = 0;    // 0 = hladký valec, napr. 8 = osemhranný vzhľad
scallops   = true; // bočné výrezy medzi lankami (menej materiálu, členitejší povrch)
scallop_d  = 1.15; // vzdialenosť stredu výrezu od osi / R
scallop_r  = 0.55; // polomer výrezu / R
bend_target = 160; // koľko stupňov ohybu má zvládnuť kladka (geometria dovolí aj viac)

/* [Chrbtica - PU hadičky] */
bb_d_base  = 10;   // PU hadička 10x8 v spodnej časti (vnútri ide aj 6x4)
bb_d_tip   = 6;    // PU hadička 6x4 – ide celým chápadlom
bb_split   = 0;    // len hadička 6x4 v celom chápadle (10x8 bola príliš tuhá)
bb_clear   = 0.3;

/* [Lanká] */
cable_hole_d = 1.8;  // pletená šnúra 0,25 mm; tlačený otvor vyjde o niečo menší
chamfer_d    = 2.4;
chamfer_h    = 0.6;

/* [Koncovka] */
tip_type    = "stick";  // [cone, stick]  cone = špička, stick = palička na prilepenie ruky
stick_d     = 8;       // priemer paličky
stick_len   = 35;      // dĺžka paličky
stick_wire  = 3.2;     // otvor na drôtenú kostru ruky (0 = bez otvoru)
stick_wire_depth = 25;
stick_through    = true;  // otvor cez celú paličku (káble k LED v ruke)
stick_hose       = true;  // hadička 6x4 vojde do spodku paličky a zaistí sa kolíkom ako články
stick_hose_len   = 12;    // ako hlboko nad kužeľom ide hadička do paličky
stick_hose_od    = 11;    // hrubšia spodná časť paličky okolo hadičky

/* [Kolík z filamentu] */
use_pin = true;
pin_d   = 1.9;

/* [Základňa chápadla] */
base_h        = 16;
flange_r      = 24;
flange_t      = 4;
mount_hole_d  = 3.4; // M3
mount_hole_r  = 19.5;
ptfe_od       = 4.0;
ptfe_clear    = 0.15;
ptfe_depth    = 8;
base_socket_r = 10;   // polomer pätíc PTFE v základni (kanál sa šikmo vyosí k lankám)

/* [Medzikus] */
spacer_len    = 30;  // rovná časť od krabice [mm]
spacer_angle  = 0;   // 0 = rovný, napr. 30-45 = chápadlo mieri šikmo hore
spacer_bend_r = 40;  // polomer ohybu (PTFE trubičky vnútri)
spacer_len2   = 12;  // rovná časť za ohybom (len ak angle > 0 alebo offset > 0)
spacer_offset = 0;   // bočné vyosenie (S-tvar) v mm; 30 = obe ruky presne oproti sebe
spacer_rot    = 0;   // natočenie medzikusu na porte (0 = ohyb hore, 90 = vyosenie k stredu krabice)
sp_od         = 31;
sp_id         = 26;
nut_d         = 6.5; // M3 matica cez rohy (+vôľa)
nut_h         = 2.6;

/* [Kladka a servo] */
servo_half_range  = 90;
pulley_r_override = 0;
groove_w     = 2.2;
flange_extra = 2.5;
horn_d       = 20.5;     // (pôvodný okrúhly horn – už sa nepoužíva)
horn_hub_d   = 8.5;      // výstupok v strede hornu (s vôľou)
horn_boss_h  = 1.6;      // plytšie zahĺbenie: kladka si sadne priamo na vrch stredu hornu
horn_access_d = 6.5;     // priechodný otvor pre skrutkovač a hlavu skrutky hornu
horn_fix_r   = 10.5;     // poloha skrutiek do ramena hornu
horn_long_r  = 21;       // dlhé ramená prechádzajú až von cez okraj (dĺžka nerozhoduje)
horn_w_long  = 5.0;      // šírka dlhého ramena pri strede (+ vôľa)
horn_w_long_tip = 3.6;   // šírka na konci (r = horn_long_r)
horn_short_r = 11.5;     // krátke rameno (+ vôľa na dĺžku)
horn_w_short = 5.0;      // šírka krátkeho ramena pri strede (+ vôľa)
horn_w_short_tip = 3.8;  // šírka na konci
horn_depth   = 2.0;
horn_extra   = 1.0;      // o koľko širšie drážky pre DLHÉ ramená (krátke a stred boli ok)
horn_root_extra = 0.5;   // dlhé rameno ešte širšie pri koreni (od stredu po polovicu ramena)
horn_root_r  = 9;        // po aký polomer platí rozšírenie pri koreni (rameno ~17 mm)
horn_screw_d = 2.6;      // otvor pre stredovú skrutku
horn_head_d  = 6;        // zahĺbenie pre hlavu stredovej skrutky
pulley_floor = 0.6;      // tenká podlaha (3 vrstvy) – stačí pôvodná skrutka hornu
floor_t      = 0.8;
anchor_d     = 2.2;      // kanálik pre šnúru (tlačený vyjde ~1,9 mm)
peg_pilot_d  = 2.6;      // predvŕtanie pre napínaciu skrutku M3 (samoreznú)

/* [Servo MG90S/SG90 - ZMERAJTE] */
servo_body          = [23.2, 12.8];
servo_screw_spacing = 27.8;
servo_screw_d       = 2.0;  // predvŕtanie pre samorezné skrutky serva
servo_cable_end     = 1;    // na ktorom konci tela vychádza kábel serva (+1 = ďalej od hriadeľa, -1 = bližšie)
servo_cable_notch   = [3.5, 5.5]; // výrez pre kábel: hĺbka za telom, šírka
shaft_offset        = 5.6;  // os hriadeľa voči stredu tela serva
pulley_base_z       = 12;   // výška spodku kladky nad plochou, na ktorej ležia úchyty serva
plate_t             = 3;

/* [Krabica] */
box_in    = 130;   // vnútorný rozmer (štvorec)
box_in_h  = 84;    // vnútorná výška
box_wall  = 3;
box_floor = 3;
servo_x   = 30;    // x-poloha serv jedného chápadla
y_hi      = -44;   // zadné (vyvýšené) servo
y_lo      = 13;    // predné (nízke) servo
P_hi      = 52;    // výška podstavca zadného serva
P_lo      = 25;    // výška podstavca predného serva
ped_wall  = 2.4;
ped_top   = 3;
ped_screw_len = 10;   // hĺbka plného materiálu pod skrutkou úchytu serva
windows    = true;  // okná v stenách (rýchlejšia tlač, prístup ku skrutkám)
win_margin = 12;
skewer_d     = 3.4;  // otvory pre špajdle zospodu v rohových stĺpikoch
skewer_depth = 12;

/* [Oči na veku krabice] */
eye_y          = 48;     // polovičná vzdialenosť očí (stred - stred = 2x)
disp_pcb       = [36.2, 45.2]; // doska displeja: šírka (vrátane presahu skla), výška
disp_thick     = 4.5;    // hrúbka modulu: doska + sklo (bez súčiastok vzadu)
disp_center_off= -0.3;   // stred skla voči stredu dosky (sklo Ø ~35,6 mm začína 5 mm od vrchu dosky)
disp_window_d  = 33.6;   // okienko pred aktívnou plochou (32,4 mm)
pod_wall       = 2;
tube_id        = 34;     // vnútorný priemer tubusu pred displejom
tube_len       = 22;     // dĺžka tubusu
visor_len      = 8;      // strieška: o koľko je vrch tubusu dlhší
pod_tilt       = -15;    // sklon displeja: záporné = nahor (tekvica leží na zemi, ľudia sa pozerajú zhora)
rod_hole_d     = 8.6;    // tyč M8 (tlačená s BOSL2 alebo kovová)
nut_af         = 13.3;   // matica M8 cez plochy (+vôľa)
nut_t6         = 6.9;    // hrúbka matice M8 (+vôľa)
keel_h         = 19;     // výška lišty pod displejom, cez ktorú ide tyč
deck_drain_w = 5;     // šírka štrbiny (rovnaká je vo veku)
deck_drain_l = 70;    // dĺžka štrbiny
deck_t_min   = 2;     // hrúbka dosky pri štrbine
keel_off       = 15;     // lišta s tyčou je posunutá do strany (dovnútra), aby pod okom ostal voľný priestor na odkvapkávanie
keel_w         = nut_af + 5;  // šírka lišty
pod_nut_trap   = true;   // false = lišta bez otvoru (závit sa vyreže v oci_zavit.scad cez BOSL2)
drain_slot     = [6, 9]; // výrez pod okom: dĺžka (dopredu), šírka
deck_t         = 3;      // doska nadstavca (slúži zároveň ako veko krabice)
pb_seg         = 28;     // dĺžka úsekov stien kolísky powerbanky
tower_h        = 10;      // výška osi tyče nad nadstavcom
pb_size        = [141, 64, 23]; // powerbanka
board_size     = [81, 51]; // Perma-Proto polovičné

$fn = 64;
eps = 0.05;

// ---------------------------------------------------------------------
// Odvodené hodnoty
// ---------------------------------------------------------------------
function s(x)    = 1 - (1 - s_tip) * x / (N - 1);
function Rr(x)   = R0 * s(x);
function Hh(i)   = H0 * s(i);
function Gg(x)   = G0 * s(x);
function rf(x)   = face_ratio * Rr(x);
function dc(x)   = (seg_style == "cone") ? Rr(x) - cable_edge : cable_ratio * Rr(x);
function bb_d(i) = (i < bb_split) ? bb_d_base : bb_d_tip;

function rn(x)   = max(neck_ratio * Rr(x), bb_d(max(0, floor(x))) / 2 + bb_clear / 2 + neck_wall);
function hct(x)  = (Rr(x) - rn(x)) * tan(a_top);   // výška horného kužeľa
function hcb(x)  = (Rr(x) - rn(x)) * tan(a_bot);   // výška spodného kužeľa
// uhlová vôľa jednej strany kĺbu (krček / okraj kužeľa)
function clr(r_n, R, a, g) = min(atan((g / 2) / r_n), atan((g / 2 + (R - r_n) * tan(a)) / R));
function jtheta(j) = (seg_style == "cone")
  ? clr(rn(max(j, 0)), Rr(max(j, 0)), a_top, Gg(j + 0.5)) + clr(rn(j + 1), Rr(j + 1), a_bot, Gg(j + 0.5))
  : 2 * atan(Gg(j + 0.5) / (2 * rf(max(j, 0))));
function total_bend(j = -1) = (j > N - 2) ? 0 : jtheta(j) + total_bend(j + 1);
function travel(j = -1)     = (j > N - 2) ? 0 : dc(j + 0.5) * jtheta(j) * PI / 180 + travel(j + 1);
function seglen(i = 0)      = (i >= N) ? 0 : Hh(i) + ((i < N - 1) ? Gg(i + 0.5) : 0) + seglen(i + 1);

travel_target = travel() * min(1, bend_target / total_bend());
pulley_r = (pulley_r_override > 0) ? pulley_r_override
         : ceil(2 * travel_target / (servo_half_range * PI / 180)) / 2;

// kladka (z = 0 na strane serva)
b0       = horn_depth + floor_t;
div_t    = 1.6;
top_t    = 1.6;
g1_z     = b0;
g2_z     = b0 + groove_w + div_t;
pulley_T = g2_z + groove_w + top_t;

// vodiaci blok pre PTFE
blk_y0   = pulley_r + flange_extra + 3;
blk_len  = 14;
blk_h    = pulley_base_z + pulley_T + 3;
shoulder = 2;          // doraz trubičky na strane kladky
zt1      = pulley_base_z + g1_z + groove_w / 2;   // spodná drážka -> vpravo (+x)
zt2      = pulley_base_z + g2_z + groove_w / 2;   // horná drážka  -> vľavo (-x)

// krabica
half   = box_in / 2;
BW     = box_in + 2 * box_wall;
port_z = round(((P_lo + P_hi) / 2) + (zt1 + zt2) / 2);

tip_h     = Hh(N - 1) * 1.4;
tip_drop  = (seg_style == "cone") ? hct(N - 1) : 0;   // špička sedí na hornom kuželi
total_len = (base_h - flange_t) + Gg(-0.5) + seglen() + (tip_type == "stick" ? 9 + stick_len : tip_h) - tip_drop;

echo(str("Chápadlo nad prírubou: ", round(total_len), " mm, max. ohyb ", round(total_bend()), " st."));
echo(str("Ťah lanka pre ", bend_target, " st. ohybu: ", round(travel_target * 10) / 10, " mm, polomer kladky: ", pulley_r, " mm"));
echo(str("Krabica: ", BW, " x ", BW, " x ", box_in_h + box_floor, " mm, os portu vo výške ", port_z, " mm nad dnom"));
echo(str("Medzikus: rovná časť ", spacer_len, " mm, ohyb ", spacer_angle, " st., vyosenie ", spacer_offset, " mm"));
// ---------------------------------------------------------------------
// Segment
// ---------------------------------------------------------------------
module bead_profile(R, H, f, n = 32) {
  polygon(concat(
    [[0, 0]],
    [for (k = [0:n]) let(t = sin(180 * k / n)) [f + (R - f) * pow(t, bulge_pow), H * k / n]],
    [[0, H]]
  ));
}

// šikmý otvor pre lanko (sleduje zužovanie chápadla) + zrazené hrany
module cable_hole(rb, rt, H) {
  hull() {
    translate([rb, 0, -eps]) cylinder(d = cable_hole_d, h = eps, $fn = 20);
    translate([rt, 0, H])    cylinder(d = cable_hole_d, h = eps, $fn = 20);
  }
  translate([rb, 0, -eps])           cylinder(d1 = chamfer_d, d2 = cable_hole_d, h = chamfer_h + eps, $fn = 20);
  translate([rt, 0, H - chamfer_h])  cylinder(d1 = cable_hole_d, d2 = chamfer_d, h = chamfer_h + eps, $fn = 20);
}

module pin_hole(z, len) {
  translate([0, 0, z]) rotate([0, 0, 45]) rotate([0, 90, 0])
    cylinder(d = pin_d, h = len, center = true, $fn = 20);
}

// profil: krček dole -> spodný kužeľ -> valec -> horný kužeľ -> krček hore
module cone_profile(i) {
  R = Rr(i); H = Hh(i); n = rn(i);
  polygon([[0, 0], [n, 0], [R, hcb(i)], [R, H - hct(i)], [n, H], [0, H]]);
}

module segment(i) {
  R = Rr(i); H = Hh(i); f = rf(i);
  zb = (seg_style == "cone") ? hcb(i) : 0;          // spodok valcovej časti
  zt = (seg_style == "cone") ? H - hct(i) : H;      // vrch valcovej časti
  difference() {
    if (seg_style == "cone")
      rotate([0, 0, seg_facets > 0 ? 45 + 180 / seg_facets : 0])
        scale(seg_facets > 0 ? [1 / cos(180 / seg_facets), 1 / cos(180 / seg_facets), 1] : [1, 1, 1])   // steny mnohouholníka na polomere R
          rotate_extrude($fn = seg_facets > 0 ? seg_facets : 96) cone_profile(i);
    else
      rotate_extrude($fn = 96) bead_profile(R, H, f);
    translate([0, 0, -1]) cylinder(d = bb_d(i) + bb_clear, h = H + 2, $fn = 40);
    for (a = [0:90:270]) rotate([0, 0, a]) {
      if (seg_style == "cone") {
        hull() {
          translate([dc(i - 0.5), 0, -eps]) cylinder(d = cable_hole_d, h = eps, $fn = 20);
          translate([dc(i + 0.5), 0, H])    cylinder(d = cable_hole_d, h = eps, $fn = 20);
        }
      } else cable_hole(dc(i - 0.5), dc(i + 0.5), H);
    }
    if (use_pin) pin_hole((zb + zt) / 2, 2 * R + 2);
    if (scallops) for (a = [45:90:315]) rotate([0, 0, a])
      translate([scallop_d * R, 0, -1]) cylinder(r = scallop_r * R, h = H + 2, $fn = 48);
  }
}

// ---------------------------------------------------------------------
// Špička (nalepí sa na posledný segment, prekryje uzlíky lanok)
// ---------------------------------------------------------------------
module tip_cap() {
  i = N - 1;
  f = (seg_style == "cone") ? Rr(i) : rf(i);
  difference() {
    rotate_extrude($fn = 96)
      polygon(concat(
        [[0, 0]],
        [for (k = [0:24]) let(t = k / 24) [f * pow(1 - t, 0.8) * (1 + 0.18 * sin(180 * t)), tip_h * t]],
        [[0, tip_h]]
      ));
    if (seg_style == "cone")   // dutina kopíruje horný kužeľ segmentu + miesto na uzlíky
      rotate_extrude($fn = 96) polygon([[0, -eps], [f - 0.7, -eps], [f - 0.7, 0.7 * tan(a_top) + 1.2],
                                        [rn(i), hct(i) + 1.2], [0, hct(i) + 1.2]]);
    else
      translate([0, 0, -eps]) cylinder(r = f - 0.8, h = 1.6, $fn = 64);   // miesto na uzlíky
  }
}

// Palička namiesto špičky: na prilepenie ruky páskou / zasunutie drôtenej kostry
// Spodok má rovnakú dutinu ako špička (sedí na poslednom segmente, kryje uzlíky).
stick_collar = 5;
stick_rim    = 1.0;   // golier paličky širší ako článok -> spodná stena ~1,7 mm (4 obvody)
stick_cone   = 4;
stick_total  = stick_collar + stick_cone + stick_len;
module tip_stick() {
  i = N - 1;
  f  = Rr(i);
  rs = stick_d / 2;
  z0 = stick_collar + stick_cone;
  difference() {
    rh = stick_hose ? stick_hose_od / 2 : rs;          // hrubšia časť okolo hadičky
    zh = stick_hose ? z0 + stick_hose_len : z0;        // koniec hadičky
    rotate_extrude($fn = 96) polygon([
      [0, 0], [f + stick_rim, 0], [f + stick_rim, stick_collar], [rh, z0], [rh, zh], [rs, zh + (rh - rs) * 1.5],
      [rs, stick_total - 1], [rs - 1, stick_total], [0, stick_total]
    ]);
    // dutina na posledný segment + uzlíky
    rotate_extrude($fn = 96) polygon([[0, -eps], [f - 0.7, -eps], [f - 0.7, 0.7 * tan(a_top) + 1.2],
                                      [rn(i), hct(i) + 1.2], [0, hct(i) + 1.2]]);
    // drážky, aby páska / lepidlo lepšie držali
    // hadička 6x4 (otvor ako v článkoch) + kolík z filamentu naprieč
    if (stick_hose) {
      translate([0, 0, -1]) cylinder(d = bb_d_tip + bb_clear, h = z0 + stick_hose_len + 1, $fn = 40);
      pin_hole(z0 + stick_hose_len / 2, stick_hose_od + 2);
    }
    for (z = [(stick_hose ? z0 + stick_hose_len + 4 : z0 + 3) : 4 : stick_total - 4]) translate([0, 0, z])
      rotate_extrude($fn = 64) translate([rs - 0.6, 0]) square([1, 1.2]);
    // otvor na drôtenú kostru prstov
    if (stick_wire > 0) translate([0, 0, stick_through ? -1 : stick_total - stick_wire_depth])
      cylinder(d = stick_wire, h = (stick_through ? stick_total : stick_wire_depth) + 2, $fn = 24);
    // priečny otvor na sťahovaciu pásku alebo drôt
    translate([0, 0, stick_total - 6]) rotate([90, 0, 0]) cylinder(d = 2.5, h = stick_d + 2, center = true, $fn = 20);
  }
}

// ---------------------------------------------------------------------
// Základňa (príruba na montáž, pätice pre PTFE trubičky zospodu)
// ---------------------------------------------------------------------
module base() {
  R = Rr(0); f = rf(0); rc = dc(-0.5);
  difference() {
    union() {
      cylinder(r = flange_r, h = flange_t, $fn = 96);
      rotate_extrude($fn = 96)
        polygon((seg_style == "cone")
          ? [[0, 0], [R, 0], [R, base_h - hct(0)], [rn(0), base_h], [0, base_h]]
          : [[0, 0], [R, 0], [R, base_h - 5], [f, base_h], [0, base_h]]);
    }
    translate([0, 0, -1]) cylinder(d = bb_d(0) + bb_clear, h = base_h + 2, $fn = 40);
    for (a = [0:90:270]) rotate([0, 0, a]) {
      translate([base_socket_r, 0, -eps]) cylinder(d = ptfe_od + ptfe_clear, h = ptfe_depth, $fn = 32);
      hull() {   // šikmý kanál od pätice k otvoru lanka
        translate([base_socket_r, 0, ptfe_depth - 1]) cylinder(d = cable_hole_d, h = eps, $fn = 20);
        translate([rc, 0, base_h + 1])                cylinder(d = cable_hole_d, h = eps, $fn = 20);
      }
      if (seg_style != "cone")
        translate([rc, 0, base_h - chamfer_h]) cylinder(d1 = cable_hole_d, d2 = chamfer_d, h = chamfer_h + eps, $fn = 20);
    }
    for (a = [45:90:315]) rotate([0, 0, a]) translate([mount_hole_r, 0, -1])
      cylinder(d = mount_hole_d, h = flange_t + 2, $fn = 24);
    if (use_pin) pin_hole(flange_t + (base_h - flange_t) / 2, 2 * R + 2);
  }
}


// ---------------------------------------------------------------------
// Dvojdrážková kladka s napínacími kolíkmi
//  spodná drážka: lanko odchádza vpravo (x = +r) smerom +Y
//  horná drážka:  lanko odchádza vľavo  (x = -r) smerom +Y
//  Lanko: drážka -> radiálny otvor -> hore na vrchnú plochu -> navinúť
//  na skrutku M2 (napínací kolík); otáčaním skrutky sa lanko napne.
// ---------------------------------------------------------------------
// vybranie pre krížový horn (+ stredová skrutka); spoločné pre kladku aj malý test
// šírka drážky dlhého ramena na polomere r (pôvodné zúženie + horn_extra)
function wl(r) = horn_w_long + (horn_w_long_tip - horn_w_long) * r / horn_long_r + horn_extra;

module horn_pocket() {
  e = horn_extra;
  translate([0, 0, -eps]) linear_extrude(horn_depth + eps) {
    circle(d = horn_hub_d, $fn = 48);
    // ramená sa ku koncu zužujú (ako na horne); dlhé ramená idú až von cez okraj
    re = horn_root_extra; r1 = horn_root_r; r2 = horn_root_r + 2;   // na 2 mm plynulý prechod
    for (a = [0, 180]) rotate(a) polygon([
      [0, -(wl(0) + re) / 2], [r1, -(wl(r1) + re) / 2], [r2, -wl(r2) / 2], [horn_long_r, -wl(horn_long_r) / 2],
      [horn_long_r, wl(horn_long_r) / 2], [r2, wl(r2) / 2], [r1, (wl(r1) + re) / 2], [0, (wl(0) + re) / 2]]);
    for (a = [90, 270]) rotate(a) polygon([[0, -horn_w_short / 2], [horn_short_r, -horn_w_short_tip / 2],
                                           [horn_short_r, horn_w_short_tip / 2], [0, horn_w_short / 2]]);
  }
  translate([0, 0, -eps]) cylinder(d = horn_hub_d, h = horn_depth + horn_boss_h, $fn = 48);
  translate([0, 0, -1]) cylinder(d = horn_screw_d, h = 50, $fn = 24);
  translate([0, 0, horn_depth + horn_boss_h + pulley_floor]) cylinder(d = horn_head_d, h = 50, $fn = 32);
}

// malý test vybrania pre horn: tenká doska namiesto celej kladky
module horn_test() {
  t = horn_depth + 0.6;   // pod drážkami len 3 vrstvy (stred je priechodný)
  difference() {
    cylinder(d = 2 * horn_long_r - 2, h = t, $fn = 96);   // dlhé drážky vyjdú von cez okraj
    horn_pocket();
  }
}

module pulley() {
  rp = pulley_r; rF = rp + flange_extra;
  ra = rp - 3.8;
  difference() {
    // drážky so sklonom 45° na strane, ktorá pri tlači (kladka hore nohami) smeruje dolu
    // -> žiadny previs; stredy drážok (výška šnúry) sú rovnaké ako predtým
    rotate_extrude($fn = 128) let(c1 = g1_z + groove_w / 2, c2 = g2_z + groove_w / 2, gd = 2.0, gb = 0.5, rG = rp + gd)
      polygon([
        [0, 0], [rG, 0], [rG, c1 - gb - gd],
        [rp, c1 - gb], [rp, c1 + gb], [rG, c1 + gb],
        [rG, c2 - gb - gd],
        [rp, c2 - gb], [rp, c2 + gb], [rG, c2 + gb],
        [rG, pulley_T], [0, pulley_T]
      ]);
    // vybranie v tvare kríža pre krížový horn zo sady serva (zuby hornu nepreklzujú,
    // kríž prenáša krútiaci moment). Dlhé ramená hornu treba skrátiť na horn_long_r.
    horn_pocket();
    for (p = [[240, g1_z, 225], [300, g2_z, 315]]) {
      rotate([0, 0, p[0]]) {
        translate([ra, 0, b0]) cylinder(d = anchor_d, h = pulley_T, $fn = 20);
        translate([ra, 0, p[1] + groove_w / 2]) rotate([0, 90, 0])
          cylinder(d = anchor_d, h = rp - ra + 1, $fn = 20);
      }
      rotate([0, 0, p[2]]) translate([7.5, 0, pulley_T - 7])
        cylinder(d = peg_pilot_d, h = 8, $fn = 16);
    }
  }
}

// ---------------------------------------------------------------------
// Vodiaci blok PTFE (lokálne: os serva v počiatku, z = 0 plocha pod úchytmi)
// Trubička sa zasúva zo zadnej strany až na doraz.
// ---------------------------------------------------------------------
module tube_block() {
  rp = pulley_r;
  difference() {
    translate([-rp - 6, blk_y0, 0]) cube([2 * rp + 12, blk_len, blk_h]);
    for (p = [[rp, zt1], [-rp, zt2]]) {
      translate([p[0], blk_y0 - 1, p[1]]) rotate([-90, 0, 0]) cylinder(d = 2.6, h = blk_len + 2, $fn = 20);
      translate([p[0], blk_y0 + shoulder, p[1]]) rotate([-90, 0, 0])
        cylinder(d = ptfe_od + ptfe_clear, h = blk_len, $fn = 32);
    }
  }
}

// notch = výrez na kábel (potrebný, keď sa servo vkladá zhora);
// both  = skrutky na oboch úchytoch (keď výrez netreba)
module servo_cut(depth, notch = true, both = false) {
  translate([shaft_offset - servo_body[0] / 2, -servo_body[1] / 2, -depth - 1])
    cube([servo_body[0], servo_body[1], depth + 2]);
  if (notch)   // výrez, ktorým pri vkladaní zhora prejde kábel vychádzajúci z boku serva
    translate([shaft_offset + servo_cable_end * (servo_body[0] / 2 + servo_cable_notch[0] / 2) - servo_cable_notch[0] / 2 - 0.5,
               -servo_cable_notch[1] / 2, -depth - 1])
      cube([servo_cable_notch[0] + 1, servo_cable_notch[1], depth + 2]);
  for (e = both ? [-1, 1] : [-servo_cable_end])
    translate([shaft_offset + e * servo_screw_spacing / 2, 0, -depth - ped_screw_len])
      cylinder(d = servo_screw_d, h = depth + ped_screw_len + 1, $fn = 16);
}

// footprint podstavca v lokálnych súradniciach
ped_xa = -(pulley_r + 6);
ped_xb = max(pulley_r + 6, shaft_offset + servo_screw_spacing / 2 + 4);
ped_ya = -servo_body[1] / 2 - ped_wall - 0.2;
ped_yb = blk_y0 + blk_len;

// dutý podstavec výšky P s okienkom na káble (strana -Y) a blokom na vrchu
// Podstavec serva: dutina sa smerom nahor zužuje pod 45° až na tvar otvoru
// pre servo (žiadne vodorovné stropy -> tlač bez podpier). Strana -Y je
// otvorená v celom profile dutiny (veľký "gotický" otvor takmer po vrch):
// šetrí materiál a je cez neň prístup ku káblu a skrutke serva.
// Pod vodiacim blokom sa dutina rozširuje dozadu tiež len pod 45°.
module pedestal(P) {
  // otvor pre servo (vrátane výrezu na kábel) = horný obdĺžnik dutiny
  tx0 = shaft_offset - servo_body[0] / 2;    // servo sa zasúva zboku, výrez na kábel netreba
  tx1 = shaft_offset + servo_body[0] / 2;
  ty0 = -servo_body[1] / 2; ty1 = servo_body[1] / 2;
  zt  = P - ped_top;                          // spodok vrchnej dosky
  zs  = zt - ped_screw_len;                   // pod skrutkou úchytu ostane plný pilier
  // spodný obdĺžnik dutiny (strana -Y otvorená, dozadu max. pod 45°)
  cx0 = ped_xa + ped_wall; cx1 = ped_xb - ped_wall;
  cy0 = ped_ya - 1;
  cy1 = min(ped_yb - ped_wall, ty1 + zs - 1);
  hf  = max(tx0 - cx0, cx1 - tx1, cy1 - ty1, 1);
  zb  = max(zs - hf, 0);
  difference() {
    translate([ped_xa, ped_ya, 0]) cube([ped_xb - ped_xa, ped_yb - ped_ya, P]);
    hull() {   // rozširujúca sa časť (45°) pod plným pilierom
      translate([cx0, cy0, -1]) cube([cx1 - cx0, cy1 - cy0, zb + 1]);
      translate([tx0, cy0, zs]) cube([tx1 - tx0, ty1 - cy0, 0.01]);
    }
    translate([tx0, cy0, zs]) cube([tx1 - tx0, ty1 - cy0, ped_top + ped_screw_len + 1]);   // zvislá šachta pre telo serva
    translate([0, 0, P]) servo_cut(ped_top, notch = false, both = true);
  }
  translate([0, 0, P]) tube_block();
}

// ---------------------------------------------------------------------
// Krabica ("kocka")
//  chápadlo A: port na stene +Y (x = -servo_x), servá na strane x < 0
//  chápadlo B: port na stene -Y (x = +servo_x), servá na strane x > 0
// ---------------------------------------------------------------------
servo_places = [   // [x, y, výška podstavca, otočenie]
  [-servo_x, y_hi,  P_hi,   0], [-servo_x, y_lo,  P_lo,   0],
  [ servo_x, -y_hi, P_hi, 180], [ servo_x, -y_lo, P_lo, 180]
];
port_places = [[-servo_x, 0], [servo_x, 180]];  // [x, otočenie]

module teardrop2d(r) { union() { circle(r = r); rotate(45) square(r); } }

module port_holes() {
  // otvor + 4 diery M3; lokálne: stena v rovine XZ, os portu = +Y
  translate([0, half + box_wall + 1, port_z]) rotate([90, 0, 0]) {
    linear_extrude(box_wall + 2) teardrop2d(sp_id / 2);
    for (a = [45:90:315]) rotate(a) translate([mount_hole_r, 0])
      linear_extrude(box_wall + 2) circle(d = mount_hole_d, $fn = 20);
  }
}

col = 9;   // rohové stĺpiky
post_leg = 8;    // dĺžka výstužných rebier pri stĺpikoch (dlhšie by zavadzali maticiam medzikusov)
post_rib = 3;    // hrúbka rebier

module win2d(w, h) {   // okno so zrazeným vrchom (tlačiteľné aj postojačky)
  c = min(w, h) * 0.25;
  polygon([[-w / 2, 0], [w / 2, 0], [w / 2, h - c], [w / 2 - c, h], [-w / 2 + c, h], [-w / 2, h - c]]);
}
module win_y(x0, x1, z0, z1)   // okno v stene +Y
  translate([(x0 + x1) / 2, half + box_wall + 1, z0]) rotate([90, 0, 0])
    linear_extrude(box_wall + 2) win2d(x1 - x0, z1 - z0);
module win_x(y0, y1, z0, z1)   // okno v stene +X
  translate([half - 1, (y0 + y1) / 2, z0]) rotate([90, 0, 90])
    linear_extrude(box_wall + 2) win2d(y1 - y0, z1 - z0);

scr_z_port = [15, box_in_h - 15];   // výšky skrutiek stien s portom
scr_z_side = [25, box_in_h - 25];   // výšky skrutiek bočných stien

// stena s portom (umiestnená na +Y, port pri x = -servo_x); tlačí sa 2x
// Háčiky na ploché káble serv na vnútornej strane stien s portom (pri dne).
// Kábel sa vkladá zhora do štrbiny medzi stenou a lemom (zmestia sa 3–4 káble na sebe).
hook_x   = [-40, -15, 15, 40];
hook_z0  = 3.5;   // spodok štrbiny nad dnom
hook_slot = 4.6;  // výška štrbiny (šírka plochého kábla ~3,8 mm)
hook_gap  = 4.2;  // hĺbka štrbiny (káble na sebe)
hook_len  = 6;    // dĺžka háčika pozdĺž steny
module cable_hooks() {
  for (hx = hook_x) translate([hx - hook_len / 2, half, 0]) {
    // spodná polička
    translate([0, -hook_gap - 1.4, hook_z0 - 1.6]) cube([hook_len, hook_gap + 1.4 + 0.01, 1.6]);
    // predný lem (hore otvorený o ~1,5 mm na zacvaknutie kábla)
    translate([0, -hook_gap - 1.4, hook_z0 - 1.6]) cube([hook_len, 1.4, 1.6 + hook_slot - 1.5]);
  }
}

module wall_port(screws = true) {
  cable_hooks();
  difference() {
    translate([-BW / 2, half, 0]) cube([BW, box_wall, box_in_h]);
    translate([-servo_x, 0, 0]) port_holes();
    if (windows) {
      win_y(2, half - 15, win_margin, box_in_h - win_margin);                 // polovica bez portu
      win_y(-servo_x - 20, -servo_x + 20, 10, port_z - flange_r - 5);         // pod portom
    }
    if (screws) for (sx = [-1, 1], z = scr_z_port)
      translate([sx * (half - col / 2), half - 1, z]) rotate([-90, 0, 0])
        cylinder(d = mount_hole_d, h = box_wall + 2, $fn = 20);
  }
}

// bočná stena (umiestnená na +X, medzi stenami s portom); tlačí sa 2x
// Lišty na vonkajšej strane zadnej bočnej steny pre Perma-Proto s Picom chápadiel.
// Doska sa zasúva zhora (spájkami k stene), dole má doraz; káble idú oknom za ňou.
pp_w   = 81;     // Perma-Proto (poloviční) – šírka
pp_h   = 51;     //                          – výška
pp_t   = 1.6;
pp_gap = 4;      // medzera medzi stenou a doskou (spájky)
pp_z0  = 18;     // spodok dosky nad dnom
pp_lip = 1.5;    // o koľko lišta prekrýva okraj dosky
pp_clr = 0.6;    // vôľa drážky na hrúbku dosky (pôvodne 0,4 – doska išla dosť ťažko)
module board_rails() {
  // obdĺžniková drážka (overená tlačou); vôľa pp_clr sa dá zväčšiť, ak doska ide ťažko
  rw = 5; rh = pp_gap + pp_t + pp_clr + 1.6;
  for (sy = [-1, 1]) mirror([0, sy > 0 ? 0 : 1, 0]) difference() {
    // lišta ide cez celú hrúbku steny a je ukotvená pod aj nad oknom
    translate([half, pp_w / 2 - pp_lip, win_margin - 5]) cube([box_wall + rh, rw + 0.3, box_in_h - 2 * win_margin + 10]);
    // drážka (zhora otvorená, dole doraz)
    translate([half + box_wall + pp_gap, pp_w / 2 - pp_lip - 0.01, pp_z0]) cube([pp_t + pp_clr, pp_lip + 0.3, box_in_h]);
  }
}

module wall_side(screws = true, rails = false) {
  if (rails) board_rails();
  difference() {
    union() {
      translate([half, -half, 0]) cube([box_wall, box_in, box_in_h]);
    }
    if (windows) win_x(-half + 15, half - 15, win_margin, box_in_h - win_margin);
    if (screws) for (sy = [-1, 1], z = scr_z_side)
      translate([half - 1, sy * (half - col / 2), z]) rotate([0, 90, 0])
        cylinder(d = mount_hole_d, h = box_wall + 2, $fn = 20);
  }
}

// dno s podstavcami a rohovými stĺpikmi
module box_floor(pilots = true) {
  difference() {
    union() {
      translate([-BW / 2, -BW / 2, -box_floor]) cube([BW, BW, box_floor]);
      for (p = servo_places) translate([p[0], p[1], 0]) rotate([0, 0, p[3]]) pedestal(p[2]);
      for (sx = [-1, 1], sy = [-1, 1]) translate([sx * (half - col / 2), sy * (half - col / 2), 0])
        translate([-col / 2, -col / 2, 0]) cube([col, col, box_in_h]);
      // výstuha stĺpikov: rebrá pozdĺž oboch stien (profil L), pri tlači sa stĺpik nekýve
      for (sx = [-1, 1], sy = [-1, 1]) {
        translate([sx > 0 ? half - col - post_leg : -half + col, sy > 0 ? half - post_rib : -half, 0])
          cube([post_leg, post_rib, box_in_h - 2]);
        translate([sx > 0 ? half - post_rib : -half, sy > 0 ? half - col - post_leg : -half + col, 0])
          cube([post_rib, post_leg, box_in_h - 2]);
      }
    }
    // skrutky veka
    for (sx = [-1, 1], sy = [-1, 1]) translate([sx * (half - col / 2), sy * (half - col / 2), box_in_h - 12])
      cylinder(d = 2.6, h = 13, $fn = 16);
    // predvŕtania pre skrutky stien
    if (pilots) for (sx = [-1, 1], sy = [-1, 1]) {
      for (z = scr_z_port) translate([sx * (half - col / 2), sy * (half + 1), z])
        rotate([sy > 0 ? 90 : -90, 0, 0]) cylinder(d = 2.6, h = 13, $fn = 16);
      for (z = scr_z_side) translate([sx * (half + 1), sy * (half - col / 2), z])
        rotate([0, sx > 0 ? -90 : 90, 0]) cylinder(d = 2.6, h = 13, $fn = 16);
    }
    // otvory pre špajdle (nožičky) zospodu v rohových stĺpikoch
    for (sx = [-1, 1], sy = [-1, 1]) translate([sx * (half - col / 2), sy * (half - col / 2), -box_floor - 1])
      cylinder(d = skewer_d, h = box_floor + skewer_depth + 1, $fn = 20);
    // dno: stred = káble + odtok, montáž M4, odtokové otvory
    translate([0, 0, -box_floor - 1]) cylinder(d = dc_spigot + 0.6, h = box_floor + 2, $fn = 32);   // odtok zo stĺpika
    for (p = [[56, 0], [-56, 0], [0, 56], [0, -56]]) translate([p[0], p[1], -box_floor - 1])
      cylinder(d = 4.5, h = box_floor + 2, $fn = 24);
    for (p = [[0, 30], [0, -30]]) translate([p[0], p[1], -box_floor - 1])
      cylinder(d = 5, h = box_floor + 2, $fn = 24);
  }
}

// celá krabica ako jeden kus
// ---------------------------------------------------------------------
// Odtokový stĺpik: žliabok pod štrbinou vo veku + rúrka do otvoru v dne.
// Stojí voľne v strede krabice (medzi podstavcami), veko ho zhora pridrží.
// ---------------------------------------------------------------------
dc_od     = 12;     // rúrka (podstavce sú od stredu 6,5 mm)
dc_id     = 8;
dc_spigot = 9;      // čap do otvoru v dne
tr_w      = deck_drain_w + 4;    // vnútorná šírka žliabku
tr_l      = deck_drain_l + 6;    // vnútorná dĺžka žliabku
tr_h      = 7;
tr_wall   = 1.4;
tr_top    = box_in_h - 0.5;      // vrch žliabku tesne pod vekom

module drain_column() {
  difference() {
    union() {
      // žliabok
      translate([-tr_w / 2 - tr_wall, -tr_l / 2 - tr_wall, tr_top - tr_h]) cube([tr_w + 2 * tr_wall, tr_l + 2 * tr_wall, tr_h]);
      // rúrka
      cylinder(d = dc_od, h = tr_top - tr_h + 1, $fn = 40);
      // čap do dna
      translate([0, 0, -box_floor + 0.3]) cylinder(d = dc_spigot, h = box_floor, $fn = 32);
    }
    // vnútro žliabku: prierez V (steny 45° -> pri tlači hore nohami bez previsu),
    // hrot klesá od koncov (2,5 mm) k rúrke v strede (4,5 mm)
    hull() for (yy = [-tr_l / 2, 0, tr_l / 2]) let(d = yy == 0 ? tr_w / 2 : deck_drain_w / 2)
      translate([0, yy, tr_top + 0.01]) rotate([90, 0, 0]) linear_extrude(0.01, center = true)
        polygon([[-d - 0.01, 0], [d + 0.01, 0], [0, -d]]);
    // vnútro rúrky a čapu
    translate([0, 0, -box_floor - 1]) cylinder(d = dc_id - 2.5, h = tr_top + box_floor + 2, $fn = 32);
    translate([0, 0, 0]) cylinder(d = dc_id, h = tr_top, $fn = 32);
  }
}

module box() {
  box_floor(false);
  for (r = [0, 180]) rotate([0, 0, r]) { wall_port(false); wall_side(false); }
}

module lid() {
  ov = 2;
  difference() {
    translate([-BW / 2 - ov, -BW / 2 - ov, 0]) cube([BW + 2 * ov, BW + 2 * ov, 2.4]);
    for (sx = [-1, 1], sy = [-1, 1]) translate([sx * (half - col / 2), sy * (half - col / 2), -1])
      cylinder(d = mount_hole_d, h = 5, $fn = 20);
    // odtoková štrbina pod nadstavcom (voda kvapká stredom krabice na odtok v dne)
    translate([-deck_drain_w / 2, -deck_drain_l / 2, -1]) cube([deck_drain_w, deck_drain_l, 5]);
  }
  // vnútorné vodiace rebrá
  for (r = [0:90:270]) rotate([0, 0, r])
    translate([-half + col + 2, half - 1.6 - 0.4, -4]) cube([box_in - 2 * col - 4, 1.6, 4]);
}

// ---------------------------------------------------------------------
// Medzikus: príruba ku krabici (z = 0) -> rovná časť -> (ohyb) -> príruba k chápadlu
// Rovné medzikusy sa dajú aj skladať za sebou.
// ---------------------------------------------------------------------
sp_off = spacer_offset > 0;
sp_L2  = (spacer_angle > 0 || sp_off) ? max(spacer_len2, flange_t + 10) : 0;
sp_a   = sp_off ? acos(1 - spacer_offset / (2 * spacer_bend_r)) : spacer_angle;  // uhol oblúka

// oblúk z počiatku smerom +Z, zatáča k +X
module arc_px(Rb, a, r) {
  translate([Rb, 0, 0]) rotate([-90, 0, 0]) rotate([0, 0, 180])
    rotate_extrude(angle = a, $fn = 96) translate([Rb, 0]) circle(r = r, $fn = 48);
}
module arc_end(Rb, a) { translate([Rb * (1 - cos(a)), 0, Rb * sin(a)]) rotate([0, a, 0]) children(); }

module sp_end() {
  Rb = spacer_bend_r;
  if (sp_off)
    translate([0, 0, spacer_len]) arc_end(Rb, sp_a) mirror([1, 0, 0]) arc_end(Rb, sp_a) mirror([1, 0, 0])
      translate([0, 0, sp_L2]) children();
  else
    translate([0, 0, spacer_len]) arc_end(Rb, sp_a) translate([0, 0, sp_L2]) children();
}

module sp_path(r, ext = 0) {
  Rb = spacer_bend_r;
  translate([0, 0, -ext]) cylinder(r = r, h = spacer_len + ext + eps, $fn = 64);
  if (sp_a > 0) translate([0, 0, spacer_len]) {
    arc_px(Rb, sp_a, r);
    // spoj dvoch oblúkov S: krátky valec po osi (nevytŕča za koniec medzikusu)
    if (sp_off) arc_end(Rb, sp_a) {
      cylinder(r = r, h = 0.6, center = true, $fn = 48);
      mirror([1, 0, 0]) arc_px(Rb, sp_a, r);
    }
  }
  sp_end() translate([0, 0, -sp_L2 - eps]) cylinder(r = r, h = sp_L2 + ext + 2 * eps, $fn = 64);
}

sp_ft    = 7;     // hrúbka príruby pri krabici
sp_floor = 4;     // plný materiál medzi maticou a dosadacou plochou (obe príruby)
module spacer() {
  cone_h = flange_r - sp_od / 2;
  difference() {
    union() {
      sp_path(sp_od / 2);
      cylinder(r = flange_r, h = sp_ft, $fn = 96);
      sp_end() {
        translate([0, 0, -flange_t]) cylinder(r = flange_r, h = flange_t, $fn = 96);
        translate([0, 0, -flange_t - cone_h]) cylinder(r1 = sp_od / 2, r2 = flange_r, h = cone_h + eps, $fn = 96);
      }
    }
    sp_path(sp_id / 2, 1);
    // príruba ku krabici: skrutka zvnútra krabice, matica 4 mm nad dosadacou plochou
    for (a = [45:90:315]) rotate([0, 0, a]) translate([mount_hole_r, 0, 0]) {
      translate([0, 0, -1]) cylinder(d = mount_hole_d, h = sp_ft + 2, $fn = 20);
      translate([0, 0, sp_floor]) cylinder(d = nut_d, h = sp_ft, $fn = 6);
    }
    // príruba k chápadlu: matica 4 mm pod dosadacou plochou, vkladá sa zboku/zospodu
    sp_end() for (a = [45:90:315]) rotate([0, 0, a]) translate([mount_hole_r, 0, 0]) {
      translate([0, 0, -sp_floor - nut_h - 1]) cylinder(d = mount_hole_d, h = sp_floor + nut_h + 2, $fn = 20);
      translate([0, 0, -flange_t - cone_h - 2]) cylinder(d = nut_d, h = flange_t + cone_h + 2 - sp_floor, $fn = 6);
    }
  }
}

// ---------------------------------------------------------------------
// OČI: držiak displeja GC9A01 s tubusom, striežkou a odtokom
//  lokálne: X dopredu (k stene tekvice), Z hore, zadná strana dosky displeja v x = 0
//  displej sa zasúva zhora; pod ním je lišta s maticou M6 pre závitovú tyč
// ---------------------------------------------------------------------
pod_pw  = disp_pcb[0] + 0.6;
pod_ph  = disp_pcb[1] + 0.6;
pod_st  = disp_thick + 0.5;     // hĺbka drážky na displej
pod_fw  = 1.6;                  // predná stena s okienkom
pod_rw  = 1.6;                  // zadný rámik
pod_zc  = disp_center_off;      // os tubusu
pod_bot = -pod_ph / 2 - pod_wall;
pod_rod_z = pod_bot - keel_h / 2 + 1.5;   // os tyče
tube_or = tube_id / 2 + pod_wall;
keel_len = pod_rw + pod_st + pod_fw;   // lišta nesmie presahovať pred držiak, inak by narazila na stenu tekvice

module eye_pod() {
  x0 = -pod_rw; xf = pod_st + pod_fw;
  difference() {
    union() {
      // telo s drážkou
      translate([x0, -pod_pw / 2 - pod_wall, pod_bot]) cube([xf - x0, pod_pw + 2 * pod_wall, pod_ph + pod_wall]);
      // tubus + strieška
      translate([xf, 0, pod_zc]) rotate([0, 90, 0]) cylinder(r = tube_or, h = tube_len, $fn = 96);
      intersection() {
        translate([xf + tube_len - eps, 0, pod_zc]) rotate([0, 90, 0]) cylinder(r = tube_or, h = visor_len, $fn = 96);
        translate([xf, -tube_or, pod_zc + 2]) cube([tube_len + visor_len, 2 * tube_or, tube_or]);
      }
      // lišta pre tyč
      translate([x0, keel_off - keel_w / 2, pod_rod_z - keel_h / 2]) cube([keel_len, keel_w, pod_bot - (pod_rod_z - keel_h / 2) + 3.5]);
    }
    // drážka na displej (otvorená zhora)
    translate([0, -pod_pw / 2, -pod_ph / 2]) cube([pod_st, pod_pw, pod_ph + 10]);
    // okienko a vnútro tubusu
    translate([pod_st - 1, 0, pod_zc]) rotate([0, 90, 0]) cylinder(d = disp_window_d, h = pod_fw + 2, $fn = 96);
    translate([xf - eps, 0, pod_zc]) rotate([0, 90, 0]) cylinder(d = tube_id, h = tube_len + visor_len + 1, $fn = 96);
    // zadný rámik: otvor pre súčiastky a plochý kábel
    translate([x0 - 1, -pod_pw / 2 + 3, -pod_ph / 2 + 3]) cube([pod_rw + 2, pod_pw - 6, pod_ph + 10]);
    // odtok kondenzátu zo spodku drážky (mimo lišty)
    for (yy = [-(pod_pw / 2 - 4), keel_off - keel_w / 2 - 2.5]) translate([pod_st / 2, yy, pod_bot - 1])   // mimo lišty
      cylinder(d = 3, h = pod_wall + 2, $fn = 20);
    // výrez pod okom: voda, ktorá stečie k okienku, odkvapká rovno dolu (lišta s tyčou je bokom)
    translate([xf - eps, -drain_slot[1] / 2, pod_zc - tube_id / 2 - pod_wall - 6])
      cube([drain_slot[0], drain_slot[1], pod_wall + 6 + 1.5]);
    if (pod_nut_trap) {
    // otvor pre tyč (sklonený podľa pod_tilt)
    translate([x0 - 1, keel_off, pod_rod_z]) rotate([0, -pod_tilt, 0]) rotate([0, 90, 0])
      cylinder(d = rod_hole_d, h = keel_len + 4, $fn = 32);
    // matica M8 (vkladá sa zospodu)
    translate([x0 + keel_len / 2 - nut_t6 / 2, keel_off - nut_af / 2, pod_rod_z - keel_h])
      cube([nut_t6, nut_af, keel_h / 2 + nut_af / 2 / cos(30) + 0.5]);
    }
  }
}

// ---------------------------------------------------------------------
// Nadstavec na veko: veže pre tyče očí, koľajnice pre dosku s Picom očí,
// kolíska pre powerbanku. Priskrutkuje sa spolu s vekom (dlhšie skrutky M3).
//  lokálne: stred veka, X dopredu (tvár), z = 0 spodok nadstavca
// ---------------------------------------------------------------------
deck_hx  = (BW + 4) / 2;
tw_x0    = deck_hx - 14;               // veža: x od tw_x0 po deck_hx
rod_z    = deck_t + tower_h;
gar_x0   = 2;
gar_len  = board_size[1] + 3;
gar_y    = board_size[0] / 2 + 0.5;    // vnútorná polovičná šírka
gar_wall = 5;
gar_h    = 4 + 1.6 + 14;               // pod doskou / doska / súčiastky
gar_tilt = 10;                          // sklon dosky (vzadu nižšie), aby z nej stekala voda
gar_gz   = rod_z + 9;                  // spodok drážky vzadu; vpredu je vyššie o gar_len*tan(gar_tilt)
board_t_deck = 1.6;
pbz_x0   = -deck_hx + 6;               // zóna powerbanky (pozdĺž Y), za lemom a háčikmi
pbz_x1   = pbz_x0 + pb_size[1] + 1;

// doska nadstavca je mierne sklonená k stredovej odtokovej štrbine
// háčik na gumičku: stĺpik s V-zárezom (45°, bez previsu) na strane odvrátenej od powerbanky
pb_pad    = 2.5;   // výška podložiek pod powerbankou (nad hlavami skrutiek)
pb_lip    = pb_pad + 1.5;   // lem 1,5 mm nad podložkami
pb_hook_y = 41;    // poloha gumičiek (pozdĺž Y)
module rb_hook(dir) {   // dir = -1: zárez smeruje k -X, +1: k +X
  hw = 6; hd = 6; hh = 14; nz = 9; nd = 1.4;   // hrubší stĺpik, plytší zárez -> pevnejší háčik
  difference() {
    cube([hw, hd, deck_t + hh]);
    translate([dir < 0 ? -0.01 : hw + 0.01, -1, deck_t + nz]) rotate([-90, 0, 0])
      linear_extrude(hd + 2) polygon([[0, -nd], [0, nd], [-dir * nd, 0]]);
  }
}

rb_boss_h = 6;     // výška nálitku nad doskou
rb_screws = [      // [x, y, priemer nálitku]
  [-deck_hx + 3, -pb_hook_y, 6], [-deck_hx + 3, pb_hook_y, 6],                   // vzadu pri leme
  [gar_x0 + 6, gar_y + 3 + 8, 8], [gar_x0 + 6, -(gar_y + 3 + 8), 8]              // vpredu pri koľajniciach
];
module eye_deck() {
  difference() {
    union() {
      for (sx = [-1, 1]) hull() {
        translate([sx > 0 ? 0 : -eps, -deck_hx, 0]) cube([eps, 2 * deck_hx, deck_t_min]);
        translate([sx * deck_hx - (sx > 0 ? eps : 0), -deck_hx, 0]) cube([eps, 2 * deck_hx, deck_t]);
      }
      // veže pre tyče
      for (sy = [-1, 1]) translate([tw_x0, sy * (eye_y - keel_off) - keel_w / 2 - 2, 0]) cube([deck_hx - tw_x0, keel_w + 4, rod_z + 9]);
      // koľajnice pre Perma-Proto s Picom očí: drážka je nad vežičkami a maticami,
      // takže doska sa dá zasunúť spredu ponad vežičky (alebo zozadu ponad oddeľovač)
      // drážka je sklonená (vzadu nižšie), aby voda z dosky stekala dozadu k odtokovej štrbine
      for (sy = [-1, 1]) translate([0, sy > 0 ? gar_y - 2 : -gar_y - gar_wall + 2, 0]) hull() {
        translate([gar_x0, 0, 0]) cube([0.01, gar_wall, gar_gz + pp_t + pp_clr + 1.8]);
        translate([gar_x0 + gar_len - 0.01, 0, 0]) cube([0.01, gar_wall, gar_gz + gar_len * tan(gar_tilt) + pp_t + pp_clr + 1.8]);
      }
      // podložky pod powerbanku: zdvihnú ju nad hlavy skrutiek a nad prípadnú vodu na veku
      for (px = [pbz_x0 + 14, pbz_x1 - 14], py = [-40, 40]) translate([px - 5, py - 5, 0]) cube([10, 10, deck_t + pb_pad]);
      // powerbanka: vzadu nízky lem, aby sa nezošuchla; drží ju gumička na háčikoch
      translate([-deck_hx, -deck_hx + 6, 0]) cube([6, 2 * deck_hx - 12, deck_t + pb_lip]);
      // nálitky pre skrutky, na ktoré sa zaháknu gumičky (hlava skrutky gumičku pridrží)
      for (p = rb_screws) translate([p[0], p[1], 0]) cylinder(d = p[2], h = deck_t + rb_boss_h, $fn = 32);
    }
    // predvŕtanie pre skrutky na gumičky
    for (p = rb_screws) translate([p[0], p[1], deck_t + rb_boss_h - 7]) cylinder(d = 2.6, h = 8, $fn = 16);
    // skrutky veka (M3 cez nadstavec aj veko do stĺpikov krabice)
    for (sx = [-1, 1], sy = [-1, 1]) translate([sx * (half - col / 2), sy * (half - col / 2), -1]) {
      cylinder(d = mount_hole_d, h = deck_t + 2, $fn = 20);
    }
    // tyč M6 + matica (vkladá sa zhora)
    for (sy = [-1, 1]) {
      translate([tw_x0 - 1, sy * (eye_y - keel_off), rod_z]) rotate([0, 90, 0]) cylinder(d = rod_hole_d, h = 20, $fn = 32);
      translate([(tw_x0 + deck_hx) / 2 - nut_t6 / 2, sy * (eye_y - keel_off) - nut_af / 2, rod_z - nut_af / 2 / cos(30) - 0.3])
        cube([nut_t6, nut_af, 20]);
    }
    // drážky pre dosku (obdĺžnikové, overené tlačou), zasúvanie pozdĺž X
    // (vzadu je drážka 1,5 mm pred koncom koľajnice uzavretá = doraz; doska sa zasúva spredu a skĺzne dozadu)
    translate([gar_x0, 0, gar_gz]) rotate([0, -gar_tilt, 0]) {
      for (sy = [-1, 1]) translate([1.5, sy > 0 ? gar_y - 2.1 : -gar_y + 2.1 - 1.8, 0])
        cube([gar_len + 10, 1.8, pp_t + pp_clr]);
      // priestor medzi koľajnicami
      translate([-1, -gar_y + 2.1, 0]) cube([gar_len + 10, 2 * gar_y - 4.2, 40]);
    }
    // odtok vody z nadstavca: stredová štrbina (pod ňou je štrbina vo veku a voľný priestor až po odtok v dne krabice)
    translate([-deck_drain_w / 2, -deck_drain_l / 2, -1]) cube([deck_drain_w, deck_drain_l, deck_t + 2]);
  }
}

// striežka nad doskou s Picom (tlačí sa vrchom dolu, bez podpier)
// ---------------------------------------------------------------------
// Plošina pre Perma-Proto s Picom očí: nad tyčami očí, na štyroch nožičkách
// priskrutkovaných zospodu cez veko. Doska sa vloží zhora do plytkého
// lôžka (stred otvorený kvôli spájkam) a pridrží sa dvoma pásikmi suchého
// zipsu cez štrbiny (mimo Pica, ktoré je v strede dosky).
// ---------------------------------------------------------------------
sh_z     = rod_z + 10.5;          // spodok plošiny nad doskou veka (nad maticami tyčí)
sh_t     = 3;
sh_x0    = 2;                     // plošina od steny kolísky po vežičky
sh_x1    = tw_x0;
sh_legs  = [[14, 47], [52, 47], [14, -47], [52, -47]];
sh_pw    = board_size[0] + 0.8;   // lôžko pre dosku (pozdĺž Y)
sh_pl    = board_size[1] + 0.8;   // (pozdĺž X)
sh_pc    = [(sh_x0 + sh_x1) / 2, 0];
module eye_board_shelf() {
  W = max(sh_pw + 8, 2 * (47 + 4));
  difference() {
    union() {
      translate([sh_x0, -W / 2, sh_z]) cube([sh_x1 - sh_x0, W, sh_t + 1.6]);
      for (l = sh_legs) translate([l[0] - 4, l[1] - 4, deck_t]) cube([8, 8, sh_z - deck_t + 0.01]);
    }
    // lôžko pre dosku
    translate([sh_pc[0] - sh_pl / 2, -sh_pw / 2, sh_z + sh_t]) cube([sh_pl, sh_pw, 5]);
    // otvorený stred (spájky), okraj 2 mm nesie dosku
    translate([sh_pc[0] - sh_pl / 2 + 2, -sh_pw / 2 + 2, sh_z - 1]) cube([sh_pl - 4, sh_pw - 4, 10]);
    // štrbiny na 2 pásiky suchého zipsu pri koncoch dosky (Pico je v strede)
    for (sy = [-1, 1], sx = [-1, 1]) translate([sh_pc[0] + sx * (sh_pl / 2 + 1.5) - 1.5, sy * 33 - 10, sh_z - 1]) cube([3, 20, 10]);
    // predvŕtanie v nožičkách (skrutky M3 zospodu cez veko)
    for (l = sh_legs) translate([l[0], l[1], deck_t - 1]) cylinder(d = 2.6, h = 12, $fn = 16);
  }
}

module eye_roof() {
  ov = 4;
  L = gar_len + 2 * ov; W = 2 * (gar_y + gar_wall) + 2 * 3;
  difference() {
    union() {
      translate([gar_x0 - ov, -W / 2, 0]) cube([L, W, 2]);
      for (sy = [-1, 1]) translate([gar_x0 - ov, sy > 0 ? W / 2 - 2 : -W / 2, -4]) cube([L, 2, 4]);   // okapy
      // prítlačné kolíky: pritlačia dosku na polička (na okraji dosky, mimo Pica)
      for (sy = [-1, 1], x = [gar_x0 + 6, gar_x0 + gar_len - 6])
        translate([x, sy * (gar_y - 2.5), -(gar_h - 4 - board_t_deck - 0.3)])
          cylinder(d = 4, h = gar_h - 4 - board_t_deck - 0.3 + 0.01, $fn = 20);
    }
    for (sy = [-1, 1], x = [gar_x0 + 6, gar_x0 + gar_len - 6])
      translate([x, sy * (gar_y + gar_wall / 2), -5]) cylinder(d = mount_hole_d, h = 8, $fn = 20);
  }
}

// ---------------------------------------------------------------------
// Samostatný držiak serva (ak chcete servo umiestniť inde)
// ---------------------------------------------------------------------
module servo_bracket() {
  x0 = ped_xa - 12; x1 = ped_xb + 6; y0 = ped_ya - 8; y1 = ped_yb;
  difference() {
    translate([x0, y0, -plate_t]) cube([x1 - x0, y1 - y0, plate_t]);
    servo_cut(plate_t);
    for (p = [[x0 + 5, y0 + 4], [x1 - 4, y0 + 4], [x0 + 5, y1 - 5]])
      translate([p[0], p[1], -plate_t - 1]) cylinder(d = mount_hole_d, h = plate_t + 2, $fn = 20);
  }
  tube_block();
}

// ---------------------------------------------------------------------
// Náhľady
// ---------------------------------------------------------------------
function seg_col(i) = [0.95 - 0.02 * i, 0.45 + 0.02 * i, 0.08];

module chain(i, frac, dir) {
  if (i < N) {
    color(seg_col(i)) segment(i);
    if (i == N - 1)
      translate([0, 0, Hh(i) - tip_drop]) color("DarkOrange") { if (tip_type == "stick") tip_stick(); else tip_cap(); }
    else
      translate([0, 0, Hh(i) + Gg(i + 0.5) / 2]) rotate([frac * jtheta(i) * dir, 0, 0])
        translate([0, 0, Gg(i + 0.5) / 2]) chain(i + 1, frac, dir);
  }
}

module tentacle(frac = 0, dir = 1) {
  color("DimGray") base();
  translate([0, 0, base_h + Gg(-0.5) / 2]) rotate([frac * jtheta(-1) * dir, 0, 0])
    translate([0, 0, Gg(-0.5) / 2]) chain(0, frac, dir);
}

module servo_dummy() {
  color("RoyalBlue") {
    translate([shaft_offset - 11.6, -6.2, -18]) cube([23.2, 12.4, 22]);
    translate([shaft_offset - 16.2, -6.2, 0]) cube([32.4, 12.4, 2.4]);
    translate([0, 0, 4]) cylinder(d = 11.8, h = pulley_base_z - 4);
  }
  color("White") translate([0, 0, pulley_base_z]) pulley();
}

module segments_plate() {
  cols = 4;
  for (i = [0:N - 1])
    translate([(i % cols) * (2 * R0 + 6), floor(i / cols) * (2 * R0 + 6), 0]) segment(i);
  translate([(N % cols) * (2 * R0 + 6), floor(N / cols) * (2 * R0 + 6), 0]) { if (tip_type == "stick") tip_stick(); else tip_cap(); }
}

// port -> lokálny rámec medzikusu (os +Z von zo steny, ohyb smerom hore)
module at_port(i) {
  p = port_places[i];
  rotate([0, 0, p[1]]) translate([p[1] == 0 ? p[0] : -p[0], half + box_wall, port_z])
    rotate([-90, 0, 0]) rotate([0, 0, -90 + spacer_rot]) children();
}

module assembly() {
  color("Gainsboro") box();
  for (p = servo_places) translate([p[0], p[1], p[2]]) rotate([0, 0, p[3]]) servo_dummy();
  for (i = [0:1]) at_port(i) {
    color("SlateGray") spacer();
    sp_end() tentacle(0.6, i == 0 ? 1 : -1);
  }
}

// ---------------------------------------------------------------------
if (part == "assembly")            assembly();
else if (part == "box")            box();
else if (part == "drain_column")   translate([0, 0, tr_top]) rotate([180, 0, 0]) drain_column();   // tlač žliabkom dolu
else if (part == "box_floor")      translate([0, 0, box_floor]) box_floor();
else if (part == "box_wall_port")  translate([0, 0, half + box_wall]) rotate([-90, 0, 0]) wall_port();
else if (part == "box_wall_side")  translate([0, 0, half + box_wall]) rotate([0, 90, 0]) wall_side();
else if (part == "box_wall_side_board") translate([0, 0, -half]) rotate([0, -90, 0]) wall_side(rails = true);   // vnútornou stranou na podložke
else if (part == "lid")            translate([0, 0, 2.4]) rotate([180, 0, 0]) lid();
else if (part == "spacer")         spacer();
else if (part == "horn_test")      translate([0, 0, horn_depth + 0.6]) rotate([180, 0, 0]) horn_test();   // tlač ako kladka (vybranie dole)
else if (part == "pulley")         translate([0, 0, pulley_T]) rotate([180, 0, 0]) pulley();   // tlač vrchom dole
else if (part == "segments_plate") segments_plate();
else if (part == "segment")        segment(seg_index);
else if (part == "base")           base();
else if (part == "tip")            tip_cap();
else if (part == "tip_stick")      tip_stick();
else if (part == "servo_bracket")  translate([0, 0, plate_t]) servo_bracket();
else if (part == "eye_deck")       eye_deck();
else if (part == "eye_shelf")      translate([0, 0, sh_z + sh_t + 1.6]) rotate([180, 0, 0]) eye_board_shelf();   // tlač plošinou dolu
else if (part == "eye_roof")       translate([0, 0, 2]) rotate([180, 0, 0]) eye_roof();
else if (part == "eye_pod_r")      mirror([0, 1, 0]) translate([0, 0, pod_rw]) rotate([0, -90, 0]) eye_pod();   // pravé oko (zrkadlové)
else if (part == "eye_pod")        translate([0, 0, pod_rw]) rotate([0, -90, 0]) eye_pod();   // tlač zadnou stranou dolu
