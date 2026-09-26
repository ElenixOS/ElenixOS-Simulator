/*
 * ElenixOS Widgets Gallery
 *
 * The gallery deliberately uses LVGL flex layouts for every relationship:
 * the page is a vertical flex list, cards are column flex containers, and
 * each widget stage centers its real SNI widget without fixed x/y placement.
 */

const view = eos.view.active();
const SW = eos.DISPLAY_WIDTH;
const SH = eos.DISPLAY_HEIGHT;

const C = {
    background: 0x080C12,
    surface: 0x111722,
    stage: 0x0B1018,
    border: 0x273142,
    text: 0xF2F5F8,
    secondary: 0x9DA9B8,
    divider: 0x202A38,
    blue: 0x5AC8FA,
    green: 0x4CD964,
    orange: 0xFF9F0A,
    red: 0xFF453A,
    purple: 0xBF5AF2,
    teal: 0x64D2FF,
};

const OUTER_PAD = 14;
const CONTENT_W = SW - OUTER_PAD * 2;
const CARD_PAD = 12;
const CARD_BORDER = 1;
const CARD_GAP = 12;
const SECTION_GAP = 8;
const STAGE_W = CONTENT_W - CARD_PAD * 2 - CARD_BORDER * 2;

function makeStatic(object) {
    object.removeFlag(lv.OBJ_FLAG_SCROLLABLE);
    object.removeFlag(lv.OBJ_FLAG_CLICKABLE);
}

function makeLabel(parent, text, height, color, fontSize) {
    const label = new lv.label(parent);
    label.setWidth(STAGE_W);
    label.setHeight(lv.SIZE_CONTENT);
    label.setText(text);
    label.setStyleTextColor(lv.color.hex(color), lv.PART_MAIN);
    label.setFontSize(fontSize);
    label.setStylePadAll(0, lv.PART_MAIN);
    label.setStylePadVer(3, lv.PART_MAIN);
    label.setStyleTextLineSpace(0, lv.PART_MAIN);
    makeStatic(label);
    label.updateLayout();
    return label;
}

function makeSection(titleText, detailText, stageHeight, accentColor) {
    const card = new lv.obj(gallery);
    card.setWidth(CONTENT_W);
    card.setStyleBgColor(lv.color.hex(C.surface), lv.PART_MAIN);
    card.setStyleBorderWidth(CARD_BORDER, lv.PART_MAIN);
    card.setStyleBorderColor(lv.color.hex(C.border), lv.PART_MAIN);
    card.setStyleRadius(16, lv.PART_MAIN);
    card.setStylePadAll(CARD_PAD, lv.PART_MAIN);
    card.setStylePadRow(SECTION_GAP, lv.PART_MAIN);
    card.setLayout(lv.LAYOUT_FLEX);
    card.setFlexFlow(lv.FLEX_FLOW_COLUMN);
    card.setFlexAlign(lv.FLEX_ALIGN_START, lv.FLEX_ALIGN_START, lv.FLEX_ALIGN_START);
    makeStatic(card);

    const meta = new lv.obj(card);
    meta.setWidth(STAGE_W);
    meta.setStyleBgOpa(0, lv.PART_MAIN);
    meta.setStyleBorderWidth(0, lv.PART_MAIN);
    meta.setStylePadAll(0, lv.PART_MAIN);
    meta.setStylePadColumn(8, lv.PART_MAIN);
    meta.setLayout(lv.LAYOUT_FLEX);
    meta.setFlexFlow(lv.FLEX_FLOW_ROW);
    meta.setFlexAlign(lv.FLEX_ALIGN_START, lv.FLEX_ALIGN_CENTER, lv.FLEX_ALIGN_START);
    makeStatic(meta);

    const accent = new lv.obj(meta);
    accent.setSize(7, 7);
    accent.setStyleBgColor(lv.color.hex(accentColor), lv.PART_MAIN);
    accent.setStyleRadius(4, lv.PART_MAIN);
    accent.setStylePadAll(0, lv.PART_MAIN);
    makeStatic(accent);

    const copy = new lv.obj(meta);
    copy.setFlexGrow(1);
    copy.setStyleBgOpa(0, lv.PART_MAIN);
    copy.setStyleBorderWidth(0, lv.PART_MAIN);
    copy.setStylePadAll(0, lv.PART_MAIN);
    copy.setLayout(lv.LAYOUT_FLEX);
    copy.setFlexFlow(lv.FLEX_FLOW_COLUMN);
    copy.setFlexAlign(lv.FLEX_ALIGN_START, lv.FLEX_ALIGN_START, lv.FLEX_ALIGN_START);
    makeStatic(copy);

    copy.setStylePadRow(4, lv.PART_MAIN);

    const title = makeLabel(copy, titleText, 32, C.text, eos.FONT_SIZE_SMALL);
    title.setWidth(STAGE_W - 15);
    title.updateLayout();
    const detail = makeLabel(copy, detailText, 28, C.secondary, eos.FONT_SIZE_SMALL - 2);
    detail.setWidth(STAGE_W - 15);
    detail.updateLayout();

    copy.setHeight(lv.SIZE_CONTENT);
    meta.setHeight(lv.SIZE_CONTENT);
    copy.updateLayout();
    meta.updateLayout();
    const stage = new lv.obj(card);
    stage.setWidth(STAGE_W);
    stage.setHeight(stageHeight);
    stage.setStyleBgColor(lv.color.hex(C.stage), lv.PART_MAIN);
    stage.setStyleBorderWidth(1, lv.PART_MAIN);
    stage.setStyleBorderColor(lv.color.hex(C.divider), lv.PART_MAIN);
    stage.setStyleRadius(12, lv.PART_MAIN);
    stage.setStylePadAll(0, lv.PART_MAIN);
    stage.setLayout(lv.LAYOUT_FLEX);
    stage.setFlexFlow(lv.FLEX_FLOW_ROW);
    stage.setFlexAlign(lv.FLEX_ALIGN_CENTER, lv.FLEX_ALIGN_CENTER, lv.FLEX_ALIGN_CENTER);
    makeStatic(stage);

    /* Let LVGL include the content, padding, border, and flex gap. */
    card.setHeight(lv.SIZE_CONTENT);
    card.updateLayout();

    return stage;
}

