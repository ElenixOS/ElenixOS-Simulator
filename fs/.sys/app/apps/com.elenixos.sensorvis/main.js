// ==========================================================================
//  Sensor Visualization App — ElenixOS
//  Redesigned with modern dark theme, gradients & shadows
// ==========================================================================

const view = eos.view.active();
const SW = eos.DISPLAY_WIDTH;   // 390
const SH = eos.DISPLAY_HEIGHT;

const currentActivity = eos.activity.current();
eos.activity.setAppHeaderVisible(currentActivity, false);

// ── Palette ───────────────────────────────────────────────────
const C = {
    BG:             0x060A0F,
    BG_GRAD:        0x030508,
    CARD_BG:        0x101A24,
    CARD_BG2:       0x0A1218,
    CARD_BORDER:    0x1E2A38,
    PANEL_BG:       0x161E28,
    TEXT_PRIMARY:   0xE8ECF2,
    TEXT_SECONDARY: 0x6B7686,
    ACCENT:         0x00D4FF,
    ACCENT2:        0xFF6B6B,
    SKY:            0x2A6BB8,
    SKY_GRAD:       0x1A4080,
    GROUND:         0x6B3F1A,
    GROUND_GRAD:    0x3A2810,
    GREEN:          0x30D158,
    YELLOW:         0xFFD60A,
    RED:            0xFF453A,
    ORANGE:         0xFF9F0A,
    WHITE:          0xFFFFFF,
    RING_DARK:      0x2A3444,
};

const CARD_W = SW - 24;  // 366

// The tiny-TTF font renders only three sizes — eos.FONT_SIZE_SMALL (22px),
// eos.FONT_SIZE_MEDIUM (26px), eos.FONT_SIZE_LARGE (30px) — and setFontSize()
// maps any value to the nearest of those. Its line height is ~1.448x the font
// size (Source Han Sans metrics), so a label needs more vertical room than its
// nominal font size: a fixed height smaller than the line height clips glyphs
// at the bottom. Use these when sizing/positioning text labels.
const LH_SMALL  = 32;  // line height of eos.FONT_SIZE_SMALL (22px)
const LH_MEDIUM = 38;  // line height of eos.FONT_SIZE_MEDIUM (26px)
const LH_LARGE  = 44;  // line height of eos.FONT_SIZE_LARGE (30px)

function makeStatic(obj) {
    obj.removeFlag(lv.OBJ_FLAG_SCROLLABLE);
    obj.removeFlag(lv.OBJ_FLAG_CLICKABLE);
    obj.removeFlag(lv.OBJ_FLAG_CLICK_FOCUSABLE);
}

// ── Root scrollable page ──────────────────────────────────────
const page = new lv.obj(view);
page.setSize(SW, SH);
page.setPos(0, 0);
page.setStyleBgColor(lv.color.hex(C.BG), lv.PART_MAIN);
page.setStyleBgGradColor(lv.color.hex(C.BG_GRAD), lv.PART_MAIN);
page.setStyleBgGradDir(lv.GRAD_DIR_VER, lv.PART_MAIN);
page.setStyleBorderWidth(0, lv.PART_MAIN);
page.setStylePadAll(0, lv.PART_MAIN);
page.setStylePadHor(12, lv.PART_MAIN);
page.setFlexFlow(lv.FLEX_FLOW_COLUMN);
page.setStylePadRow(10, lv.PART_MAIN);
page.setStylePadTop(12, lv.PART_MAIN);
page.setStylePadBottom(48, lv.PART_MAIN);
page.setScrollbarMode(lv.SCROLLBAR_MODE_OFF);

// ==========================================================================
//  Card helper — gradient bg, shadow, accent dot, title, separator
// ==========================================================================
function createCard(parent, titleText, accentColor, height) {
    const card = new lv.obj(parent);
    card.setSize(CARD_W, height);
    card.setStyleBgColor(lv.color.hex(C.CARD_BG), lv.PART_MAIN);
    card.setStyleBgGradColor(lv.color.hex(C.CARD_BG2), lv.PART_MAIN);
    card.setStyleBgGradDir(lv.GRAD_DIR_VER, lv.PART_MAIN);
    card.setStyleBorderWidth(1, lv.PART_MAIN);
    card.setStyleBorderColor(lv.color.hex(C.CARD_BORDER), lv.PART_MAIN);
    card.setStyleRadius(16, lv.PART_MAIN);
    card.setStylePadAll(0, lv.PART_MAIN);
    card.setStyleShadowWidth(18, lv.PART_MAIN);
    card.setStyleShadowColor(lv.color.hex(0x000000), lv.PART_MAIN);
    card.setStyleShadowOpa(60, lv.PART_MAIN);
    card.setStyleShadowSpread(0, lv.PART_MAIN);
    makeStatic(card);

    // Accent dot before title
    const dot = new lv.obj(card);
    dot.setSize(8, 8);
    dot.setPos(16, 15);
    dot.setStyleBgColor(lv.color.hex(accentColor), lv.PART_MAIN);
    dot.setStyleRadius(4, lv.PART_MAIN);
    dot.setStylePadAll(0, lv.PART_MAIN);
    dot.setStyleBorderWidth(0, lv.PART_MAIN);
    dot.setStyleShadowWidth(6, lv.PART_MAIN);
    dot.setStyleShadowColor(lv.color.hex(accentColor), lv.PART_MAIN);
    dot.setStyleShadowOpa(80, lv.PART_MAIN);
    makeStatic(dot);

    // Title text
    const title = new lv.label(card);
    title.setText(titleText);
    title.setPos(30, 3);
    title.setStyleTextColor(lv.color.hex(C.TEXT_PRIMARY), lv.PART_MAIN);
    title.setFontSize(eos.FONT_SIZE_SMALL);
    title.setStyleTextLetterSpace(1, lv.PART_MAIN);
    makeStatic(title);

    // Separator line under title
    const sep = new lv.obj(card);
    sep.setSize(CARD_W - 32, 1);
    sep.setPos(16, 38);
    sep.setStyleBgColor(lv.color.hex(C.CARD_BORDER), lv.PART_MAIN);
    sep.setStyleBgOpa(200, lv.PART_MAIN);
    sep.setStylePadAll(0, lv.PART_MAIN);
    sep.setStyleRadius(0, lv.PART_MAIN);
    makeStatic(sep);

    return card;
}

