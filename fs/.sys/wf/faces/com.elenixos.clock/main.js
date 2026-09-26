const view = eos.view.active();
const SW = eos.DISPLAY_WIDTH;
const SH = eos.DISPLAY_HEIGHT;

const root = new lv.obj(view);
root.setSize(SW, SH);
root.setStyleBgColor(lv.color.hex(0x080C12), lv.PART_MAIN);
root.setStylePadAll(0, lv.PART_MAIN);
root.removeFlag(lv.OBJ_FLAG_SCROLLABLE);

const dial = eos.ww.analogDial(root, {
    width: Math.min(SW, SH) - 32,
    height: Math.min(SW, SH) - 32,
    x: 16,
    y: 0,
    smoothSecond: true,
    ticks: { majorCount: 12, minorCount: 60, majorLength: 12, minorLength: 5 },
    numerals: { enabled: true },
    hourHand: { enabled: true, length: 62, width: 6 },
    minuteHand: { enabled: true, length: 88, width: 4 },
    secondHand: { enabled: true, length: 96, width: 2 },
    centerCap: { enabled: true, radius: 5 }
});
dial.align(lv.ALIGN_CENTER, 0, -12);

const status = new lv.label(root);
status.align(lv.ALIGN_BOTTOM_MID, 0, -18);
status.setStyleTextColor(lv.color.hex(0xAEB9C8), lv.PART_MAIN);
status.setText("--%  -- bpm");

eos.metric.subscribe("battery.percent", data => {
    const battery = data.valid ? `${Math.round(data.value)}%` : "--%";
    status.setText(`${battery}  -- bpm`);
});
eos.metric.subscribe("health.heart_rate", data => {
    const heartRate = data.valid ? `${Math.round(data.value)} bpm` : "-- bpm";
    status.setText(`--%  ${heartRate}`);
});
