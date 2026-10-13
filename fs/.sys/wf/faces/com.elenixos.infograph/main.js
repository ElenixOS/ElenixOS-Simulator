
/* ElenixOS Infograph*/


const view = eos.view.active();
const DISPLAY_WIDTH = eos.DISPLAY_WIDTH;
const DISPLAY_HEIGHT = eos.DISPLAY_HEIGHT;
const DESIGN_WIDTH = 390;
const DESIGN_HEIGHT = 450;
const FIT = Math.min(DISPLAY_WIDTH / DESIGN_WIDTH, DISPLAY_HEIGHT / DESIGN_HEIGHT);
const FACE_WIDTH = Math.round(DESIGN_WIDTH * FIT);
const FACE_HEIGHT = Math.round(DESIGN_HEIGHT * FIT);
const FACE_X = Math.round((DISPLAY_WIDTH - FACE_WIDTH) / 2);
const FACE_Y = Math.round((DISPLAY_HEIGHT - FACE_HEIGHT) / 2);
const USE_MOCK_DATA = true;

const MOCK = {
    temperature: { min: 20, value: 26, max: 29 },
    uvi: { min: 0, value: 3.6, max: 11 },
    batteryPercent: 97,
    ringProgress: [72, 86, 58],
};

const C = {
    black: 0x000000,
    white: 0xF7F7F5,
    orange: 0xFF9B38,
    green: 0x49D879,
    yellow: 0xF3D535,
    pink: 0xF22D63,
    lime: 0xB6EA32,
    cyan: 0x28C9E8,
    red: 0xE84955,
    dateDisc: 0x171719,
    ringTrack: 0x111515,
};

// Sizes must not turn zero into a 1px line or widget.
function px(value) {
    return value <= 0 ? 0 : Math.max(1, Math.round(value * FIT));
}

// Coordinates may be zero or negative, unlike visible sizes.
function coord(value) {
    return Math.round(value * FIT);
}

function faceX(value) {
    return FACE_X + coord(value);
}

function faceY(value) {
    return FACE_Y + coord(value);
}

function disableInteraction(object) {
    object.removeFlag(lv.OBJ_FLAG_SCROLLABLE);
    object.removeFlag(lv.OBJ_FLAG_CLICKABLE);
}

function addCircle(centerX, centerY, size, color) {
    const circle = new lv.obj(root);
    circle.setSize(px(size), px(size));
    circle.setPos(faceX(centerX - size / 2), faceY(centerY - size / 2));
    circle.setStyleBgColor(lv.color.hex(color), lv.PART_MAIN);
    circle.setStyleBgOpa(255, lv.PART_MAIN);
    circle.setStyleBorderWidth(0, lv.PART_MAIN);
    circle.setStyleRadius(lv.RADIUS_CIRCLE, lv.PART_MAIN);
    circle.setStylePadAll(0, lv.PART_MAIN);
    disableInteraction(circle);
    return circle;
}

function addLabel(spec) {
    const label = new lv.label(root);
    const width = px(spec.width);
    const height = px(spec.height);
    label.setSize(width, height);
    label.setPos(faceX(spec.x - spec.width / 2), faceY(spec.y - spec.height / 2));
    label.setText(spec.text);
    label.setStyleBgOpa(0, lv.PART_MAIN);
    label.setStyleBorderWidth(0, lv.PART_MAIN);
    label.setStylePadAll(0, lv.PART_MAIN);
    label.setStyleTextColor(lv.color.hex(spec.color), lv.PART_MAIN);
    label.setStyleTextAlign(lv.TEXT_ALIGN_CENTER, lv.PART_MAIN);
    label.setStyleTextLineSpace(0, lv.PART_MAIN);
    label.setFontSize(px(spec.fontSize));
    disableInteraction(label);
    return label;
}