// Frequency badge helper
function createBadge(parent, text, color) {
    const badge_width = 80;
    const badge = new lv.obj(parent);
    badge.setSize(badge_width, 32);
    badge.setPos(CARD_W - badge_width - 16, 4);
    badge.setStyleBgColor(lv.color.hex(color), lv.PART_MAIN);
    badge.setStyleBgOpa(30, lv.PART_MAIN);
    badge.setStyleRadius(16, lv.PART_MAIN);
    badge.setStyleBorderWidth(1, lv.PART_MAIN);
    badge.setStyleBorderColor(lv.color.hex(color), lv.PART_MAIN);
    badge.setStyleBorderOpa(60, lv.PART_MAIN);
    badge.setStylePadAll(0, lv.PART_MAIN);
    makeStatic(badge);

    // The smallest font is 22px (eos.FONT_SIZE_SMALL); give the label the
    // full line height so the pill doesn't clip the text at the bottom.
    const lbl = new lv.label(badge);
    lbl.setText(text);
    lbl.setWidth(badge_width);
    lbl.align(lv.ALIGN_CENTER, 0, 0);
    lbl.setStyleTextAlign(lv.TEXT_ALIGN_CENTER, lv.PART_MAIN);
    lbl.setStyleTextColor(lv.color.hex(color), lv.PART_MAIN);
    lbl.setFontSize(eos.FONT_SIZE_SMALL);
    makeStatic(lbl);

    return badge;
}

// Inner panel helper — subtle bg for data areas
function createPanel(parent, x, y, w, h) {
    const panel = new lv.obj(parent);
    panel.setSize(w, h);
    panel.setPos(x, y);
    panel.setStyleBgColor(lv.color.hex(C.PANEL_BG), lv.PART_MAIN);
    panel.setStyleBgOpa(80, lv.PART_MAIN);
    panel.setStyleRadius(8, lv.PART_MAIN);
    panel.setStyleBorderWidth(0, lv.PART_MAIN);
    panel.setStylePadAll(0, lv.PART_MAIN);
    makeStatic(panel);
    return panel;
}

