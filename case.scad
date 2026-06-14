// ---------------------------------------------------------------------------
// MiniVoltMon 3D-printed case  (two halves, M2 corner screws)
//
// Orientation (looking down into the bottom half):
//   +X (right)  = red/black screw-terminal end.  Opening spans the right
//                 wall AND the lid so wires go in from the side and the
//                 terminal screws are reachable from the top.
//   +Y (back)   = "far side": USB-C opening in the back wall.
//   Lid has a cutout for the slide switch and two holes for the push
//   buttons next to the USB-C connector.
//
// Side walls are 4 mm (M2 screws drive into the corners); floor and lid 3 mm.
//
// !! Dimensions in the "MEASURE AND ADJUST" block were estimated from
// !! "Board Image.jpg" (scaled off the ESP32-C3 SuperMini, 22.5 x 17.8 mm).
// !! Verify them with calipers before printing.
// ---------------------------------------------------------------------------

PART = "both";   // "bottom" | "lid" | "both" (print layout) | "assembled"

$fn = 18;
eps = 0.1;       // overlap fudge for clean difference() cuts

// ----------------------- MEASURE AND ADJUST -------------------------------
board_len = 56;     // total assembly length, left edge of ESP32 -> right edge of perfboard
board_w   = 23;     // widest point (ESP32-C3 SuperMini, ~22.5 mm)
board_t   = 1.6;    // PCB thickness
cavity_h  = 12;     // floor top -> lid underside; must clear the screw terminal
                    // (terminal is ~9-10 mm above the perfboard)

// Screw terminal (right end), opening in right wall + lid
term_w        = 10;   // Y width of the opening
term_open_z   = 4;    // wall notch starts this far above the floor (~proto board top)
term_lid_depth = 8;  // how far the lid cutout reaches in from the right inner wall

// USB-C (back wall).  X position measured from the LEFT edge of the board.
usb_cx = 9;           // center of USB-C connector
usb_w  = 10.5;        // opening width
usb_z  = 4.5;           // bottom of the USB-C connector above the inside floor
usb_h  = 4.5;         // opening height (connector is ~3.3 mm + clearance)

// Slide switch (lid cutout).  Position from board origin (left-front corner).
sw_cx = 24;           // center X
sw_cy = 5;            // center Y (switch is near the FRONT edge of the perfboard)
sw_l  = 6;           // cutout size along X
sw_w  = 3;            // cutout size along Y

// Push buttons (lid holes).  Positions from board origin.
btn_d  = 4.5;         // hole diameter
btn1_x = 7;           // first button center X
btn2_x = 12;          // second button center X
btn_y  = 14;          // both buttons' center Y (~9 mm in from the back edge)
btn_top_z = 7.5;      // floor -> top of the push buttons (ESP32 PCB top + ~2.5)

// Board hold: locating ribs (floor) + hold-down ribs (lid underside)
esp_t = 1.0;          // ESP32 SuperMini PCB thickness (perfboard uses board_t)
esp_len = 17.8;       // ESP32 extent along X from the board's left edge
perf_inset_front = 1.0; // perfboard front edge sits this far inside the board envelope
rib_clear = 0.3;      // gap between rib face and PCB edge
rib_w = 4;            // rib width along X (front/perfboard ribs)
back_rib_w = 3.5;     // rib width along X (back/ESP32 ribs)
front_rib_xs = [28, 43];   // board-X spots on the perfboard FRONT edge
                           // (keep clear of the slide switch and terminal)
back_rib_xs  = [1.8, 16.2]; // board-X spots on the ESP32 BACK edge,
                            // either side of the USB connector
// ---------------------------------------------------------------------------

// ------------------------------- case body --------------------------------
wall  = 4;            // side wall thickness
floor_t = 3;          // bottom thickness
lid_t = 3;            // lid thickness
round_r = 0.8;        // rounding radius on the outside corners

gap_side = 0.8;       // clearance board edge -> side walls
gap_end  = 1.5;       // clearance board end -> end walls

// Inner shelf: a ledge rising off the floor along the inside of the walls,
// running the left, front and back walls but NOT the right (terminal) wall.
shelf_h = 2;          // height above the floor
shelf_w = 3;          // how far it protrudes inward from the wall

// M2 screws — driven straight into the wall corners (no posts in the cavity)
pilot_d  = 1.8;       // self-tap pilot hole down the corner walls
pilot_depth = 8;
screw_clr_d = 2.4;    // clearance hole in lid
cb_d     = 4.2;       // M2 pan-head diameter (used for edge clearance)

