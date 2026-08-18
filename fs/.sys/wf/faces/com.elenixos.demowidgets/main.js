const view = eos.view.active();

const SW = eos.DISPLAY_WIDTH;
const SH = eos.DISPLAY_HEIGHT;

// ── Palette ───────────────────────────────────────────────────
const BG = 0x080C12;
const RING = 0x1A2030;
const TICK_MAJOR = 0xB8C0CC;
const TICK_MINOR = 0x2E3640;
const DIGIT = 0xD4DAE4;
const CAP_OUTER = 0xD0D8E4;
const CAP_INNER = 0xFF4040;

function makeStatic(obj) {
    obj.removeFlag(lv.OBJ_FLAG_SCROLLABLE);
    obj.removeFlag(lv.OBJ_FLAG_CLICKABLE);
    obj.removeFlag(lv.OBJ_FLAG_CLICK_FOCUSABLE);
}

// ── Root background ────────────────────────────────────────────
const root = eos.ww.background(view, SW, SH, 0, BG);

// ── Top status row ─────────────────────────────────────────────
const top = new lv.obj(root);
top.setSize(SW, 60);
top.setPos(0, 8);
top.setStyleBgOpa(0, lv.PART_MAIN);
top.setStylePadAll(0, lv.PART_MAIN);
top.setFlexFlow(lv.FLEX_FLOW_ROW);
top.setFlexAlign(lv.FLEX_ALIGN_SPACE_EVENLY, lv.FLEX_ALIGN_CENTER, lv.FLEX_ALIGN_CENTER);
makeStatic(top);

const digitalClock = eos.ww.digitalClock(top);
const date = eos.ww.date(top);
const dateWindow = eos.ww.dateWindow(top);
const weekdayRing = eos.ww.weekdayRing(top, 0x5A6472, 0xFFFFFF);

// ── Analog clock face ─────────────────────────────────────────
const FACE_RADIUS = 120;
const FACE_W = FACE_RADIUS * 2;
const FACE_X = Math.floor((SW - FACE_W) / 2);
const FACE_Y = Math.floor((SH - FACE_W) / 2) - 10;

const face = new lv.obj(root);
face.setSize(FACE_W, FACE_W);
face.setPos(FACE_X, FACE_Y);
face.setStyleBgOpa(0, lv.PART_MAIN);
face.setStyleBorderWidth(3, lv.PART_MAIN);
face.setStyleBorderColor(lv.color.hex(RING), lv.PART_MAIN);
face.setStyleRadius(FACE_RADIUS, lv.PART_MAIN);
face.setStylePadAll(0, lv.PART_MAIN);
makeStatic(face);

// Decorative rings built by the C widgets
eos.ww.tickRing(face, FACE_RADIUS, 60, 5, 14, 16, 8, 3, 1, TICK_MAJOR, TICK_MINOR);
eos.ww.numeralRing(face, FACE_RADIUS, 12, FACE_RADIUS - 42, 40, DIGIT);

// Hands (styled lv.obj driven by eos.clockHand)
function makeHand(parent, type, length, width, color) {
    const hand = new lv.obj(parent);
    hand.setSize(width, length);
    hand.setStyleBgColor(lv.color.hex(color), lv.PART_MAIN);
    hand.setStyleBgOpa(255, lv.PART_MAIN);
    hand.setStyleBorderWidth(0, lv.PART_MAIN);
    hand.setStyleRadius(Math.ceil(width / 2), lv.PART_MAIN);
    hand.setStylePadAll(0, lv.PART_MAIN);
    hand.setStyleTransformPivotX(Math.floor(width / 2), lv.PART_MAIN);
    hand.setStyleTransformPivotY(Math.floor(length * 0.85), lv.PART_MAIN);
    eos.clockHand.attach(hand, type);
    hand.setPos(FACE_RADIUS - Math.floor(width / 2), FACE_RADIUS - Math.floor(length * 0.85));
    makeStatic(hand);
    return hand;
}

makeHand(face, eos.CLOCK_HAND_HOUR, Math.floor(FACE_RADIUS * 0.56), 10, 0xE4E8F0);
makeHand(face, eos.CLOCK_HAND_MINUTE, Math.floor(FACE_RADIUS * 0.80), 6, 0xC0C8D4);
makeHand(face, eos.CLOCK_HAND_SECOND, Math.floor(FACE_RADIUS * 0.93), 2, 0xFF3B3B);

eos.ww.centerCap(face, 14, 6, CAP_OUTER, CAP_INNER);

// ── Status widget grid (bottom) ────────────────────────────────
const grid = new lv.obj(root);
grid.setSize(SW, 100);
grid.setPos(0, SH - 108);
grid.setStyleBgOpa(0, lv.PART_MAIN);
grid.setStylePadAll(0, lv.PART_MAIN);
grid.setFlexFlow(lv.FLEX_FLOW_ROW_WRAP);
grid.setFlexAlign(lv.FLEX_ALIGN_SPACE_EVENLY, lv.FLEX_ALIGN_CENTER, lv.FLEX_ALIGN_CENTER);
makeStatic(grid);

const battery = eos.ww.battery(grid);
const charging = eos.ww.charging(grid);
const heartRate = eos.ww.heartRate(grid);
const steps = eos.ww.steps(grid);
const spo2 = eos.ww.spo2(grid);
const temperature = eos.ww.temperature(grid);
const barometer = eos.ww.barometer(grid);
const moonPhase = eos.ww.moonPhase(grid);

// ── Rings / arcs (left side) ───────────────────────────────────
const arcs = new lv.obj(root);
arcs.setSize(120, 240);
arcs.setPos(8, FACE_Y);
arcs.setStyleBgOpa(0, lv.PART_MAIN);
arcs.setStylePadAll(0, lv.PART_MAIN);
arcs.setFlexFlow(lv.FLEX_FLOW_COLUMN);
arcs.setFlexAlign(lv.FLEX_ALIGN_SPACE_AROUND, lv.FLEX_ALIGN_CENTER, lv.FLEX_ALIGN_CENTER);
makeStatic(arcs);

const batteryArc = eos.ww.batteryArc(arcs, 56, 0x1A2030);
const heartRateArc = eos.ww.heartRateArc(arcs, 56, 0x1A2030);
const compass = eos.ww.compass(arcs, 56, 0xE4E8F0);

// ── Activity rings + progress ring (right side) ────────────────
const right = new lv.obj(root);
right.setSize(120, 240);
right.setPos(SW - 128, FACE_Y);
right.setStyleBgOpa(0, lv.PART_MAIN);
right.setStylePadAll(0, lv.PART_MAIN);
right.setFlexFlow(lv.FLEX_FLOW_COLUMN);
right.setFlexAlign(lv.FLEX_ALIGN_SPACE_AROUND, lv.FLEX_ALIGN_CENTER, lv.FLEX_ALIGN_CENTER);
makeStatic(right);

const activityRings = eos.ww.activityRings(right, 84, 0x1A2030);
const progressRing = eos.ww.progressRing(right);
eos.ww.progressRingSetRange(progressRing, 0, 100);
eos.ww.progressRingSetValue(progressRing, 65);