// ==========================================================================
//  1. IMU Attitude Indicator  (姿态仪)
// ==========================================================================
function createIMUCard(parent) {
    const card = createCard(parent, "IMU 姿态仪", C.ACCENT, 380);
    createBadge(card, "25Hz", C.ACCENT);

    const AI = 160;
    const AI_X = Math.floor((CARD_W - AI) / 2);
    const AI_Y = 46;

    // ── Outer bezel ring with glow ─────────────────────────────
    const bezel = new lv.obj(card);
    bezel.setSize(AI + 8, AI + 8);
    bezel.setPos(AI_X - 4, AI_Y - 4);
    bezel.setStyleBgOpa(0, lv.PART_MAIN);
    bezel.setStyleBorderWidth(2, lv.PART_MAIN);
    bezel.setStyleBorderColor(lv.color.hex(C.RING_DARK), lv.PART_MAIN);
    bezel.setStyleRadius(Math.floor((AI + 8) / 2), lv.PART_MAIN);
    bezel.setStylePadAll(0, lv.PART_MAIN);
    bezel.setStyleShadowWidth(12, lv.PART_MAIN);
    bezel.setStyleShadowColor(lv.color.hex(C.ACCENT), lv.PART_MAIN);
    bezel.setStyleShadowOpa(25, lv.PART_MAIN);
    makeStatic(bezel);

    // ── Porthole (circular dark area) ─────────────────────────
    const porthole = new lv.obj(card);
    porthole.setSize(AI, AI);
    porthole.setPos(AI_X, AI_Y);
    porthole.setStyleBgColor(lv.color.hex(0x05080C), lv.PART_MAIN);
    porthole.setStyleRadius(Math.floor(AI / 2), lv.PART_MAIN);
    porthole.setStylePadAll(0, lv.PART_MAIN);
    porthole.setStyleBorderWidth(1, lv.PART_MAIN);
    porthole.setStyleBorderColor(lv.color.hex(0x1A2230), lv.PART_MAIN);
    porthole.setStyleClipCorner(true, lv.PART_MAIN);
    makeStatic(porthole);

    // ── Horizon disc (rotates for roll) ───────────────────────
    const DISC = AI * 2;
    const horizon = new lv.obj(porthole);
    horizon.setSize(DISC, DISC);
    horizon.setPos(Math.floor((AI - DISC) / 2), Math.floor((AI - DISC) / 2));
    horizon.setStyleBgColor(lv.color.hex(C.SKY), lv.PART_MAIN);
    horizon.setStyleBgGradColor(lv.color.hex(C.SKY_GRAD), lv.PART_MAIN);
    horizon.setStyleBgGradDir(lv.GRAD_DIR_VER, lv.PART_MAIN);
    horizon.setStylePadAll(0, lv.PART_MAIN);
    horizon.setStyleRadius(0, lv.PART_MAIN);
    horizon.setStyleTransformPivotX(Math.floor(DISC / 2), lv.PART_MAIN);
    horizon.setStyleTransformPivotY(Math.floor(DISC / 2), lv.PART_MAIN);
    makeStatic(horizon);

    // Ground half with gradient
    const ground = new lv.obj(horizon);
    ground.setSize(DISC, Math.floor(DISC / 2));
    ground.setPos(0, Math.floor(DISC / 2));
    ground.setStyleBgColor(lv.color.hex(C.GROUND), lv.PART_MAIN);
    ground.setStyleBgGradColor(lv.color.hex(C.GROUND_GRAD), lv.PART_MAIN);
    ground.setStyleBgGradDir(lv.GRAD_DIR_VER, lv.PART_MAIN);
    ground.setStylePadAll(0, lv.PART_MAIN);
    ground.setStyleRadius(0, lv.PART_MAIN);
    makeStatic(ground);

    // Horizon line (white, semi-transparent)
    const hLine = new lv.obj(horizon);
    hLine.setSize(DISC, 2);
    hLine.setPos(0, Math.floor(DISC / 2) - 1);
    hLine.setStyleBgColor(lv.color.hex(C.WHITE), lv.PART_MAIN);
    hLine.setStyleBgOpa(220, lv.PART_MAIN);
    hLine.setStylePadAll(0, lv.PART_MAIN);
    makeStatic(hLine);

    // Pitch ladder marks
    for (let i = -4; i <= 4; i++) {
        if (i === 0) continue;
        const mark = new lv.obj(horizon);
        const w = (Math.abs(i) % 2 === 0) ? 50 : 30;
        const y = Math.floor(DISC / 2) + i * 12;
        mark.setSize(w, 1);
        mark.setPos(Math.floor(DISC / 2) - Math.floor(w / 2), y);
        mark.setStyleBgColor(lv.color.hex(0xB0B8C4), lv.PART_MAIN);
        mark.setStyleBgOpa(180, lv.PART_MAIN);
        mark.setStylePadAll(0, lv.PART_MAIN);
        makeStatic(mark);
    }

    // ── Fixed aircraft symbol (yellow) ────────────────────────
    const wingL = new lv.obj(card);
    wingL.setSize(24, 3);
    wingL.setPos(AI_X + Math.floor(AI / 2) - 40, AI_Y + Math.floor(AI / 2) - 1);
    wingL.setStyleBgColor(lv.color.hex(C.YELLOW), lv.PART_MAIN);
    wingL.setStyleRadius(1, lv.PART_MAIN);
    wingL.setStylePadAll(0, lv.PART_MAIN);
    makeStatic(wingL);

    const wingR = new lv.obj(card);
    wingR.setSize(24, 3);
    wingR.setPos(AI_X + Math.floor(AI / 2) + 16, AI_Y + Math.floor(AI / 2) - 1);
    wingR.setStyleBgColor(lv.color.hex(C.YELLOW), lv.PART_MAIN);
    wingR.setStyleRadius(1, lv.PART_MAIN);
    wingR.setStylePadAll(0, lv.PART_MAIN);
    makeStatic(wingR);

    const cDot = new lv.obj(card);
    cDot.setSize(6, 6);
    cDot.setPos(AI_X + Math.floor(AI / 2) - 3, AI_Y + Math.floor(AI / 2) - 3);
    cDot.setStyleBgColor(lv.color.hex(C.YELLOW), lv.PART_MAIN);
    cDot.setStyleRadius(3, lv.PART_MAIN);
    cDot.setStylePadAll(0, lv.PART_MAIN);
    makeStatic(cDot);

    // Top index marker
    const topIdx = new lv.obj(card);
    topIdx.setSize(2, 8);
    topIdx.setPos(AI_X + Math.floor(AI / 2) - 1, AI_Y + 4);
    topIdx.setStyleBgColor(lv.color.hex(C.WHITE), lv.PART_MAIN);
    topIdx.setStylePadAll(0, lv.PART_MAIN);
    makeStatic(topIdx);

    // ── Roll / Pitch / Yaw data boxes ────────────────────────
    const boxY = AI_Y + AI + 14;
    const boxW = Math.floor((CARD_W - 44) / 3);
    const boxH = 64;
    const boxGap = 6;

    function makeDataBox(x, label) {
        const box = createPanel(card, x, boxY, boxW, boxH);
        box.setStyleBorderColor(lv.color.hex(C.CARD_BORDER), lv.PART_MAIN);
        box.setStyleBorderWidth(1, lv.PART_MAIN);

        const lbl = new lv.label(box);
        lbl.setText(label);
        lbl.setSize(boxW, LH_SMALL);
        lbl.setPos(0, 0);
        lbl.setStyleTextAlign(lv.TEXT_ALIGN_CENTER, lv.PART_MAIN);
        lbl.setStyleTextColor(lv.color.hex(C.TEXT_SECONDARY), lv.PART_MAIN);
        lbl.setFontSize(eos.FONT_SIZE_SMALL);
        makeStatic(lbl);

        const val = new lv.label(box);
        val.setText("0.0°");
        val.setSize(boxW, LH_SMALL);
        val.setPos(0, LH_SMALL);
        val.setStyleTextAlign(lv.TEXT_ALIGN_CENTER, lv.PART_MAIN);
        val.setStyleTextColor(lv.color.hex(C.ACCENT), lv.PART_MAIN);
        val.setFontSize(eos.FONT_SIZE_SMALL);
        makeStatic(val);

        return val;
    }

    const startX = 16;
    const rollVal  = makeDataBox(startX, "Roll");
    const pitchVal = makeDataBox(startX + boxW + boxGap, "Pitch");
    const yawVal   = makeDataBox(startX + (boxW + boxGap) * 2, "Yaw");

    // ── ACC / GYR raw data panel ─────────────────────────────
    const rawDataY = boxY + boxH + 6;
    const rawPanel = createPanel(card, 16, rawDataY, CARD_W - 32, 72);

    const accLbl = new lv.label(rawPanel);
    accLbl.setText("ACC  0.00  0.00  1.00 g");
    accLbl.setPos(10, 4);
    accLbl.setStyleTextColor(lv.color.hex(C.TEXT_SECONDARY), lv.PART_MAIN);
    accLbl.setFontSize(eos.FONT_SIZE_SMALL);
    makeStatic(accLbl);

    const gyrLbl = new lv.label(rawPanel);
    gyrLbl.setText("GYR  0.00  0.00  0.00 °/s");
    gyrLbl.setPos(10, 36);
    gyrLbl.setStyleTextColor(lv.color.hex(C.TEXT_SECONDARY), lv.PART_MAIN);
    gyrLbl.setFontSize(eos.FONT_SIZE_SMALL);
    makeStatic(gyrLbl);

    return {
        horizon: horizon,
        discSize: DISC,
        aiSize: AI,
        aiY: AI_Y,
        rollVal: rollVal,
        pitchVal: pitchVal,
        yawVal: yawVal,
        accLbl: accLbl,
        gyrLbl: gyrLbl,
    };
}