// Button plungers (free-floating pins that ride in the lid holes)
plunger_d     = btn_d - 0.5;  // sliding fit in the lid hole
plunger_proud = 0.8;          // how far the head sticks out above the lid at rest
flange_t = 1.2;               // retaining flange at the bottom of the stem
flange_x = plunger_d;         // no overhang along X (plungers sit 5 mm apart)
flange_y = 6.0;               // overhangs the hole along Y so it can't fall out

// Plunger guide sleeves: tubes hanging off the lid underside around each
// button hole, to keep the plungers riding straight.
sleeve_h    = 3;              // how far the sleeve reaches down into the cavity
sleeve_wall = 1.0;            // sleeve wall thickness

// ------------------------------- derived ----------------------------------
inner_l = board_len + 2*gap_end;
inner_w = board_w   + 2*gap_side;
outer_l = inner_l + 2*wall;
outer_w = inner_w + 2*wall;
bot_h   = floor_t + cavity_h;

bx0 = wall + gap_end;    // board origin (left-front corner of board) in case coords
by0 = wall + gap_side;

screw_inset = cb_d/2 + 0.3;         // screws nudged inward so the counterbore
                                    // stays 0.3 clear of the lid's outer edges
screw_pos = [
    [screw_inset,           screw_inset],
    [outer_l - screw_inset, screw_inset],
    [screw_inset,           outer_w - screw_inset],
    [outer_l - screw_inset, outer_w - screw_inset]
];

term_cy = by0 + board_w/2;          // terminal opening centered on the board
back_wall_y = wall + inner_w;       // inner face of the back wall


plunger_len = (cavity_h - btn_top_z) + lid_t + plunger_proud;

// Board stack (no standoffs): the proto board rests on its underside solder
// bumps, the ESP32 sits directly on the proto board, and the USB-C shell
// sits on the ESP32 PCB.  usb_z (= ESP32 PCB top) fixes the whole stack.
esp_top  = usb_z;                   // ESP32 PCB top above the inside floor
esp_bot  = usb_z - esp_t;           // ESP32 PCB bottom
perf_top = esp_bot;                 // proto board top
perf_bot = perf_top - board_t;      // proto board bottom (solder bumps below)

echo(str("Outer size: ", outer_l, " x ", outer_w, " x ", bot_h + lid_t, " mm"));

// ------------------------------- modules ----------------------------------
// Outer shell with rounded vertical edges and one rounded face (bottom of
// the base, top of the lid).  The mating face stays square so the two
// halves still sit flush against each other.
module rshell(l, w, h, r, round_top) {
    hull()
        for (x = [r, l - r], y = [r, w - r]) {
            translate([x, y, round_top ? h - r : r]) sphere(r = r);
            translate([x, y, round_top ? 0 : h - 0.5]) cylinder(r = r, h = 0.5);
        }
}

module bottom() {
    difference() {
        union() {
            // shell
            difference() {
                rshell(outer_l, outer_w, bot_h, round_r, false);
                translate([wall, wall, floor_t])
                    cube([inner_l, inner_w, cavity_h + eps]);
            }

            // inner shelf: ledge around the floor on the left/front/back walls,
            // open on the right (terminal) wall.  Built as the floor footprint
            // minus an inner cutout that runs out through the right wall.

            difference() {
                translate([wall, wall, floor_t])
                    cube([inner_l, inner_w, shelf_h]);
                translate([wall + shelf_w, wall + shelf_w, floor_t - eps])
                    cube([inner_l - shelf_w + eps, inner_w - 2*shelf_w, shelf_h + 2*eps]);
            }

            // --- floor locating ribs (stop the board sliding around) ---
            // left end: ESP32 left edge (rib face only meets the PCB edge,
            // 0.3 mm shy of the board, so nothing below the board hits it)
            translate([wall, by0 + board_w/2 - 5, floor_t])
                cube([gap_end - rib_clear, 10, esp_top - 0.2 + shelf_h]);
            // right end: perfboard right edge (stays below the terminal notch)
            translate([wall + inner_l - (gap_end - rib_clear), term_cy - 6, floor_t])
                cube([gap_end - rib_clear, 12, term_open_z - 0.2 + shelf_h]);
            // front wall: perfboard front edge
            for (x = front_rib_xs)
                translate([bx0 + x - rib_w/2, wall, floor_t])
                    cube([rib_w, gap_side + perf_inset_front - rib_clear, perf_top - 0.2 + shelf_h]);
            // back wall: ESP32 back edge, either side of the USB connector
            for (x = back_rib_xs)
                translate([bx0 + x - back_rib_w/2, back_wall_y - (gap_side - rib_clear), floor_t])
                    cube([back_rib_w, gap_side - rib_clear, esp_top - 0.2 + shelf_h]);
            // back wall: perfboard back edge, across from the right-hand front rib
            // (protrudes 2 mm further into the cavity than the other back ribs)
            translate([bx0 + front_rib_xs[1] - rib_w/2, back_wall_y - (gap_side - rib_clear) - 2, floor_t])
                cube([rib_w, gap_side - rib_clear + 2, perf_top - 0.2 + shelf_h]);

        }

        // screw-terminal opening in the right wall (open through the top edge)
        translate([outer_l - wall - eps, term_cy - term_w/2, floor_t + term_open_z])
            cube([wall + 2*eps, term_w, bot_h]);

        // USB-C opening in the back (far) wall (connector bottom at usb_z;
        // the cut starts 0.5 below it for clearance)
        translate([bx0 + usb_cx - usb_w/2, outer_w - wall - eps, floor_t + usb_z - 0.5])
            cube([usb_w, wall + 2*eps, usb_h + 0.5]);

        // pilot holes straight down the wall corners
        for (p = screw_pos)
            translate([p[0], p[1], bot_h - pilot_depth])
                cylinder(d = pilot_d, h = pilot_depth + eps);
    }
}