function addIcon(source, x, y, width, height) {
    const icon = new lv.image(root);
    icon.setSrc(source);
    icon.setSize(px(width), px(height));
    icon.setPivot(0, 0);
    icon.setScale(Math.max(1, Math.round(FIT * 256)));
    icon.setPos(faceX(x), faceY(y));
    icon.setAntialias(true);
    icon.setStyleBgOpa(0, lv.PART_MAIN);
    icon.setStyleBorderWidth(0, lv.PART_MAIN);
    disableInteraction(icon);
    return icon;
}

function addSystemIcon(glyph, centerX, centerY, size, color) {
    const icon = addLabel({
        x: centerX,
        y: centerY,
        width: size,
        height: size,
        fontSize: 30,
        color,
        text: glyph,
    });
    return icon;
}

// All curved labels share geometry and style; only these options vary.
function addArcLabel(spec) {
    const label = new lv.arclabel(root);
    label.setSize(FACE_WIDTH, FACE_HEIGHT);
    label.setPos(FACE_X, FACE_Y);
    label.setCenterOffsetX(0);
    label.setCenterOffsetY(px(5));
    label.setRadius(px(spec.radius));
    label.setAngleStart(spec.angle);
    label.setAngleSize(360);
    label.setDir(spec.dir !== undefined ? spec.dir : lv.ARCLABEL_DIR_CLOCKWISE);
    label.setTextHorizontalAlign(lv.ARCLABEL_TEXT_ALIGN_CENTER);
    label.setTextVerticalAlign(lv.ARCLABEL_TEXT_ALIGN_DEFAULT);
    label.setOverflow(lv.ARCLABEL_OVERFLOW_CLIP);
    label.setFontSize(spec.fontSize);
    label.setText(spec.text);
    label.setStyleBgOpa(0, lv.PART_MAIN);
    label.setStyleBorderWidth(0, lv.PART_MAIN);
    label.setStylePadAll(0, lv.PART_MAIN);
    label.setStyleTextColor(lv.color.hex(spec.color), lv.PART_MAIN);
    label.setStyleTextLetterSpace(0, lv.PART_MAIN);
    label.setRecolor(true);
    disableInteraction(label);
    return label;
}

const root = new lv.obj(view);
root.setSize(DISPLAY_WIDTH, DISPLAY_HEIGHT);
root.setPos(0, 0);
root.setStyleBgColor(lv.color.hex(C.black), lv.PART_MAIN);
root.setStyleBgOpa(255, lv.PART_MAIN);
root.setStyleBorderWidth(0, lv.PART_MAIN);
root.setStylePadAll(0, lv.PART_MAIN);
disableInteraction(root);

// Tick marks belong behind every complication and curved label. AnalogDial
// paints them in its own draw phase, so this instance is deliberately created
// before the face content; its hands and cap are disabled.
eos.ww.analogDial(root, {
    width: px(340),
    height: px(340),
    x: faceX(25),
    y: faceY(60),
    smoothSecond: false,
    ticks: {
        major: { count: 12, length: px(13), width: px(2), color: C.white, opacity: 245 },
        minor: { count: 60, length: px(6), width: px(1), color: 0x85878A, opacity: 220 },
        // AnalogDial uses 0 degrees at 12 o'clock, clockwise. Leave the top
        // title's existing tick-free interval unchanged.
        hiddenRanges: [{ startAngle: 324, endAngle: 36 }],
    },
    numerals: { enabled: false },
    hands: {
        hour: { enabled: false },
        minute: { enabled: false },
        second: { enabled: false },
    },
    centerCap: { enabled: false },
});

// The face uses one 390x450 coordinate plane. Widget-local arc centers stay local;
// screen offsets are applied only by faceX/faceY when the LVGL object is positioned.
eos.ww.arcTrack(root, {
    width: FACE_WIDTH,
    height: FACE_HEIGHT,
    x: FACE_X,
    y: FACE_Y,
    centerX: px(195),
    centerY: px(230),
    radius: px(181),
    lineWidth: px(8),
    startAngle: 225,
    endAngle: 255,
    color: C.orange,
    rounded: true,
});