// ==========================================================================
//  2. Magnetometer Compass  (指南针)
// ==========================================================================
function createCompassCard(parent) {
    const card = createCard(parent, "磁力计 指南针", C.ACCENT, 242);
    createBadge(card, "10Hz", C.ACCENT);

    const CS = 140;
    const CX = Math.floor((CARD_W - CS) / 2);
    const CY = 46;

    // Outer ring with subtle glow
    const ring = new lv.obj(card);
    ring.setSize(CS, CS);
    ring.setPos(CX, CY);
    ring.setStyleBgOpa(0, lv.PART_MAIN);
    ring.setStyleBorderWidth(2, lv.PART_MAIN);
    ring.setStyleBorderColor(lv.color.hex(C.RING_DARK), lv.PART_MAIN);
    ring.setStyleRadius(Math.floor(CS / 2), lv.PART_MAIN);
    ring.setStylePadAll(0, lv.PART_MAIN);
    ring.setStyleShadowWidth(10, lv.PART_MAIN);
    ring.setStyleShadowColor(lv.color.hex(C.ACCENT), lv.PART_MAIN);
    ring.setStyleShadowOpa(20, lv.PART_MAIN);
    makeStatic(ring);

    // Inner rotating dial
    const DS = CS - 24;
    const dial = new lv.obj(card);
    dial.setSize(DS, DS);
    dial.setPos(CX + 12, CY + 12);
    dial.setStyleBgOpa(0, lv.PART_MAIN);
    dial.setStyleRadius(Math.floor(DS / 2), lv.PART_MAIN);
    dial.setStylePadAll(0, lv.PART_MAIN);
    dial.setStyleTransformPivotX(Math.floor(DS / 2), lv.PART_MAIN);
    dial.setStyleTransformPivotY(Math.floor(DS / 2), lv.PART_MAIN);
    makeStatic(dial);

    const dCx = Math.floor(DS / 2);
    const dCy = Math.floor(DS / 2);
    const tickOuter = Math.floor(DS / 2) - 2;

    // Cardinal labels (N/E/S/W)
    const dirs = [
        { text: "N", angle: 0,   color: C.RED },
        { text: "E", angle: 90,  color: C.TEXT_SECONDARY },
        { text: "S", angle: 180, color: C.TEXT_SECONDARY },
        { text: "W", angle: 270, color: C.TEXT_SECONDARY },
    ];
    for (let i = 0; i < 4; i++) {
        const d = dirs[i];
        const rad = d.angle * Math.PI / 180;
        const lx = Math.floor(Math.sin(rad) * (tickOuter - 14));
        const ly = Math.floor(-Math.cos(rad) * (tickOuter - 14));

        const lbl = new lv.label(dial);
        lbl.setText(d.text);
        lbl.align(lv.ALIGN_CENTER, lx, ly);
        lbl.setStyleTextAlign(lv.TEXT_ALIGN_CENTER, lv.PART_MAIN);
        lbl.setStyleTextColor(lv.color.hex(d.color), lv.PART_MAIN);
        lbl.setFontSize(eos.FONT_SIZE_SMALL);
        makeStatic(lbl);
    }

    // Degree tick marks (every 30°)
    for (let i = 0; i < 12; i++) {
        const angle = i * 30;
        const rad = angle * Math.PI / 180;
        const isMajor = (i % 3 === 0);
        const tickLen = isMajor ? 8 : 4;
        const x1 = dCx + Math.floor(Math.sin(rad) * tickOuter);
        const y1 = dCy + Math.floor(-Math.cos(rad) * tickOuter);

        const tick = new lv.obj(dial);
        tick.setSize(1, tickLen);
        tick.setPos(x1, Math.min(y1, y1 - tickLen + 1));
        tick.setStyleBgColor(lv.color.hex(isMajor ? 0x7A8496 : 0x3A4454), lv.PART_MAIN);
        tick.setStylePadAll(0, lv.PART_MAIN);
        tick.setStyleTransformPivotX(0, lv.PART_MAIN);
        tick.setStyleTransformPivotY(Math.floor(tickLen / 2), lv.PART_MAIN);
        tick.setStyleTransformRotation(angle * 10, lv.PART_MAIN);
        makeStatic(tick);
    }

    // Fixed needle (red, pointing up = heading direction)
    const needle = new lv.obj(card);
    needle.setSize(3, Math.floor(CS / 2) - 24);
    needle.setPos(CX + Math.floor(CS / 2) - 1, CY + 22);
    needle.setStyleBgColor(lv.color.hex(C.RED), lv.PART_MAIN);
    needle.setStylePadAll(0, lv.PART_MAIN);
    needle.setStyleRadius(2, lv.PART_MAIN);
    needle.setStyleShadowWidth(8, lv.PART_MAIN);
    needle.setStyleShadowColor(lv.color.hex(C.RED), lv.PART_MAIN);
    needle.setStyleShadowOpa(40, lv.PART_MAIN);
    needle.setStyleTransformPivotX(1, lv.PART_MAIN);
    needle.setStyleTransformPivotY(Math.floor(CS / 2) - 24, lv.PART_MAIN);
    makeStatic(needle);

    // Center cap
    const cap = new lv.obj(card);
    cap.setSize(10, 10);
    cap.setPos(CX + Math.floor(CS / 2) - 5, CY + Math.floor(CS / 2) - 5);
    cap.setStyleBgColor(lv.color.hex(C.TEXT_PRIMARY), lv.PART_MAIN);
    cap.setStyleRadius(5, lv.PART_MAIN);
    cap.setStylePadAll(0, lv.PART_MAIN);
    cap.setStyleBorderWidth(1, lv.PART_MAIN);
    cap.setStyleBorderColor(lv.color.hex(C.CARD_BORDER), lv.PART_MAIN);
    makeStatic(cap);

    // Heading text
    const headingLbl = new lv.label(card);
    headingLbl.setText("0°  N");
    headingLbl.setPos(0, CY + CS + 6);
    headingLbl.setSize(CARD_W, LH_MEDIUM);
    headingLbl.setStyleTextAlign(lv.TEXT_ALIGN_CENTER, lv.PART_MAIN);
    headingLbl.setStyleTextColor(lv.color.hex(C.ACCENT), lv.PART_MAIN);
    headingLbl.setFontSize(eos.FONT_SIZE_MEDIUM);
    makeStatic(headingLbl);

    return {
        dial: dial,
        headingLbl: headingLbl,
    };
}