module lid() {
    difference() {
        union() {
            rshell(outer_l, outer_w, lid_t, round_r, true);

            // plunger guide sleeves: hang down from the lid underside (local
            // -z) around each button hole to keep the plungers riding straight
            for (bx = [btn1_x, btn2_x])
                translate([bx0 + bx, by0 + btn_y, -sleeve_h])
                    cylinder(d = btn_d + 2*sleeve_wall, h = sleeve_h + eps);
        }

        // M2 clearance holes (pan heads sit proud on the lid surface)
        for (p = screw_pos)
            translate([p[0], p[1], -eps])
                cylinder(d = screw_clr_d, h = lid_t + 2*eps);

        // screw-terminal access: cutout meets the right-wall notch so the
        // combined opening is L-shaped (insert wires from the right,
        // tighten screws from the top)
        translate([outer_l - wall - term_lid_depth, term_cy - term_w/2, -eps])
            cube([wall + term_lid_depth + eps, term_w, lid_t + 2*eps]);

        // slide switch window
        translate([bx0 + sw_cx - sw_l/2, by0 + sw_cy - sw_w/2, -eps])
            cube([sw_l, sw_w, lid_t + 2*eps]);

        // push-button holes (bored through the lid and the guide sleeves)
        for (bx = [btn1_x, btn2_x])
            translate([bx0 + bx, by0 + btn_y, -sleeve_h - eps])
                cylinder(d = btn_d, h = lid_t + sleeve_h + 2*eps);
    }
}

// Free-floating button plunger: drop into the lid hole from the inside
// before screwing the lid down.  The bottom flange keeps it captive;
// gravity rests it on the push button.
module plunger() {
    // flat-topped stem, 2 mm taller than the nominal length
    cylinder(d = plunger_d, h = plunger_len + 2);
    translate([-flange_x/2, -flange_y/2, 0])
        cube([flange_x, flange_y, flange_t]);
}

// ------------------------------- output -----------------------------------
if (PART == "bottom") {
    bottom();
} else if (PART == "lid") {
    // printed top-face-down: flat outside surface on the bed
    translate([outer_l, 0, lid_t]) rotate([0, 180, 0]) lid();
} else if (PART == "plungers") {
    // printed flange-down: flat flange on the bed, domed tip up
    for (i = [0, 1])
        translate([0, i * (flange_y + 4), 0])
            plunger();
} else if (PART == "assembled") {
    bottom();
    color("steelblue", 0.5) translate([0, 0, bot_h]) lid();
    color("tomato")
        for (bx = [btn1_x, btn2_x])
            translate([bx0 + bx, by0 + btn_y, floor_t + btn_top_z])
                plunger();
} else { // "both" — print layout, parts side by side
    bottom();
    translate([0, outer_w + 8, 0])
        translate([outer_l, 0, lid_t]) rotate([0, 180, 0]) lid();
    for (i = [0, 1])
        translate([outer_l + 8, outer_w + 12 + i * (flange_y + 4), 0])
            plunger();
}