const temperatureGauge = eos.ww.arcTrack(root, {
    width: FACE_WIDTH,
    height: FACE_HEIGHT,
    x: FACE_X,
    y: FACE_Y,
    centerX: px(195),
    centerY: px(230),
    radius: px(181),
    lineWidth: px(7),
    startAngle: 286,
    endAngle: 314,
    colors: [C.green, C.yellow, C.orange, C.red],
    segmentGapAngle: 0,
    rounded: true,
    knobSize: px(13),
    knobBorderWidth: px(3),
    knobBorderColor: C.black,
});

const uviGauge = eos.ww.arcTrack(root, {
    width: FACE_WIDTH,
    height: FACE_HEIGHT,
    x: FACE_X,
    y: FACE_Y,
    centerX: px(195),
    centerY: px(230),
    radius: px(181),
    lineWidth: px(7),
    startAngle: 106,
    endAngle: 134,
    colors: [0x9227E9, 0xE746B7, C.red, C.orange, C.yellow, C.green],
    segmentGapAngle: 0,
    rounded: true,
    knobSize: px(13),
    knobBorderWidth: px(3),
    knobBorderColor: C.black,
});

// Use the supplied high-resolution clock artwork at the original glyph position.
addIcon("assets/icons/clock-orange.png", 56, 38, 44, 43);

// LVGL Arc renders the music ring as one continuous progress arc with native round ends.
const musicRing = eos.ww.metricRings(root, {
    size: px(82),
    ringWidth: px(6),
    gap: 0,
    startAngle: 270,
    angleRange: 270,
    trackColor: C.black,
    rounded: true,
    animate: false,
    x: faceX(70),
    y: faceY(189),
    rings: [{ min: 0, max: 100, color: C.pink }],
});
eos.ww.metricRingsSetValue(musicRing, 0, 100, true);

// Use LVGL's native ArcLabel directly for curved face text. The face-wide
// controls share one geometry; per-label radius and angle place each string,
// while center alignment uses LVGL's measured glyph advances and spacing.
const titleText = addArcLabel({
    radius: 145,
    angle: 90,
    fontSize: 16,
    color: C.white,
    text: "TRAINING WITH ELENIX",
});

const timeText = addArcLabel({
    radius: 172,
    angle: 30,
    fontSize: 14,
    color: C.orange,
    text: "10:08AM",
});

const uviText = addArcLabel({
    radius: 181,
    angle: 320,
    dir: lv.ARCLABEL_DIR_COUNTER_CLOCKWISE,
    fontSize: 12,
    color: C.yellow,
    text: "UVI",
});

const batteryText = addArcLabel({
    radius: 178,
    angle: 238,
    dir: lv.ARCLABEL_DIR_COUNTER_CLOCKWISE,
    fontSize: 15,
    color: C.green,
    text: "BAT 97%",
});

// The date disc and complication art are layered below the moving clock hands.
addCircle(195, 143, 88, C.dateDisc);

// LVGL's built-in SYMBOL_AUDIO is U+F001 (Font Awesome music symbol).
const posX = 109;
const posY = 230;
const musicGlyph = addSystemIcon(lv.SYMBOL_AUDIO, posX, posY, 48, C.white);

// LVGL labels top-align their text line. Center its 30px line in the 48px box.
musicGlyph.setPos(faceX(posX - 48 / 2), faceY(posY - 30 / 2));
addIcon("assets/icons/globe-realistic.png", 155, 270, 80, 80);

const activityRings = eos.ww.metricRings(root, {
    size: px(82),
    ringWidth: px(6),
    gap: px(4),
    startAngle: 270,
    angleRange: 360,
    trackColor: C.ringTrack,
    rounded: true,
    animate: true,
    x: faceX(241),
    y: faceY(189),
    rings: [
        { min: 0, max: 100, color: C.pink },
        { min: 0, max: 100, color: C.lime },
        { min: 0, max: 100, color: C.cyan },
    ],
});