// ==========================================================================
//  3. Heart Rate Monitor  (心率监测)
// ==========================================================================
function createHeartRateCard(parent) {
    const card = createCard(parent, "心率监测", C.ACCENT2, 178);
    createBadge(card, "1Hz", C.ACCENT2);

    // Big BPM number
    const bpmLbl = new lv.label(card);
    bpmLbl.setText("78");
    bpmLbl.setPos(16, 48);
    bpmLbl.setSize(120, 48);
    bpmLbl.setStyleTextColor(lv.color.hex(C.ACCENT2), lv.PART_MAIN);
    bpmLbl.setFontSize(eos.FONT_SIZE_LARGE);
    makeStatic(bpmLbl);

    // BPM unit
    const bpmUnit = new lv.label(card);
    bpmUnit.setText("BPM");
    bpmUnit.setPos(130, 72);
    bpmUnit.setStyleTextColor(lv.color.hex(C.TEXT_SECONDARY), lv.PART_MAIN);
    bpmUnit.setFontSize(eos.FONT_SIZE_SMALL);
    makeStatic(bpmUnit);

    // Heart pulse icon (animated via scale transform)
    const heart = new lv.obj(card);
    heart.setSize(24, 24);
    heart.setPos(CARD_W - 48, 46);
    heart.setStyleBgColor(lv.color.hex(C.ACCENT2), lv.PART_MAIN);
    heart.setStyleRadius(12, lv.PART_MAIN);
    heart.setStylePadAll(0, lv.PART_MAIN);
    heart.setStyleTransformPivotX(12, lv.PART_MAIN);
    heart.setStyleTransformPivotY(12, lv.PART_MAIN);
    heart.setStyleShadowWidth(12, lv.PART_MAIN);
    heart.setStyleShadowColor(lv.color.hex(C.ACCENT2), lv.PART_MAIN);
    heart.setStyleShadowOpa(50, lv.PART_MAIN);
    makeStatic(heart);

    // Waveform chart
    const chartW = CARD_W - 32;
    const chartH = 60;
    const chart = new lv.chart(card);
    chart.setSize(chartW, chartH);
    chart.setPos(16, 106);
    chart.setType(lv.CHART_TYPE_LINE);
    chart.setRange(lv.CHART_AXIS_PRIMARY_Y, 0, 100);
    chart.setPointCount(50);
    chart.setStyleBgColor(lv.color.hex(C.PANEL_BG), lv.PART_MAIN);
    chart.setStyleBgOpa(60, lv.PART_MAIN);
    chart.setStyleBorderWidth(0, lv.PART_MAIN);
    chart.setStyleRadius(8, lv.PART_MAIN);
    chart.setStylePadAll(4, lv.PART_MAIN);
    makeStatic(chart);

    const series = chart.addSeries(lv.color.hex(C.ACCENT2), lv.CHART_AXIS_PRIMARY_Y);

    return {
        bpmLbl: bpmLbl,
        chart: chart,
        series: series,
        heart: heart,
    };
}

