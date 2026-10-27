// =====================================================================
//  NÁHĽAD: celá zostava v tekvici (krabica, servá, trubičky, ruky,
//  Pico + Perma-Proto, powerbanka, kabeláž, oči, zvuk)
//  Vyžaduje chapadlo.scad v rovnakom priečinku. Len na vizualizáciu (F5).
// =====================================================================
include <chapadlo.scad>

part          = "none";   // vypne výber dielu v chapadlo.scad
spacer_offset = 30;       // S-medzikus: ruky presne oproti sebe
spacer_rot    = 90;
spacer_len    = 8;

/* [Náhľad] */
show_pumpkin = true;
show_lid     = true;   // veko + powerbanka
pumpkin_r    = 160;   // polomer tekvice
pumpkin_h    = 120;   // polovičná výška
pk_z         = port_z; // stred tekvice vo výške portov
arm_bend     = 0.32;  // ako veľmi ruky visia
$fn = 32;

// ---------------- pomocné ----------------
module wire(pts, d = 1.6, col = "Red") {
  color(col) for (k = [0:len(pts) - 2]) hull() {
    translate(pts[k]) sphere(d = d, $fn = 10);
    translate(pts[k + 1]) sphere(d = d, $fn = 10);
  }
}
module label_box(sz, col) color(col) cube(sz, center = true);

// ---------------- ruka (penová izolácia + obväz) ----------------
module hand() {
  color("Tan") {
    translate([0, 0, 14]) scale([1, 0.5, 1.25]) sphere(r = 14, $fn = 24);
    for (k = [0:3]) {
      a = -27 + k * 18; L = [40, 50, 48, 38][k];
      translate([0, 0, 26]) rotate([0, a, 0]) {
        cylinder(r = 4, h = L * 0.55, $fn = 12);
        translate([0, 0, L * 0.55]) rotate([0, 0, 0]) rotate([-25, 0, 0]) {
          sphere(r = 4, $fn = 12); cylinder(r1 = 4, r2 = 3, h = L * 0.45, $fn = 12);
        }
      }
    }
    translate([0, 0, 14]) rotate([0, -70, 0]) { cylinder(r = 4.5, h = 26, $fn = 12); }
  }
  // červená LED v dlani
  color("Red") translate([0, -7, 16]) sphere(r = 2.5, $fn = 12);
}

// prepis chain(): na konci palička + ruka
module chain(i, frac, dir) {
  if (i < N) {
    color(seg_col(i)) segment(i);
    if (i == N - 1)
      translate([0, 0, Hh(i) - tip_drop]) { color("DarkOrange") tip_stick(); translate([0, 0, stick_total]) hand(); }
    else
      translate([0, 0, Hh(i) + Gg(i + 0.5) / 2]) rotate([frac * jtheta(i) * dir, 0, 0])
        translate([0, 0, Gg(i + 0.5) / 2]) chain(i + 1, frac, dir);
  }
}

// ---------------- elektronika ----------------
lid_top  = box_in_h;                // nadstavec leží priamo na krabici (je zároveň vekom)
deck_top = lid_top + deck_t;        // vrch nadstavca
pb_c     = [(pbz_x0 + pbz_x1) / 2, 0, deck_top + pb_size[2] / 2];
board_x  = -(half + box_wall) - 6;  // Perma-Proto chápadiel na vonkajšej strane zadnej steny
board_c  = [board_x, 0, 42];
pod_x    = deck_hx + 32;            // poloha zadnej strany držiaka oka na tyči
rod_wz   = lid_top + rod_z;         // výška osi tyčí

module display_dummy() {            // displej v držiaku (lokálne súradnice držiaka)
  color("RoyalBlue") translate([0.2, -disp_pcb[0] / 2, -disp_pcb[1] / 2]) cube([1.6, disp_pcb[0], disp_pcb[1]]);
  color("Black") translate([1.8, 0, pod_zc]) rotate([0, 90, 0]) cylinder(d = 37, h = 1.5);
  color("White") translate([3.3, 0, pod_zc]) rotate([0, 90, 0]) cylinder(d = 30, h = 0.3);
  color("Black") translate([3.5, 0, pod_zc]) rotate([0, 90, 0]) cylinder(d = 11, h = 0.3);
}
module pod_at(sy) {
  translate([pod_x, sy * eye_y, rod_wz]) rotate([0, pod_tilt, 0]) translate([pod_rw, 0, -pod_rod_z]) mirror([0, sy > 0 ? 1 : 0, 0]) children();
}