const page = new lv.obj(view);
page.setSize(SW, SH);
page.setStyleBgColor(lv.color.hex(C.background), lv.PART_MAIN);
page.setStyleBorderWidth(0, lv.PART_MAIN);
page.setStylePadAll(0, lv.PART_MAIN);
page.setStylePadHor(OUTER_PAD, lv.PART_MAIN);
page.setStylePadTop(16, lv.PART_MAIN);
page.setLayout(lv.LAYOUT_FLEX);
page.setFlexFlow(lv.FLEX_FLOW_COLUMN);
page.setFlexAlign(lv.FLEX_ALIGN_START, lv.FLEX_ALIGN_START, lv.FLEX_ALIGN_START);
makeStatic(page);

const header = new lv.obj(page);
header.setWidth(CONTENT_W);
header.setHeight(76);
header.setStyleBgColor(lv.color.hex(C.background), lv.PART_MAIN);
header.setStyleBorderWidth(0, lv.PART_MAIN);
header.setStylePadAll(0, lv.PART_MAIN);
header.setLayout(lv.LAYOUT_FLEX);
header.setFlexFlow(lv.FLEX_FLOW_COLUMN);
header.setFlexAlign(lv.FLEX_ALIGN_START, lv.FLEX_ALIGN_START, lv.FLEX_ALIGN_START);
makeStatic(header);

const headerTitle = makeLabel(header, "Widgets", 40, C.text, eos.FONT_SIZE_MEDIUM);
headerTitle.setWidth(CONTENT_W);
const headerDetail = makeLabel(header, "6 widgets  ·  swipe", 26, C.secondary, eos.FONT_SIZE_SMALL - 2);
headerDetail.setWidth(CONTENT_W);
header.setHeight(lv.SIZE_CONTENT);
header.updateLayout();