// ==========================================================================
//  4. Environment Sensors  (环境传感器)
// ==========================================================================
function createEnvCard(parent) {
    const card = createCard(parent, "环境传感器", C.GREEN, 262);

    const grid = new lv.obj(card);
    grid.setSize(CARD_W - 32, 200);
    grid.setPos(16, 46);
    grid.setStyleBgOpa(0, lv.PART_MAIN);
    grid.setStylePadAll(0, lv.PART_MAIN);
    grid.setFlexFlow(lv.FLEX_FLOW_ROW_WRAP);
    grid.setFlexAlign(lv.FLEX_ALIGN_SPACE_EVENLY, lv.FLEX_ALIGN_START, lv.FLEX_ALIGN_START);
    grid.setStylePadRow(8, lv.PART_MAIN);
    makeStatic(grid);

    const cellW = Math.floor((CARD_W - 48) / 2);

    function createGauge(label, unit, minV, maxV, color) {
        const cell = new lv.obj(grid);
        cell.setSize(cellW, 96);
        cell.setStyleBgColor(lv.color.hex(C.PANEL_BG), lv.PART_MAIN);
        cell.setStyleBgOpa(60, lv.PART_MAIN);
        cell.setStyleRadius(10, lv.PART_MAIN);
        cell.setStyleBorderWidth(1, lv.PART_MAIN);
        cell.setStyleBorderColor(lv.color.hex(C.CARD_BORDER), lv.PART_MAIN);
        cell.setStylePadAll(0, lv.PART_MAIN);
        makeStatic(cell);

        const GS = 52;
        const arcX = Math.floor((cellW - GS) / 2);
        const arcY = 6;

        // Single arc: PART_MAIN = background track, PART_INDICATOR = progress
        const arc = new lv.arc(cell);
        arc.setSize(GS, GS);
        arc.setPos(arcX, arcY);
        arc.startAngle = 135;
        arc.endAngle = 45;
        arc.value = 0;

        // Background track (full circle segment)
        arc.setStyleArcColor(lv.color.hex(0x1A222C), lv.PART_MAIN);
        arc.setStyleArcWidth(4, lv.PART_MAIN);
        arc.setStyleArcRounded(true, lv.PART_MAIN);

        // Indicator (progress arc)
        arc.setStyleArcColor(lv.color.hex(color), lv.PART_INDICATOR);
        arc.setStyleArcWidth(4, lv.PART_INDICATOR);
        arc.setStyleArcRounded(true, lv.PART_INDICATOR);

        // Hide knob
        arc.setStyleBgOpa(0, lv.PART_KNOB);
        arc.setStylePadAll(0, lv.PART_KNOB);

        // Clean up main background
        arc.setStyleBorderWidth(0, lv.PART_MAIN);
        arc.setStyleBgOpa(0, lv.PART_MAIN);
        arc.setStylePadAll(0, lv.PART_MAIN);
        makeStatic(arc);

        // Center value label (positioned in arc center)
        const valLbl = new lv.label(cell);
        valLbl.setText("0");
        valLbl.setSize(cellW, LH_SMALL);
        valLbl.setPos(0, arcY + Math.floor((GS - LH_SMALL) / 2));
        valLbl.setStyleTextAlign(lv.TEXT_ALIGN_CENTER, lv.PART_MAIN);
        valLbl.setStyleTextColor(lv.color.hex(C.TEXT_PRIMARY), lv.PART_MAIN);
        valLbl.setFontSize(eos.FONT_SIZE_SMALL);
        makeStatic(valLbl);

        // Bottom label
        const lbl = new lv.label(cell);
        lbl.setText(label + " " + unit);
        lbl.setSize(cellW, LH_SMALL);
        lbl.setPos(0, arcY + GS + 4);
        lbl.setStyleTextAlign(lv.TEXT_ALIGN_CENTER, lv.PART_MAIN);
        lbl.setStyleTextColor(lv.color.hex(C.TEXT_SECONDARY), lv.PART_MAIN);
        lbl.setFontSize(eos.FONT_SIZE_SMALL);
        makeStatic(lbl);

        return {
            arc: arc,
            valLbl: valLbl,
            minV: minV,
            maxV: maxV,
            setValue: function (v) {
                if (v < minV) v = minV;
                if (v > maxV) v = maxV;
                const normalized = Math.floor((v - minV) / (maxV - minV) * 100);
                arc.value = normalized;
                valLbl.setText((Math.round(v * 10) / 10) + "");
            },
        };
    }

    return {
        temp:  createGauge("温度",  "°C",   0,     50,     C.ORANGE),
        baro:  createGauge("气压",  "hPa",  950,   1050,   C.ACCENT),
        light: createGauge("光照",  "lux",  0,     10000,  C.YELLOW),
        step:  createGauge("步数",  "步",   0,     10000,  C.GREEN),
    };
}

