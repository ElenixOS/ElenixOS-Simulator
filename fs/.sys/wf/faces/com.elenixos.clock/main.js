const view = eos.view.active();

// ── Display & layout constants ────────────────────────────────
const SW = eos.DISPLAY_WIDTH;
const SH = eos.DISPLAY_HEIGHT;
const FACE_RADIUS = Math.floor(Math.min(SW, SH) * 0.435);

// ── Color palette – clean minimalist ──────────────────────────
const COLOR_BG = lv.color.hex(0x080C12);
const COLOR_TICK_MAJOR = lv.color.hex(0xB8C0CC);
const COLOR_TICK_MINOR = lv.color.hex(0x2E3640);
const COLOR_DIGIT = lv.color.hex(0xD4DAE4);
const COLOR_RING = lv.color.hex(0x1A2030);
const COLOR_HOUR_HAND = lv.color.hex(0xE4E8F0);
const COLOR_MINUTE_HAND = lv.color.hex(0xC0C8D4);
const COLOR_SECOND_HAND = lv.color.hex(0xFF3B3B);
const COLOR_CAP_OUTER = lv.color.hex(0xD0D8E4);
const COLOR_CAP_INNER = lv.color.hex(0xFF4040);

// ── Helper: strip scrollable & clickable flags ────────────────
function makeStatic(obj) {
	obj.removeFlag(lv.OBJ_FLAG_SCROLLABLE);
	obj.removeFlag(lv.OBJ_FLAG_CLICKABLE);
	obj.removeFlag(lv.OBJ_FLAG_CLICK_FOCUSABLE);
}

// ── Root layer ─────────────────────────────────────────────────
const root = new lv.obj(view);
root.setSize(SW, SH);
root.setPos(0, 0);
root.setStyleBgColor(COLOR_BG, lv.PART_MAIN);
root.setStyleBorderWidth(0, lv.PART_MAIN);
root.setStyleRadius(0, lv.PART_MAIN);
root.setStylePadAll(0, lv.PART_MAIN);
root.setStyleShadowWidth(0, lv.PART_MAIN);
makeStatic(root);

// ── Face container – explicit position, not align() ────────────
const FACE_W = FACE_RADIUS * 2;
const FACE_H = FACE_RADIUS * 2;
const FACE_X = Math.floor((SW - FACE_W) / 2);
const FACE_Y = Math.floor((SH - FACE_H) / 2);

const face = new lv.obj(root);
face.setSize(FACE_W, FACE_H);
face.setPos(FACE_X, FACE_Y);
face.setStyleBgOpa(0, lv.PART_MAIN);
face.setStyleBorderWidth(3, lv.PART_MAIN);
face.setStyleBorderColor(COLOR_RING, lv.PART_MAIN);
face.setStyleRadius(FACE_RADIUS, lv.PART_MAIN);
face.setStylePadAll(0, lv.PART_MAIN);
face.setStyleShadowWidth(0, lv.PART_MAIN);
makeStatic(face);

// ── Tick marks ─────────────────────────────────────────────────
const TICK_MAJOR_W = 3;
const TICK_MAJOR_L = 16;
const TICK_MINOR_W = 1;
const TICK_MINOR_L = 8;
const TICK_MARGIN = 14;   // distance from face outer edge to tick outer end

/**
 * Create one radial tick mark.
 * Positions the tick at the 12‑o'clock reference slot, pivots it around
 * the face centre, then rotates it to the target angle.
 */
function createTick(parent, angleDeg, width, length, color) {
	const tick = new lv.obj(parent);
	tick.setSize(width, length);

	// Reference position: top of face container
	tick.setPos(FACE_RADIUS - Math.floor(width / 2), TICK_MARGIN);

	// Pivot at the centre of the face (relative to tick origin)
	tick.setStyleTransformPivotX(Math.floor(width / 2), lv.PART_MAIN);
	tick.setStyleTransformPivotY(FACE_RADIUS - TICK_MARGIN, lv.PART_MAIN);

	// LVGL uses tenths of degrees
	tick.setStyleTransformRotation(angleDeg * 10, lv.PART_MAIN);

	tick.setStyleBgColor(color, lv.PART_MAIN);
	tick.setStyleBgOpa(255, lv.PART_MAIN);
	tick.setStyleBorderWidth(0, lv.PART_MAIN);
	tick.setStyleRadius(Math.ceil(width / 2), lv.PART_MAIN);
	tick.setStylePadAll(0, lv.PART_MAIN);
	tick.setStyleShadowWidth(0, lv.PART_MAIN);
	makeStatic(tick);
	return tick;
}

function buildTickMarks(parent) {
	for (let i = 0; i < 60; i++) {
		const angleDeg = i * 6;       // 6° per minute
		if (i % 5 === 0) {
			createTick(parent, angleDeg, TICK_MAJOR_W, TICK_MAJOR_L, COLOR_TICK_MAJOR);
		} else {
			createTick(parent, angleDeg, TICK_MINOR_W, TICK_MINOR_L, COLOR_TICK_MINOR);
		}
	}
}

// ── Digit labels (12 · 3 · 6 · 9) ─────────────────────────────
const DIGIT_RADIUS = FACE_RADIUS - TICK_MARGIN - TICK_MAJOR_L - 22;