const temperatureLowValue = addArcLabel({
    radius: 170,
    angle: 100,
    fontSize: px(19),
    color: C.green,
    text: "--",
});

const temperatureValue = addArcLabel({
    radius: 190,
    angle: 123,
    fontSize: px(36),
    color: C.white,
    text: "--°",
});

const temperatureHighValue = addArcLabel({
    radius: 170,
    angle: 143,
    fontSize: px(19),
    color: C.red,
    text: "--",
});

const weekdayValue = addLabel({ x: 195, y: 124, width: 70, height: 24, fontSize: 18, color: C.red, text: "---" });
const dateValue = addLabel({ x: 195, y: 158, width: 72, height: 44, fontSize: 39, color: C.white, text: "--" });

const uviValueCenter = { x: 72, y: 398 };
const uviValueDx = uviValueCenter.x - 195;
const uviValueDy = uviValueCenter.y - 230;
const uviValueRadius = Math.sqrt(uviValueDx * uviValueDx + uviValueDy * uviValueDy);
const uviValueCenterAngle = Math.atan2(uviValueDy, uviValueDx) * 180 / Math.PI;

// A centered 360° layout places the text midpoint opposite ArcLabel's start angle.
const uviValue = addArcLabel({
    radius: uviValueRadius,
    angle: uviValueCenterAngle + 175,
    dir: lv.ARCLABEL_DIR_COUNTER_CLOCKWISE,
    fontSize: px(27),
    color: C.white,
    text: "--",
});

// Every label that follows a dial path uses LVGL ArcLabel directly. Place the
// short lower-right brand text on the same tangent as the old -34° label.
const eosTextCenter = { x: 310, y: 414 };
const eosTextDx = eosTextCenter.x - 195;
const eosTextDy = eosTextCenter.y - 235;
const eosTextRadius = Math.sqrt(eosTextDx * eosTextDx + eosTextDy * eosTextDy);
const eosTextCenterAngle = Math.atan2(eosTextDy, eosTextDx) * 180 / Math.PI;

const eosText = addArcLabel({
    radius: eosTextRadius - 15,
    angle: eosTextCenterAngle + 180,
    dir: lv.ARCLABEL_DIR_COUNTER_CLOCKWISE,
    fontSize: px(16),
    color: C.white,
    text: "Elenix#ff0000 OS#",
});

// Hands and pivot render above the complications, while their own tick layer
// is empty so it cannot paint over the labels and progress rings.
eos.ww.analogDial(root, {
    width: px(360),
    height: px(360),
    x: faceX(15),
    y: faceY(50),
    smoothSecond: true,
    ticks: {
        major: { count: 0, length: 0, width: 0, color: C.white, opacity: 0 },
        minor: { count: 1, length: 0, width: 0, color: C.white, opacity: 0 },
    },
    numerals: { enabled: false },
    hands: {
        hour: {
            enabled: true,
            type: "image",
            src: "/.sys/wf/faces/com.elenixos.infograph/assets/hands/hour-hand.png",
            pivot: { x: 0.5, y: 0.5 },
            angleOffset: 52,
        },
        minute: {
            enabled: true,
            type: "image",
            src: "/.sys/wf/faces/com.elenixos.infograph/assets/hands/minute-hand.png",
            pivot: { x: 0.5, y: 0.5 },
            angleOffset: -54,
        },
        second: { enabled: true, type: "line", length: px(173), width: px(2), color: C.red, tailLength: 0 },
    },
    centerCap: { enabled: true, radius: px(5), color: C.red },
});

addCircle(195, 230, 4, C.white);

function sampleIsCurrent(sample) {
    return Boolean(sample && sample.valid && !sample.stale);
}

function isFiniteNumber(value) {
    return typeof value === "number" && value === value && value !== Infinity && value !== -Infinity;
}