// ==========================================================================
//  Build UI
// ==========================================================================
const imu     = createIMUCard(page);
const compass = createCompassCard(page);
const hr      = createHeartRateCard(page);
const env     = createEnvCard(page);

// ==========================================================================
//  Sensor configuration
// ==========================================================================
// Enable all sensors we need. Note: we re-apply the sample period in the
// update loop (every 25 ticks ≈ 1s) to keep sensors enabled even if the
// watchface widgets unsubscribe when the watchface is hidden.
eos.sensor.setSamplePeriod(eos.SENSOR_ACCE,   40);
eos.sensor.setSamplePeriod(eos.SENSOR_GYRO,   40);
eos.sensor.setSamplePeriod(eos.SENSOR_MAG,   100);
eos.sensor.setSamplePeriod(eos.SENSOR_HR,   1000);
eos.sensor.setSamplePeriod(eos.SENSOR_TEMP, 2000);
eos.sensor.setSamplePeriod(eos.SENSOR_BARO, 2000);
eos.sensor.setSamplePeriod(eos.SENSOR_LIGHT,1000);
eos.sensor.setSamplePeriod(eos.SENSOR_STEP, 1000);

// ==========================================================================
//  Sensor update loop
// ==========================================================================
let roll  = 0.0;
let pitch = 0.0;
let yaw   = 0.0;
let tick  = 0;

function fmt(v, d) {
    const m = Math.pow(10, d);
    return (Math.round(v * m) / m).toFixed(d);
}

function padL(s, w) {
    s = s + "";
    while (s.length < w) s = " " + s;
    return s;
}