function addDigit(parent, value, angleDeg) {
	// Convert clock angle (0° = 12h, clockwise) → LVGL math angle (0° = 3h, CCW)
	const rad = (angleDeg - 90) * Math.PI / 180;
	const ox = Math.round(Math.cos(rad) * DIGIT_RADIUS);
	const oy = Math.round(Math.sin(rad) * DIGIT_RADIUS);

	const label = new lv.label(parent);
	label.setSize(50, 50);
	label.align(lv.ALIGN_CENTER, ox, oy + 10);
	label.setStyleTextAlign(lv.TEXT_ALIGN_CENTER, lv.PART_MAIN);
	label.setStyleTextColor(COLOR_DIGIT, lv.PART_MAIN);
	label.setStyleBgOpa(0, lv.PART_MAIN);
	label.setStyleBorderWidth(0, lv.PART_MAIN);
	label.setStylePadAll(0, lv.PART_MAIN);
	label.setStyleShadowWidth(0, lv.PART_MAIN);
	label.setFontSize(eos.FONT_SIZE_LARGE);
	label.setText(String(value));
	makeStatic(label);
	return label;
}

function buildDigits(parent) {
	addDigit(parent, 12, 0);
	addDigit(parent, 3, 90);
	addDigit(parent, 6, 180);
	addDigit(parent, 9, 270);
}

// ── Clock hands (styled lv.obj + eos.clockHand.attach) ───────
//
// Hands are plain lv.obj rectangles styled in JS (full LVGL prototype),
// then attached to the clock-hand timer via eos.clockHand.attach().
// This gives full control over color, size, and shape.
//
const HAND_TAIL_RATIO = 0.15;  // portion of hand behind the pivot centre

function createStyledHand(parent, handType, length, width, color) {
	const pivotY = Math.floor(length * (1.0 - HAND_TAIL_RATIO));
	const halfW = Math.floor(width / 2);

	const hand = new lv.obj(parent);
	hand.setSize(width, length);
	hand.setStyleBgColor(color, lv.PART_MAIN);
	hand.setStyleBgOpa(255, lv.PART_MAIN);
	hand.setStyleBorderWidth(0, lv.PART_MAIN);
	hand.setStyleRadius(Math.ceil(width / 2), lv.PART_MAIN);
	hand.setStylePadAll(0, lv.PART_MAIN);
	hand.setStyleShadowWidth(0, lv.PART_MAIN);

	// Transform pivot (rotation centre) – near the bottom of the hand
	hand.setStyleTransformPivotX(halfW, lv.PART_MAIN);
	hand.setStyleTransformPivotY(pivotY, lv.PART_MAIN);

	// Let eos.clockHand drive the rotation timer
	eos.clockHand.attach(hand, handType);

	// Position: pivot (halfW, pivotY) aligns with the face centre
	hand.setPos(FACE_RADIUS - halfW, FACE_RADIUS - pivotY);

	makeStatic(hand);
	return hand;
}

// ── Centre cap ─────────────────────────────────────────────────
function createCenterCap(parent) {
	const offset = 4;
	// Outer ring
	const outer = new lv.obj(parent);
	outer.setSize(14, 14);
	outer.align(lv.ALIGN_CENTER, offset, offset);
	outer.setStyleBgColor(COLOR_CAP_OUTER, lv.PART_MAIN);
	outer.setStyleBorderWidth(0, lv.PART_MAIN);
	outer.setStyleRadius(7, lv.PART_MAIN);
	outer.setStylePadAll(0, lv.PART_MAIN);
	outer.setStyleShadowWidth(0, lv.PART_MAIN);
	makeStatic(outer);

	// Inner dot
	const inner = new lv.obj(parent);
	inner.setSize(6, 6);
	inner.align(lv.ALIGN_CENTER, offset, offset);
	inner.setStyleBgColor(COLOR_CAP_INNER, lv.PART_MAIN);
	inner.setStyleBorderWidth(0, lv.PART_MAIN);
	inner.setStyleRadius(3, lv.PART_MAIN);
	inner.setStylePadAll(0, lv.PART_MAIN);
	inner.setStyleShadowWidth(0, lv.PART_MAIN);
	makeStatic(inner);
}

// ── Assemble ───────────────────────────────────────────────────
buildTickMarks(face);
buildDigits(face);

// Hand dimensions (fraction of face radius)
const HAND_LEN_HOUR = Math.floor(FACE_RADIUS * 0.56);
const HAND_LEN_MINUTE = Math.floor(FACE_RADIUS * 0.80);
const HAND_LEN_SECOND = Math.floor(FACE_RADIUS * 0.93);

// Hour hand   – widest, shortest, light silver
const hourHand = createStyledHand(face, eos.CLOCK_HAND_HOUR,
	HAND_LEN_HOUR, 10, COLOR_HOUR_HAND);
// Minute hand – medium width, longer
const minuteHand = createStyledHand(face, eos.CLOCK_HAND_MINUTE,
	HAND_LEN_MINUTE, 6, COLOR_MINUTE_HAND);
// Second hand – thinnest, longest, red
const secondHand = createStyledHand(face, eos.CLOCK_HAND_SECOND,
	HAND_LEN_SECOND, 2, COLOR_SECOND_HAND);

createCenterCap(face);