const gallery = new lv.obj(page);
gallery.setWidth(CONTENT_W);
gallery.setHeight(0);
gallery.setFlexGrow(1);
gallery.setStyleBgColor(lv.color.hex(C.background), lv.PART_MAIN);
gallery.setStyleBgOpa(255, lv.PART_MAIN);
gallery.setStyleBorderWidth(0, lv.PART_MAIN);
gallery.setStylePadAll(0, lv.PART_MAIN);
gallery.setStylePadTop(8, lv.PART_MAIN);
gallery.setStylePadBottom(24, lv.PART_MAIN);
gallery.setStylePadRow(CARD_GAP, lv.PART_MAIN);
gallery.setLayout(lv.LAYOUT_FLEX);
gallery.setFlexFlow(lv.FLEX_FLOW_COLUMN);
gallery.setFlexAlign(lv.FLEX_ALIGN_START, lv.FLEX_ALIGN_START, lv.FLEX_ALIGN_START);
gallery.addFlag(lv.OBJ_FLAG_SCROLLABLE);
gallery.setScrollbarMode(lv.SCROLLBAR_MODE_OFF);

// Analog Dial variants -----------------------------------------------------
const ANALOG_SIZE = 158;
const IMAGE_HAND_SRC = "/.sys/wf/faces/com.elenixos.demowidgets/assets/analog_hand.png";

function addAnalogVariant(title, detail, accent, config, exercise) {
    const stage = makeSection(title, detail, 176, accent);
    const dial = eos.ww.analogDial(stage, {
        width: ANALOG_SIZE,
        height: ANALOG_SIZE,
        smoothSecond: true,
        ticks: config.ticks,
        numerals: config.numerals,
        hands: config.hands,
        centerCap: config.centerCap,
    });
    if (exercise) exercise(dial);
    return dial;
}

// 1. Analog Dial / Classic -----------------------------------------------
const analogDial = addAnalogVariant("Analog Dial", "Classic", C.blue, {
    ticks: {
        major: { count: 12, length: 12, width: 3, color: 0xffffff, opacity: 255 },
        minor: { count: 60, length: 5, width: 1, color: 0x394554, opacity: 255 },
    },
    numerals: { enabled: true },
    hands: {
        hour: { enabled: true, type: "bar", length: 62, width: 7, color: 0xe4e8f0, tailLength: 4 },
        minute: { enabled: true, type: "needle", length: 88, width: 5, color: 0xc0c8d4, tailLength: 5 },
        second: {
            enabled: true,
            type: "line",
            length: 68,
            width: 2,
            color: 0xff3b3b,
            tailLength: 4,
            angleOffset: 0,
            offset: { x: 0, y: 0 },
        },
    },
    centerCap: { enabled: true, radius: 5, color: 0xff3b3b },
}, dial => {
    dial.hourHand.setStyle({ type: "bar", length: 62, width: 7, tailLength: 4 });
    dial.minuteHand.setColor(0xc0c8d4);
    dial.minuteHand.setWidth(5);
    dial.secondHand.setTailLength(4);
    dial.secondHand.setOffset({ x: 0, y: 0 });
    dial.secondHand.setAngleOffset(0);
});

// 2. Sport ---------------------------------------------------------------
addAnalogVariant("Sport", "High contrast", C.orange, {
    ticks: {
        major: { count: 12, length: 11, width: 5, color: C.teal, opacity: 255 },
        minor: { count: 60, length: 3, width: 2, color: 0x244457, opacity: 255 },
    },
    numerals: { enabled: true },
    hands: {
        hour: { enabled: true, type: "arrow", length: 58, width: 8, color: 0xf2f5f8, tailLength: 4 },
        minute: { enabled: true, type: "needle", length: 70, width: 6, color: C.teal, tailLength: 5 },
        second: { enabled: true, type: "line", length: 68, width: 3, color: C.orange, tailLength: 7 },
    },
    centerCap: { enabled: true, radius: 6, color: C.orange },
}, dial => {
    dial.minuteHand.setStyle({ type: "needle", length: 70, width: 6, color: C.teal, tailLength: 5 });
    dial.secondHand.setColor(C.orange);
});