function updateSensors() {
    tick++;

    // Re-apply sample period every ~1s to keep sensors enabled
    // (watchface unsubscribe may disable them when watchface is hidden)
    if (tick % 25 === 0) {
        eos.sensor.setSamplePeriod(eos.SENSOR_ACCE,   40);
        eos.sensor.setSamplePeriod(eos.SENSOR_GYRO,   40);
        eos.sensor.setSamplePeriod(eos.SENSOR_MAG,   100);
        eos.sensor.setSamplePeriod(eos.SENSOR_HR,   1000);
        eos.sensor.setSamplePeriod(eos.SENSOR_TEMP, 2000);
        eos.sensor.setSamplePeriod(eos.SENSOR_BARO, 2000);
        eos.sensor.setSamplePeriod(eos.SENSOR_LIGHT,1000);
        eos.sensor.setSamplePeriod(eos.SENSOR_STEP, 1000);
    }

    // ── IMU ───────────────────────────────────────────────────
    const acce = eos.sensor.readLatest(eos.SENSOR_ACCE);
    const gyro = eos.sensor.readLatest(eos.SENSOR_GYRO);

    if (acce && gyro) {
        const ax = acce.x / 1000.0;  // mg → g
        const ay = acce.y / 1000.0;
        const az = acce.z / 1000.0;

        // Tilt angles from accelerometer
        const accRoll  = Math.atan2(ay, Math.sqrt(ax * ax + az * az)) * 180 / Math.PI;
        const accPitch = Math.atan2(-ax, Math.sqrt(ay * ay + az * az)) * 180 / Math.PI;

        // Complementary filter (smooth)
        roll  = roll  * 0.92 + accRoll  * 0.08;
        pitch = pitch * 0.92 + accPitch * 0.08;

        // Yaw from gyro z integration
        yaw += gyro.z * 0.04 / 1000.0; // dps * dt(0.04s) = degrees
        if (yaw >= 360) yaw -= 360;
        if (yaw < 0)    yaw += 360;

        // ── Rotate horizon disc for roll (unit: 0.1°) ──
        imu.horizon.setStyleTransformRotation(Math.floor(roll * 10), lv.PART_MAIN);

        // ── Pitch: vertical offset of horizon disc ──
        const pitchPx = Math.floor(pitch * (imu.aiSize / 90));
        const baseX = Math.floor((imu.aiSize - imu.discSize) / 2);
        const baseY = Math.floor((imu.aiSize - imu.discSize) / 2);
        imu.horizon.setPos(baseX, baseY + pitchPx);

        imu.rollVal.setText(fmt(roll, 1) + "°");
        imu.pitchVal.setText(fmt(pitch, 1) + "°");
        imu.yawVal.setText(fmt(yaw, 1) + "°");

        imu.accLbl.setText(
            "ACC  " + padL(fmt(ax, 2), 5) +
            "  "   + padL(fmt(ay, 2), 5) +
            "  "   + padL(fmt(az, 2), 5) + " g"
        );
        imu.gyrLbl.setText(
            "GYR  " + padL(fmt(gyro.x / 1000.0, 2), 5) +
            "  "   + padL(fmt(gyro.y / 1000.0, 2), 5) +
            "  "   + padL(fmt(gyro.z / 1000.0, 2), 5) + " °/s"
        );
    }

    // ── Compass ───────────────────────────────────────────────
    const mag = eos.sensor.readLatest(eos.SENSOR_MAG);
    if (mag) {
        const heading = Math.atan2(mag.y, mag.x) * 180 / Math.PI;
        let hd = heading;
        if (hd < 0) hd += 360;

        // Rotate dial so N aligns with magnetic heading
        compass.dial.setStyleTransformRotation(Math.floor(hd * 10), lv.PART_MAIN);

        let dir = "N";
        if      (hd >= 22.5  && hd < 67.5)  dir = "NE";
        else if (hd >= 67.5  && hd < 112.5) dir = "E";
        else if (hd >= 112.5 && hd < 157.5) dir = "SE";
        else if (hd >= 157.5 && hd < 202.5) dir = "S";
        else if (hd >= 202.5 && hd < 247.5) dir = "SW";
        else if (hd >= 247.5 && hd < 292.5) dir = "W";
        else if (hd >= 292.5 && hd < 337.5) dir = "NW";

        compass.headingLbl.setText(Math.floor(hd) + "°  " + dir);
    }

    // ── Heart Rate ────────────────────────────────────────────
    const hrData = eos.sensor.readLatest(eos.SENSOR_HR);
    if (hrData) {
        const bpm = hrData.heart_rate;
        hr.bpmLbl.setText(bpm + "");

        // Simulate ECG-like waveform
        const beatMs = 60000 / bpm;
        const phase = (tick * 40) % beatMs / beatMs;

        let w = 25; // baseline

        if (phase < 0.08) {
            w = 25 + 10 * Math.sin(phase * Math.PI / 0.08);          // P wave
        } else if (phase < 0.13) {
            w = 25 - 8 * Math.sin((phase - 0.08) * Math.PI / 0.05); // Q wave
        } else if (phase < 0.18) {
            w = 25 + 65 * Math.sin((phase - 0.13) * Math.PI / 0.05); // R peak
        } else if (phase < 0.23) {
            w = 25 - 15 * Math.sin((phase - 0.18) * Math.PI / 0.05); // S wave
        } else if (phase < 0.40) {
            w = 25 + 20 * Math.sin((phase - 0.23) * Math.PI / 0.17); // T wave
        } else {
            w = 25 + Math.random() * 3 - 1.5;                          // baseline noise
        }

        hr.chart.setNextValue(hr.series, Math.floor(w));

        // Heart pulse animation (LVGL scale = 256 → 1.0x)
        if (phase < 0.12) {
            const s = 1.0 + 0.25 * Math.sin(phase * Math.PI / 0.12);
            hr.heart.setStyleTransformScale(Math.floor(s * 256), lv.PART_MAIN);
        } else {
            hr.heart.setStyleTransformScale(256, lv.PART_MAIN);
        }
    }

    // ── Environment ───────────────────────────────────────────
    const temp = eos.sensor.readLatest(eos.SENSOR_TEMP);
    if (temp) env.temp.setValue(temp.temp / 100.0);

    const baro = eos.sensor.readLatest(eos.SENSOR_BARO);
    if (baro) env.baro.setValue(baro.pressure / 100.0); // Pa → hPa

    const light = eos.sensor.readLatest(eos.SENSOR_LIGHT);
    if (light) env.light.setValue(light.lux);

    const step = eos.sensor.readLatest(eos.SENSOR_STEP);
    if (step) env.step.setValue(step.steps % 10000);
}

// Start update timer: 40 ms = 25 Hz
const updateTimer = new lv.timer(updateSensors, 40, null);
// lv.timer is one-shot by default (auto_delete=true); disable auto-delete so
// the sensor update loop runs continuously.
updateTimer.setAutoDelete(false);

eos.console.log("Sensor Visualization App started");