module electronics() {
  if (show_lid) {
    // nadstavec, striežka, powerbanka so suchým zipsom
    color("LightSteelBlue") translate([0, 0, lid_top]) eye_deck();
    translate(pb_c) {
      label_box([pb_size[1], pb_size[0], pb_size[2]], "DimGray");
      color("Black") for (y = [-35, 35]) translate([0, y, 0]) cube([pb_size[1] + 6, 20, pb_size[2] + 1.5], center = true);
    }
    // Perma-Proto s Picom očí v koľajniciach nad tyčami
    translate([0, 0, lid_top]) {
      translate([gar_x0 + 1.5, 0, gar_gz]) rotate([0, -gar_tilt, 0]) translate([board_size[1] / 2, 0, 0.8]) {
        color("SeaGreen") cube([board_size[1], board_size[0], 1.6], center = true);
        translate([0, 0, 6]) color("DarkGreen") cube([21, 51, 1.2], center = true);
        translate([0, 0, 3]) color("Black") cube([18, 51, 4], center = true);
      }
    }
  }
  // zadná bočná stena má lišty pre Perma-Proto s Picom chápadiel (-X); predná je voľná (ústa, svetlo)
  for (r = [180]) rotate([0, 0, r]) {
    color("SlateGray") board_rails();
    translate([half + box_wall + pp_gap + 0.8, 0, pp_z0 + pp_h / 2]) {
      color("SeaGreen") cube([1.6, pp_w, pp_h], center = true);
      translate([5, 0, 0]) color("DarkGreen") cube([1.2, 51, 21], center = true);
      translate([3, 0, 0]) color("Black") cube([4, 51, 18], center = true);
    }
  }
  // tyče M6 + držiaky očí s displejmi
  for (sy = [-1, 1]) {
    color("Silver") translate([tw_x0 - 2, sy * (eye_y - keel_off), rod_wz]) rotate([0, 90, 0]) cylinder(d = 6, h = pod_x - tw_x0 + keel_len + 4, $fn = 16);
    color("Silver") translate([deck_hx + 0.5, sy * (eye_y - keel_off), rod_wz]) rotate([0, 90, 0]) cylinder(d = 11, h = 5, $fn = 6);   // kontramatica
    pod_at(sy) { color("DarkOrange") eye_pod(); display_dummy(); }
  }
}

// ---------------- kabeláž ----------------
bx = board_c[0]; bz = board_c[2];
module cabling() {
  // USB z powerbanky: jeden výstup do dosky chápadiel, druhý do dosky očí
  wire([[pbz_x0 + 4, -pb_size[0] / 2 + 3, deck_top + 8], [pbz_x0 - 6, -60, deck_top + 4], [bx - 2, -30, lid_top],
        [bx - 2, -25, bz + 28]], 3, "Black");
  wire([[pbz_x1 - 4, pb_size[0] / 2 - 3, deck_top + 8], [pbz_x1 + 2, 60, deck_top + 26], [gar_x0 + 10, 30, deck_top + 12]], 3, "Black");
  // káble serv (z okienok podstavcov cez okno zadnej steny k doske)
  for (k = [0:3]) let(p = servo_places[k], s = p[3] == 0 ? 1 : -1,
                      wx = p[0] + s * shaft_offset, wy = p[1] - s * (servo_body[1] / 2 + 4))
    wire([[wx, wy, 6], [wx, wy - s * 8, 6], [-half + 4, (k - 1.5) * 12, 30],
          [bx + 3, (k - 1.5) * 8, bz - 10]],
         1.8, ["Orange", "Gold", "Orange", "Gold"][k]);
  // UART medzi Picami (zadná stena -> nadstavec)
  wire([[bx, 20, bz + 20], [bx - 4, 20, lid_top + 6], [gar_x0 + 2, 20, deck_top + 8]], 2, "Purple");
  // káble k displejom (z dosky dopredu k držiakom)
  for (sy = [-1, 1])
    wire([[gar_x0 + board_size[1], sy * 15, deck_top + 12], [deck_hx, sy * 30, deck_top + 40],
          [pod_x + 2, sy * eye_y, rod_wz + 52]], 3.2, "Plum");
  // LED do rúk (popri trubičkách cez porty)
  for (i = [0:1]) let(sg = i == 0 ? 1 : -1, px = i == 0 ? -servo_x : servo_x)
    wire([[bx + 4, sg * 10, bz + 5], [-half + 6, sg * 40, 78], [px, sg * (half - 6), port_z + 14],
          [px, sg * (half + box_wall + 2), port_z + 3]], 1.2, "Red");
}