// 3. Minimal --------------------------------------------------------------
addAnalogVariant("Minimal", "No numerals", C.secondary, {
    ticks: {
        major: { count: 4, length: 8, width: 2, color: 0xc8d0da, opacity: 255 },
        minor: { count: 12, length: 3, width: 1, color: 0x202a38, opacity: 0 },
    },
    numerals: { enabled: false },
    hands: {
        hour: { enabled: true, type: "bar", length: 52, width: 8, color: 0xb7c0cc, tailLength: 2 },
        minute: { enabled: true, type: "bar", length: 66, width: 5, color: 0xe4e8f0, tailLength: 3 },
        second: { enabled: true, type: "line", length: 68, width: 1, color: 0x8893a1, tailLength: 0 },
    },
    centerCap: { enabled: true, radius: 4, color: 0x8893a1 },
}, dial => {
    dial.secondHand.setWidth(1);
});

// 4. Image Hands ----------------------------------------------------------
addAnalogVariant("Image Hands", "PNG pivot", C.teal, {
    ticks: {
        major: { count: 12, length: 9, width: 2, color: C.teal, opacity: 255 },
        minor: { count: 60, length: 3, width: 1, color: 0x244457, opacity: 255 },
    },
    numerals: { enabled: false },
    hands: {
        hour: {
            enabled: true,
            type: "image",
            src: IMAGE_HAND_SRC,
            pivot: { x: 0.5, y: 0.82 },
            angleOffset: 0,
        },
        minute: { enabled: true, type: "needle", length: 70, width: 4, color: 0xf2f5f8, tailLength: 4 },
        second: { enabled: true, type: "line", length: 68, width: 2, color: 0xff6b5f, tailLength: 4 },
    },
    centerCap: { enabled: true, radius: 5, color: C.teal },
}, dial => {
    dial.hourHand.setPivot({ x: 0.5, y: 0.82 });
    dial.hourHand.setAngleOffset(0);
    dial.secondHand.setColor(0xff6b5f);
});

// 2. Compass Dial ----------------------------------------------------------
const compassStage = makeSection("Compass", "Heading", 172, C.red);
eos.ww.compassDial(compassStage, {
    width: 150,
    height: 150,
    cardinalLabels: true,
    showHeading: true,
    showCalibrationState: true,
    smoothing: 0.15,
});

// 3. Radial Gauge ----------------------------------------------------------
const gaugeStage = makeSection("Radial Gauge", "Metric gauge", 158, C.orange);
const gauge = eos.ww.radialGauge(gaugeStage, {
    width: 138,
    height: 138,
    min: 40,
    max: 180,
    startAngle: 135,
    angleRange: 270,
    majorStep: 20,
    minorStep: 5,
    valueLabel: true,
    ranges: [
        { from: 40, to: 60, color: C.blue },
        { from: 60, to: 140, color: C.green },
        { from: 140, to: 180, color: C.red },
    ],
});
eos.ww.radialGaugeSetValue(gauge, 72, true);

// 4. Metric Rings ----------------------------------------------------------
const ringsStage = makeSection("Metric Rings", "Multiple metrics", 158, C.green);
const rings = eos.ww.metricRings(ringsStage, {
    size: 136,
    ringWidth: 6,
    gap: 5,
    animate: false,
    rings: [
        { min: 0, max: 100, color: C.green },
        { min: 0, max: 10000, color: C.blue },
        { min: 40, max: 180, color: C.orange },
    ],
});
eos.ww.metricRingsSetValue(rings, 0, 88, true);
eos.ww.metricRingsSetValue(rings, 1, 7000, true);
eos.ww.metricRingsSetValue(rings, 2, 120, true);

// 5. Trend Chart -----------------------------------------------------------
const trendStage = makeSection("Trend Chart", "History", 112, C.purple);
const trend = eos.ww.trendChart(trendStage, {
    width: STAGE_W - 18,
    height: 88,
    capacity: 24,
    min: 40,
    max: 180,
    autoScale: false,
});
eos.ww.trendChartSetData(trend, [64, 70, 78, 72, 86, 92, 82, 76, 98, 88, 104, 96, 110, 100, 116, 106, 122, 112, 126, 118, 132, 124, 138, 128]);

// 6. Calendar Grid ---------------------------------------------------------
const calendarStage = makeSection("Calendar Grid", "Month view", 194, C.teal);
eos.ww.calendarGrid(calendarStage, {
    width: STAGE_W - 10,
    height: 180,
    weekStartsOn: "monday",
    showHeader: true,
    showWeekdays: true,
    highlightToday: true,
});