function hasValidRange(sample) {
    return Boolean(sample && sampleIsCurrent(sample)
        && isFiniteNumber(sample.min) && isFiniteNumber(sample.value) && isFiniteNumber(sample.max)
        && sample.max > sample.min);
}

function setMetricRing(index, value, valid) {
    eos.ww.metricRingsSetValue(activityRings, index, valid ? value : 0, valid);
}

function updateTemperatureRange(sample) {
    const valid = hasValidRange(sample);
    if (!valid) {
        temperatureLowValue.setText("--");
        const currentValid = sampleIsCurrent(sample) && isFiniteNumber(sample.value);
        temperatureValue.setText(currentValid ? `${Math.round(sample.value)}°` : "--°");
        temperatureHighValue.setText("--");
        eos.ww.arcTrackSetRange(temperatureGauge, 0, 0, 1, false);
        return;
    }

    temperatureLowValue.setText(String(Math.round(sample.min)));
    temperatureValue.setText(`${Math.round(sample.value)}°`);
    temperatureHighValue.setText(String(Math.round(sample.max)));
    eos.ww.arcTrackSetRange(temperatureGauge, sample.min, sample.value, sample.max, true);
}

function updateUviRange(sample) {
    const valid = hasValidRange(sample);
    uviValue.setText(valid ? Number(sample.value).toFixed(1) : "--");
    eos.ww.arcTrackSetRange(uviGauge, valid ? sample.min : 0, valid ? sample.value : 0, valid ? sample.max : 1, valid);
}

function updateDateTime(now) {
    if (!now || now.year < 1 || now.month < 1 || now.day < 1 || now.hour > 23 || now.min > 59
        || now.day_of_week < 0 || now.day_of_week > 6) {
        timeText.setText("--:--");
        weekdayValue.setText("---");
        dateValue.setText("--");
        return;
    }

    const dayNames = ["SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"];
    const hour12 = now.hour % 12 || 12;
    const minute = now.min < 10 ? `0${now.min}` : String(now.min);
    const suffix = now.hour < 12 ? "AM" : "PM";
    timeText.setText(`${hour12}:${minute}${suffix}`);
    weekdayValue.setText(dayNames[now.day_of_week]);
    dateValue.setText(String(now.day));
}

function subscribeToLiveTime() {
    updateDateTime(eos.time.getNow());
    eos.metric.subscribe("time.now", data => {
        if (sampleIsCurrent(data)) updateDateTime(eos.time.getNow());
        else updateDateTime(null);
    }, 1000);
}

function displayMockData() {
    subscribeToLiveTime();
    updateTemperatureRange({ ...MOCK.temperature, valid: true, stale: false });
    updateUviRange({ ...MOCK.uvi, valid: true, stale: false });
    batteryText.setText(`BAT ${MOCK.batteryPercent}%`);
    for (let i = 0; i < MOCK.ringProgress.length; i++)
        setMetricRing(i, MOCK.ringProgress[i], true);
}

function subscribeToLiveData() {
    subscribeToLiveTime();

    eos.metric.subscribe("health.heart_rate", data => {
        const valid = sampleIsCurrent(data);
        setMetricRing(2, valid ? Math.max(0, Math.min(100, data.value)) : 0, valid);
    }, 5000);

    eos.metric.subscribe("environment.temperature", data => {
        // The live Metric exposes only instantaneous degC; it has no daily min/max range.
        updateTemperatureRange({ value: data.value, valid: data.valid, stale: data.stale });
    }, 10000);

    eos.metric.subscribe("battery.percent", data => {
        const valid = sampleIsCurrent(data);
        batteryText.setText(valid ? `BAT ${Math.round(data.value)}%` : "BAT --%");
        setMetricRing(1, valid ? data.value : 0, valid);
    }, 60000);

    uviValue.setText("--");
    eos.ww.arcTrackSetRange(uviGauge, 0, 0, 1, false);
}

if (USE_MOCK_DATA)
    displayMockData();
else
    subscribeToLiveData();