// ---------------- PTFE trubičky ----------------
function bexit(p, side) =   // výstup z vodiaceho bloku: side +1 -> spodná drážka (zt1), -1 -> horná (zt2)
  let(s = p[3] == 0 ? 1 : -1)
  [p[0] + s * side * pulley_r, p[1] + s * ped_yb, p[2] + (side > 0 ? zt1 : zt2)];
module ptfe() {
  // A: servá 0 (vysoké) a 1 (nízke), port +Y pri x = -servo_x; B: 2, 3, port -Y
  for (i = [0:1]) {
    s  = i == 0 ? 1 : -1;
    px = -s * servo_x;
    wy = s * (half + box_wall + 2);
    hi = servo_places[2 * i]; lo = servo_places[2 * i + 1];
    // vysoké servo -> ľavý/pravý otvor, nízke -> horný/dolný (protiľahlé páry)
    for (sd = [-1, 1]) {
      e = bexit(hi, sd);
      wire([e, [e[0], e[1] + s * 12, e[2]], [px + sd * s * base_socket_r, wy - s * 10, port_z], [px + sd * s * base_socket_r, wy, port_z]], 4, "WhiteSmoke");
      f = bexit(lo, sd);
      wire([f, [f[0], f[1] + s * 6, f[2] + 4], [px, wy - s * 6, port_z + sd * base_socket_r], [px, wy, port_z + sd * base_socket_r]], 4, "WhiteSmoke");
    }
  }
}

// ---------------- tekvica ----------------
module pumpkin() {
  color([1.0, 0.45, 0.05, 0.16])
    for (a = [0:30:150]) rotate([0, 0, a]) scale([1, 0.9, 1]) translate([0, 0, pk_z])
      scale([pumpkin_r, pumpkin_r, pumpkin_h]) sphere(r = 1, $fn = 40);
  // vrchnák (rez) a stopka
  color([0.25, 0.12, 0, 0.6]) translate([0, 0, pk_z + pumpkin_h * 0.82])
    rotate_extrude($fn = 60) translate([62, 0]) circle(r = 1.5, $fn = 8);
  color("DarkOliveGreen") translate([0, 0, pk_z + pumpkin_h - 6]) cylinder(r1 = 12, r2 = 8, h = 40);
  // vyrezané oči (otvory, do ktorých ústi tubus)
  for (sy = [-1, 1]) pod_at(sy) color([0.1, 0.05, 0, 0.7])
    translate([pod_st + pod_fw + tube_len + 2, 0, pod_zc]) rotate([0, 90, 0])
      difference() { cylinder(r = tube_or + 6, h = 2); translate([0, 0, -1]) cylinder(r = tube_id / 2, h = 4); }
  // ústa
  color([0.1, 0.05, 0, 0.8]) translate([pumpkin_r - 6, 0, pk_z - 30]) rotate([0, 90, 0])
    linear_extrude(4) scale([1, 2.2]) difference() { circle(r = 25, $fn = 40); translate([-10, 0]) circle(r = 26, $fn = 40); }
}

// ---------------- scéna ----------------
module scene() {
  color("Gainsboro") box();
  color("DeepSkyBlue") drain_column();
  for (p = servo_places) translate([p[0], p[1], p[2]]) rotate([0, 0, p[3]]) servo_dummy();
  for (i = [0:1]) at_port(i) {
    color("SlateGray") spacer();
    sp_end() rotate([0, 0, i == 0 ? -45 : 45]) tentacle(arm_bend, -1);
  }
  ptfe();
  electronics();
  cabling();
  // špajdle
  color("BurlyWood") for (sx = [-1, 1], sy = [-1, 1])
    translate([sx * (half - col / 2), sy * (half - col / 2), -pk_z - 10]) cylinder(d = 3, h = pk_z + 18, $fn = 8);
  if (show_pumpkin) pumpkin();
}

scene();
