/**
 * lv.obj complete method coverage test
 *
 * Covers: constructor, position/size, alignment, layout, flex, hierarchy,
 *         style (set/get), flags, state, events, user-data, scroll,
 *         visibility/coords, movement, class check, lifecycle.
 *
 * Log rules: no Chinese characters. Each entry is [PASS] or [FAIL].
 */
import { test, log, assertEqual, assertOk, assertType, assertNotNull, assertThrows, assertClose, runSuite } from './framework.mjs';

export function suite() {
    runSuite('obj', () => {
    let scr = eos.view.active();
    let eventFired = false;

    /* ---- 1. Constructor ------------------------------------------- */
    let parent;
    test("constructor new lv.obj(scr)", () => {
        parent = new lv.obj(scr);
        parent.setSize(400, 400);
        parent.align(lv.ALIGN_CENTER, 0, 0);
        if (!parent) throw new Error("null handle");
    });

    let obj;
    test("constructor child new lv.obj(parent)", () => {
        obj = new lv.obj(parent);
        obj.setSize(200, 200);
        if (!obj) throw new Error("null handle");
    });

    // A sibling to use for alignTo and other tests
    let sib;
    test("constructor sibling new lv.obj(parent)", () => {
        sib = new lv.obj(parent);
        sib.setSize(80, 80);
    });

    /* ---- 2. Position & Size --------------------------------------- */
    test("setPos", () => obj.setPos(10, 20));
    test("setX", () => obj.setX(5));
    test("setY", () => obj.setY(5));
    test("setSize", () => obj.setSize(180, 180));
    test("setWidth", () => obj.setWidth(200));
    test("setHeight", () => obj.setHeight(200));
    test("setContentWidth", () => obj.setContentWidth(220));
    test("setContentHeight", () => obj.setContentHeight(220));
    test("refrSize", () => obj.refrSize());

    test("getX -> number", () => {
        let v = obj.getX();
        if (typeof v !== "number") throw new Error("expected number, got " + typeof v);
    });
    test("getX2 -> number", () => {
        if (typeof obj.getX2() !== "number") throw new Error("type");
    });
    test("getY -> number", () => {
        if (typeof obj.getY() !== "number") throw new Error("type");
    });
    test("getY2 -> number", () => {
        if (typeof obj.getY2() !== "number") throw new Error("type");
    });
    test("getXAligned -> number", () => {
        if (typeof obj.getXAligned() !== "number") throw new Error("type");
    });
    test("getYAligned -> number", () => {
        if (typeof obj.getYAligned() !== "number") throw new Error("type");
    });
    test("getWidth -> number", () => {
        if (typeof obj.getWidth() !== "number") throw new Error("type");
    });
    test("getHeight -> number", () => {
        if (typeof obj.getHeight() !== "number") throw new Error("type");
    });
    test("getContentWidth -> number", () => {
        if (typeof obj.getContentWidth() !== "number") throw new Error("type");
    });
    test("getContentHeight -> number", () => {
        if (typeof obj.getContentHeight() !== "number") throw new Error("type");
    });
    test("getSelfWidth -> number", () => {
        if (typeof obj.getSelfWidth() !== "number") throw new Error("type");
    });
    test("getSelfHeight -> number", () => {
        if (typeof obj.getSelfHeight() !== "number") throw new Error("type");
    });

    /* ---- 3. Alignment --------------------------------------------- */
    test("center", () => obj.center());
    test("align", () => obj.align(lv.ALIGN_TOP_MID, 0, 10));
    test("alignTo", () => obj.alignTo(sib, lv.ALIGN_OUT_RIGHT_MID, 5, 0));
    test("setAlign", () => obj.setAlign(lv.ALIGN_CENTER));

    /* ---- 4. Layout ------------------------------------------------- */
    test("setLayout(NONE)", () => obj.setLayout(lv.LAYOUT_NONE));
    test("isLayoutPositioned -> bool", () => {
        let r = obj.isLayoutPositioned();
        if (typeof r !== "boolean") throw new Error("expected boolean");
    });
    test("markLayoutAsDirty", () => obj.markLayoutAsDirty());
    test("updateLayout", () => obj.updateLayout());

    /* ---- 5. Flex layout ------------------------------------------- */
    let flexBox;
    test("flex: new lv.obj for flex test", () => {
        flexBox = new lv.obj(parent);
        flexBox.setSize(200, 100);
    });
    test("setFlexFlow", () => flexBox.setFlexFlow(lv.FLEX_FLOW_ROW));
    test("setFlexAlign", () => flexBox.setFlexAlign(lv.FLEX_ALIGN_START, lv.FLEX_ALIGN_CENTER, lv.FLEX_ALIGN_START));
    test("setFlexGrow", () => {
        let fc = new lv.obj(flexBox);
        fc.setFlexGrow(1);
    });

    /* ---- 6. Hierarchy --------------------------------------------- */
    test("getParent -> obj handle", () => {
        let p = obj.getParent();
        if (!p) throw new Error("null");
    });
    test("getScreen -> handle", () => {
        let s = obj.getScreen();
        if (!s) throw new Error("null");
    });
    test("getDisplay -> handle", () => {
        let d = obj.getDisplay();
        if (!d) throw new Error("null");
    });
    test("getChildCount -> number", () => {
        let n = parent.getChildCount();
        if (typeof n !== "number") throw new Error("type");
        if (n < 2) throw new Error("expected >= 2 children, got " + n);
    });
    test("getChild(0) -> handle", () => {
        let c = parent.getChild(0);
        if (!c) throw new Error("null");
    });
    test("getIndex -> number", () => {
        let idx = obj.getIndex();
        if (typeof idx !== "number") throw new Error("type");
    });
    test("getSibling -> handle", () => {
        let s = obj.getSibling(1);
        // may be null if no sibling at index 1, just check no throw
    });
    test("moveToIndex", () => obj.moveToIndex(0));
    test("setParent", () => {
        // reparent to scr then back to parent
        obj.setParent(scr);
        obj.setParent(parent);
    });

    /* ---- 7. Style: setStyle / getStyle sample --------------------- */
    let PM = lv.PART_MAIN;

    test("setStyleBgColor", () => obj.setStyleBgColor(0x003366, PM));
    test("setStyleBgOpa", () => obj.setStyleBgOpa(200, PM));
    test("setStyleBorderWidth", () => obj.setStyleBorderWidth(2, PM));
    test("setStyleBorderColor", () => obj.setStyleBorderColor(0xFF0000, PM));
    test("setStyleRadius", () => obj.setStyleRadius(8, PM));
    test("setStylePadAll", () => obj.setStylePadAll(4, PM));
    test("setStylePadTop", () => obj.setStylePadTop(2, PM));
    test("setStylePadBottom", () => obj.setStylePadBottom(2, PM));
    test("setStylePadLeft", () => obj.setStylePadLeft(2, PM));
    test("setStylePadRight", () => obj.setStylePadRight(2, PM));
    test("setStylePadRow", () => obj.setStylePadRow(2, PM));
    test("setStylePadColumn", () => obj.setStylePadColumn(2, PM));
    test("setStylePadHor", () => obj.setStylePadHor(2, PM));
    test("setStylePadVer", () => obj.setStylePadVer(2, PM));
    test("setStylePadGap", () => obj.setStylePadGap(2, PM));
    test("setStyleMarginTop", () => obj.setStyleMarginTop(1, PM));
    test("setStyleMarginBottom", () => obj.setStyleMarginBottom(1, PM));
    test("setStyleMarginLeft", () => obj.setStyleMarginLeft(1, PM));
    test("setStyleMarginRight", () => obj.setStyleMarginRight(1, PM));
    test("setStyleMarginAll", () => obj.setStyleMarginAll(1, PM));
    test("setStyleMarginHor", () => obj.setStyleMarginHor(1, PM));
    test("setStyleMarginVer", () => obj.setStyleMarginVer(1, PM));
    test("setStyleOpa", () => obj.setStyleOpa(255, PM));
    test("setStyleOpaLayered", () => obj.setStyleOpaLayered(255, PM));
    test("setStyleTextColor", () => obj.setStyleTextColor(0xFFFFFF, PM));
    test("setStyleTextOpa", () => obj.setStyleTextOpa(255, PM));
    test("setStyleTextLetterSpace", () => obj.setStyleTextLetterSpace(0, PM));
    test("setStyleTextLineSpace", () => obj.setStyleTextLineSpace(2, PM));
    test("setStyleTextDecor", () => obj.setStyleTextDecor(0, PM));
    test("setStyleTextAlign", () => obj.setStyleTextAlign(0, PM));
    test("setStyleShadowWidth", () => obj.setStyleShadowWidth(4, PM));
    test("setStyleShadowOffsetX", () => obj.setStyleShadowOffsetX(2, PM));
    test("setStyleShadowOffsetY", () => obj.setStyleShadowOffsetY(2, PM));
    test("setStyleShadowSpread", () => obj.setStyleShadowSpread(1, PM));
    test("setStyleShadowColor", () => obj.setStyleShadowColor(0x000000, PM));
    test("setStyleShadowOpa", () => obj.setStyleShadowOpa(180, PM));
    test("setStyleOutlineWidth", () => obj.setStyleOutlineWidth(1, PM));
    test("setStyleOutlineColor", () => obj.setStyleOutlineColor(0x00FF00, PM));
    test("setStyleOutlineOpa", () => obj.setStyleOutlineOpa(100, PM));
    test("setStyleOutlinePad", () => obj.setStyleOutlinePad(0, PM));
    test("setStyleBorderOpa", () => obj.setStyleBorderOpa(255, PM));
    test("setStyleBorderSide", () => obj.setStyleBorderSide(0x0F, PM));
    test("setStyleBorderPost", () => obj.setStyleBorderPost(false, PM));
    test("setStyleBgGradColor", () => obj.setStyleBgGradColor(0x006699, PM));
    test("setStyleBgGradDir", () => obj.setStyleBgGradDir(1, PM));
    test("setStyleBgMainStop", () => obj.setStyleBgMainStop(0, PM));
    test("setStyleBgGradStop", () => obj.setStyleBgGradStop(255, PM));
    test("setStyleBgMainOpa", () => obj.setStyleBgMainOpa(255, PM));
    test("setStyleBgGradOpa", () => obj.setStyleBgGradOpa(200, PM));
    test("setStyleBgImageOpa", () => obj.setStyleBgImageOpa(255, PM));
    test("setStyleBgImageRecolor", () => obj.setStyleBgImageRecolor(0x000000, PM));
    test("setStyleBgImageRecolorOpa", () => obj.setStyleBgImageRecolorOpa(0, PM));
    test("setStyleBgImageTiled", () => obj.setStyleBgImageTiled(false, PM));
    test("setStyleImageOpa", () => obj.setStyleImageOpa(255, PM));
    test("setStyleImageRecolor", () => obj.setStyleImageRecolor(0x000000, PM));
    test("setStyleImageRecolorOpa", () => obj.setStyleImageRecolorOpa(0, PM));
    test("setStyleLineWidth", () => obj.setStyleLineWidth(1, PM));
    test("setStyleLineDashWidth", () => obj.setStyleLineDashWidth(4, PM));
    test("setStyleLineDashGap", () => obj.setStyleLineDashGap(2, PM));
    test("setStyleLineRounded", () => obj.setStyleLineRounded(false, PM));
    test("setStyleLineColor", () => obj.setStyleLineColor(0xFFFFFF, PM));
    test("setStyleLineOpa", () => obj.setStyleLineOpa(255, PM));
    test("setStyleArcWidth", () => obj.setStyleArcWidth(4, PM));
    test("setStyleArcRounded", () => obj.setStyleArcRounded(true, PM));
    test("setStyleArcColor", () => obj.setStyleArcColor(0x00AAFF, PM));
    test("setStyleArcOpa", () => obj.setStyleArcOpa(255, PM));
    test("setStyleClipCorner", () => obj.setStyleClipCorner(false, PM));
    test("setStyleBlendMode", () => obj.setStyleBlendMode(0, PM));
    test("setStyleBaseDir", () => obj.setStyleBaseDir(0, PM));
    test("setStyleAnimDuration", () => obj.setStyleAnimDuration(200, PM));
    test("setStyleSize", () => obj.setStyleSize(200, 200, PM));
    test("setStyleTransformWidth", () => obj.setStyleTransformWidth(0, PM));
    test("setStyleTransformHeight", () => obj.setStyleTransformHeight(0, PM));
    test("setStyleTranslateX", () => obj.setStyleTranslateX(0, PM));
    test("setStyleTranslateY", () => obj.setStyleTranslateY(0, PM));
    test("setStyleTransformScaleX", () => obj.setStyleTransformScaleX(256, PM));
    test("setStyleTransformScaleY", () => obj.setStyleTransformScaleY(256, PM));
    test("setStyleTransformRotation", () => obj.setStyleTransformRotation(0, PM));
    test("setStyleTransformPivotX", () => obj.setStyleTransformPivotX(0, PM));
    test("setStyleTransformPivotY", () => obj.setStyleTransformPivotY(0, PM));
    test("setStyleTransformSkewX", () => obj.setStyleTransformSkewX(0, PM));
    test("setStyleTransformSkewY", () => obj.setStyleTransformSkewY(0, PM));
    test("setStyleTransformScale", () => obj.setStyleTransformScale(256, PM));
    test("setStyleX", () => obj.setStyleX(0, PM));
    test("setStyleY", () => obj.setStyleY(0, PM));
    test("setStyleAlign", () => obj.setStyleAlign(lv.ALIGN_TOP_MID, PM));
    test("setStyleWidth", () => obj.setStyleWidth(200, PM));
    test("setStyleMinWidth", () => obj.setStyleMinWidth(0, PM));
    test("setStyleMaxWidth", () => obj.setStyleMaxWidth(400, PM));
    test("setStyleHeight", () => obj.setStyleHeight(200, PM));
    test("setStyleMinHeight", () => obj.setStyleMinHeight(0, PM));
    test("setStyleMaxHeight", () => obj.setStyleMaxHeight(400, PM));
    test("setStyleLength", () => obj.setStyleLength(0, PM));
    test("setStyleRotarySensitivity", () => obj.setStyleRotarySensitivity(256, PM));
    test("setStyleLayout", () => obj.setStyleLayout(lv.LAYOUT_NONE, PM));
    test("setStyleFlexFlow", () => obj.setStyleFlexFlow(lv.FLEX_FLOW_ROW, PM));
    test("setStyleFlexMainPlace", () => obj.setStyleFlexMainPlace(lv.FLEX_ALIGN_START, PM));
    test("setStyleFlexCrossPlace", () => obj.setStyleFlexCrossPlace(lv.FLEX_ALIGN_CENTER, PM));
    test("setStyleFlexTrackPlace", () => obj.setStyleFlexTrackPlace(lv.FLEX_ALIGN_START, PM));
    test("setStyleFlexGrow", () => obj.setStyleFlexGrow(0, PM));
    test("setStyleGridColumnAlign", () => obj.setStyleGridColumnAlign(lv.GRID_ALIGN_START, PM));
    test("setStyleGridRowAlign", () => obj.setStyleGridRowAlign(lv.GRID_ALIGN_START, PM));
    test("setStyleGridCellColumnPos", () => obj.setStyleGridCellColumnPos(0, PM));
    test("setStyleGridCellXAlign", () => obj.setStyleGridCellXAlign(lv.GRID_ALIGN_START, PM));
    test("setStyleGridCellColumnSpan", () => obj.setStyleGridCellColumnSpan(1, PM));
    test("setStyleGridCellRowPos", () => obj.setStyleGridCellRowPos(0, PM));
    test("setStyleGridCellYAlign", () => obj.setStyleGridCellYAlign(lv.GRID_ALIGN_START, PM));
    test("setStyleGridCellRowSpan", () => obj.setStyleGridCellRowSpan(1, PM));
    test("setStyleColorFilterOpa", () => obj.setStyleColorFilterOpa(255, PM));

    /* ---- 8. Style getters (representative sample) ----------------- */
    test("getStyleBgOpa -> number", () => {
        if (typeof obj.getStyleBgOpa(PM) !== "number") throw new Error("type");
    });
    test("getStyleRadius -> number", () => {
        if (typeof obj.getStyleRadius(PM) !== "number") throw new Error("type");
    });
    test("getStyleBorderWidth -> number", () => {
        if (typeof obj.getStyleBorderWidth(PM) !== "number") throw new Error("type");
    });
    test("getStyleOpa -> number", () => {
        if (typeof obj.getStyleOpa(PM) !== "number") throw new Error("type");
    });
    test("getStyleOpaLayered -> number", () => {
        if (typeof obj.getStyleOpaLayered(PM) !== "number") throw new Error("type");
    });
    test("getStyleWidth -> number", () => {
        if (typeof obj.getStyleWidth(PM) !== "number") throw new Error("type");
    });
    test("getStyleHeight -> number", () => {
        if (typeof obj.getStyleHeight(PM) !== "number") throw new Error("type");
    });
    test("getStyleMinWidth -> number", () => {
        if (typeof obj.getStyleMinWidth(PM) !== "number") throw new Error("type");
    });
    test("getStyleMaxWidth -> number", () => {
        if (typeof obj.getStyleMaxWidth(PM) !== "number") throw new Error("type");
    });
    test("getStyleMinHeight -> number", () => {
        if (typeof obj.getStyleMinHeight(PM) !== "number") throw new Error("type");
    });
    test("getStyleMaxHeight -> number", () => {
        if (typeof obj.getStyleMaxHeight(PM) !== "number") throw new Error("type");
    });
    test("getStyleLength -> number", () => {
        if (typeof obj.getStyleLength(PM) !== "number") throw new Error("type");
    });
    test("getStyleX -> number", () => {
        if (typeof obj.getStyleX(PM) !== "number") throw new Error("type");
    });
    test("getStyleY -> number", () => {
        if (typeof obj.getStyleY(PM) !== "number") throw new Error("type");
    });
    test("getStyleAlign -> number", () => {
        if (typeof obj.getStyleAlign(PM) !== "number") throw new Error("type");
    });
    test("getStylePadTop -> number", () => {
        if (typeof obj.getStylePadTop(PM) !== "number") throw new Error("type");
    });
    test("getStylePadBottom -> number", () => {
        if (typeof obj.getStylePadBottom(PM) !== "number") throw new Error("type");
    });
    test("getStylePadLeft -> number", () => {
        if (typeof obj.getStylePadLeft(PM) !== "number") throw new Error("type");
    });
    test("getStylePadRight -> number", () => {
        if (typeof obj.getStylePadRight(PM) !== "number") throw new Error("type");
    });
    test("getStylePadRow -> number", () => {
        if (typeof obj.getStylePadRow(PM) !== "number") throw new Error("type");
    });
    test("getStylePadColumn -> number", () => {
        if (typeof obj.getStylePadColumn(PM) !== "number") throw new Error("type");
    });
    test("getStyleMarginTop -> number", () => {
        if (typeof obj.getStyleMarginTop(PM) !== "number") throw new Error("type");
    });
    test("getStyleMarginBottom -> number", () => {
        if (typeof obj.getStyleMarginBottom(PM) !== "number") throw new Error("type");
    });
    test("getStyleMarginLeft -> number", () => {
        if (typeof obj.getStyleMarginLeft(PM) !== "number") throw new Error("type");
    });
    test("getStyleMarginRight -> number", () => {
        if (typeof obj.getStyleMarginRight(PM) !== "number") throw new Error("type");
    });
    test("getStyleBgGradDir -> number", () => {
        if (typeof obj.getStyleBgGradDir(PM) !== "number") throw new Error("type");
    });
    test("getStyleBgMainStop -> number", () => {
        if (typeof obj.getStyleBgMainStop(PM) !== "number") throw new Error("type");
    });
    test("getStyleBgGradStop -> number", () => {
        if (typeof obj.getStyleBgGradStop(PM) !== "number") throw new Error("type");
    });
    test("getStyleBgMainOpa -> number", () => {
        if (typeof obj.getStyleBgMainOpa(PM) !== "number") throw new Error("type");
    });
    test("getStyleBgGradOpa -> number", () => {
        if (typeof obj.getStyleBgGradOpa(PM) !== "number") throw new Error("type");
    });
    test("getStyleBgImageOpa -> number", () => {
        if (typeof obj.getStyleBgImageOpa(PM) !== "number") throw new Error("type");
    });
    test("getStyleBgImageTiled -> bool", () => {
        let v = obj.getStyleBgImageTiled(PM);
        if (typeof v !== "boolean") throw new Error("type");
    });
    test("getStyleBorderOpa -> number", () => {
        if (typeof obj.getStyleBorderOpa(PM) !== "number") throw new Error("type");
    });
    test("getStyleBorderSide -> number", () => {
        if (typeof obj.getStyleBorderSide(PM) !== "number") throw new Error("type");
    });
    test("getStyleBorderPost -> bool", () => {
        let v = obj.getStyleBorderPost(PM);
        if (typeof v !== "boolean") throw new Error("type");
    });
    test("getStyleOutlineWidth -> number", () => {
        if (typeof obj.getStyleOutlineWidth(PM) !== "number") throw new Error("type");
    });
    test("getStyleOutlineOpa -> number", () => {
        if (typeof obj.getStyleOutlineOpa(PM) !== "number") throw new Error("type");
    });
    test("getStyleOutlinePad -> number", () => {
        if (typeof obj.getStyleOutlinePad(PM) !== "number") throw new Error("type");
    });
    test("getStyleShadowWidth -> number", () => {
        if (typeof obj.getStyleShadowWidth(PM) !== "number") throw new Error("type");
    });
    test("getStyleShadowOffsetX -> number", () => {
        if (typeof obj.getStyleShadowOffsetX(PM) !== "number") throw new Error("type");
    });
    test("getStyleShadowOffsetY -> number", () => {
        if (typeof obj.getStyleShadowOffsetY(PM) !== "number") throw new Error("type");
    });
    test("getStyleShadowSpread -> number", () => {
        if (typeof obj.getStyleShadowSpread(PM) !== "number") throw new Error("type");
    });
    test("getStyleShadowOpa -> number", () => {
        if (typeof obj.getStyleShadowOpa(PM) !== "number") throw new Error("type");
    });
    test("getStyleImageOpa -> number", () => {
        if (typeof obj.getStyleImageOpa(PM) !== "number") throw new Error("type");
    });
    test("getStyleLineWidth -> number", () => {
        if (typeof obj.getStyleLineWidth(PM) !== "number") throw new Error("type");
    });
    test("getStyleLineDashWidth -> number", () => {
        if (typeof obj.getStyleLineDashWidth(PM) !== "number") throw new Error("type");
    });
    test("getStyleLineDashGap -> number", () => {
        if (typeof obj.getStyleLineDashGap(PM) !== "number") throw new Error("type");
    });
    test("getStyleLineRounded -> bool", () => {
        let v = obj.getStyleLineRounded(PM);
        if (typeof v !== "boolean") throw new Error("type");
    });
    test("getStyleLineOpa -> number", () => {
        if (typeof obj.getStyleLineOpa(PM) !== "number") throw new Error("type");
    });
    test("getStyleArcWidth -> number", () => {
        if (typeof obj.getStyleArcWidth(PM) !== "number") throw new Error("type");
    });
    test("getStyleArcRounded -> bool", () => {
        let v = obj.getStyleArcRounded(PM);
        if (typeof v !== "boolean") throw new Error("type");
    });
    test("getStyleArcOpa -> number", () => {
        if (typeof obj.getStyleArcOpa(PM) !== "number") throw new Error("type");
    });
    test("getStyleTextOpa -> number", () => {
        if (typeof obj.getStyleTextOpa(PM) !== "number") throw new Error("type");
    });
    test("getStyleTextLetterSpace -> number", () => {
        if (typeof obj.getStyleTextLetterSpace(PM) !== "number") throw new Error("type");
    });
    test("getStyleTextLineSpace -> number", () => {
        if (typeof obj.getStyleTextLineSpace(PM) !== "number") throw new Error("type");
    });
    test("getStyleTextDecor -> number", () => {
        if (typeof obj.getStyleTextDecor(PM) !== "number") throw new Error("type");
    });
    test("getStyleTextAlign -> number", () => {
        if (typeof obj.getStyleTextAlign(PM) !== "number") throw new Error("type");
    });
    test("getStyleClipCorner -> bool", () => {
        let v = obj.getStyleClipCorner(PM);
        if (typeof v !== "boolean") throw new Error("type");
    });
    test("getStyleBlendMode -> number", () => {
        if (typeof obj.getStyleBlendMode(PM) !== "number") throw new Error("type");
    });
    test("getStyleAnimDuration -> number", () => {
        if (typeof obj.getStyleAnimDuration(PM) !== "number") throw new Error("type");
    });
    test("getStyleLayout -> number", () => {
        if (typeof obj.getStyleLayout(PM) !== "number") throw new Error("type");
    });
    test("getStyleBaseDir -> number", () => {
        if (typeof obj.getStyleBaseDir(PM) !== "number") throw new Error("type");
    });
    test("getStyleRotarySensitivity -> number", () => {
        if (typeof obj.getStyleRotarySensitivity(PM) !== "number") throw new Error("type");
    });
    test("getStyleTransformWidth -> number", () => {
        if (typeof obj.getStyleTransformWidth(PM) !== "number") throw new Error("type");
    });
    test("getStyleTransformHeight -> number", () => {
        if (typeof obj.getStyleTransformHeight(PM) !== "number") throw new Error("type");
    });
    test("getStyleTranslateX -> number", () => {
        if (typeof obj.getStyleTranslateX(PM) !== "number") throw new Error("type");
    });
    test("getStyleTranslateY -> number", () => {
        if (typeof obj.getStyleTranslateY(PM) !== "number") throw new Error("type");
    });
    test("getStyleTransformScaleX -> number", () => {
        if (typeof obj.getStyleTransformScaleX(PM) !== "number") throw new Error("type");
    });
    test("getStyleTransformScaleY -> number", () => {
        if (typeof obj.getStyleTransformScaleY(PM) !== "number") throw new Error("type");
    });
    test("getStyleTransformRotation -> number", () => {
        if (typeof obj.getStyleTransformRotation(PM) !== "number") throw new Error("type");
    });
    test("getStyleTransformPivotX -> number", () => {
        if (typeof obj.getStyleTransformPivotX(PM) !== "number") throw new Error("type");
    });
    test("getStyleTransformPivotY -> number", () => {
        if (typeof obj.getStyleTransformPivotY(PM) !== "number") throw new Error("type");
    });
    test("getStyleTransformSkewX -> number", () => {
        if (typeof obj.getStyleTransformSkewX(PM) !== "number") throw new Error("type");
    });
    test("getStyleTransformSkewY -> number", () => {
        if (typeof obj.getStyleTransformSkewY(PM) !== "number") throw new Error("type");
    });
    test("getStyleSpaceLeft -> number", () => {
        if (typeof obj.getStyleSpaceLeft(PM) !== "number") throw new Error("type");
    });
    test("getStyleSpaceRight -> number", () => {
        if (typeof obj.getStyleSpaceRight(PM) !== "number") throw new Error("type");
    });
    test("getStyleSpaceTop -> number", () => {
        if (typeof obj.getStyleSpaceTop(PM) !== "number") throw new Error("type");
    });
    test("getStyleSpaceBottom -> number", () => {
        if (typeof obj.getStyleSpaceBottom(PM) !== "number") throw new Error("type");
    });
    test("getStyleColorFilterOpa -> number", () => {
        if (typeof obj.getStyleColorFilterOpa(PM) !== "number") throw new Error("type");
    });
    test("getStyleFlexFlow -> number", () => {
        if (typeof obj.getStyleFlexFlow(PM) !== "number") throw new Error("type");
    });
    test("getStyleFlexMainPlace -> number", () => {
        if (typeof obj.getStyleFlexMainPlace(PM) !== "number") throw new Error("type");
    });
    test("getStyleFlexCrossPlace -> number", () => {
        if (typeof obj.getStyleFlexCrossPlace(PM) !== "number") throw new Error("type");
    });
    test("getStyleFlexTrackPlace -> number", () => {
        if (typeof obj.getStyleFlexTrackPlace(PM) !== "number") throw new Error("type");
    });
    test("getStyleFlexGrow -> number", () => {
        if (typeof obj.getStyleFlexGrow(PM) !== "number") throw new Error("type");
    });
    test("getStyleGridColumnAlign -> number", () => {
        if (typeof obj.getStyleGridColumnAlign(PM) !== "number") throw new Error("type");
    });
    test("getStyleGridRowAlign -> number", () => {
        if (typeof obj.getStyleGridRowAlign(PM) !== "number") throw new Error("type");
    });
    test("getStyleGridCellColumnPos -> number", () => {
        if (typeof obj.getStyleGridCellColumnPos(PM) !== "number") throw new Error("type");
    });
    test("getStyleGridCellXAlign -> number", () => {
        if (typeof obj.getStyleGridCellXAlign(PM) !== "number") throw new Error("type");
    });
    test("getStyleGridCellColumnSpan -> number", () => {
        if (typeof obj.getStyleGridCellColumnSpan(PM) !== "number") throw new Error("type");
    });
    test("getStyleGridCellRowPos -> number", () => {
        if (typeof obj.getStyleGridCellRowPos(PM) !== "number") throw new Error("type");
    });
    test("getStyleGridCellYAlign -> number", () => {
        if (typeof obj.getStyleGridCellYAlign(PM) !== "number") throw new Error("type");
    });
    test("getStyleGridCellRowSpan -> number", () => {
        if (typeof obj.getStyleGridCellRowSpan(PM) !== "number") throw new Error("type");
    });
    test("getStyleTransformScaleXSafe -> number", () => {
        if (typeof obj.getStyleTransformScaleXSafe(PM) !== "number") throw new Error("type");
    });
    test("getStyleTransformScaleYSafe -> number", () => {
        if (typeof obj.getStyleTransformScaleYSafe(PM) !== "number") throw new Error("type");
    });
    test("getStyleOpaRecursive -> number", () => {
        if (typeof obj.getStyleOpaRecursive(PM) !== "number") throw new Error("type");
    });
    test("calculateStyleTextAlign -> number", () => {
        if (typeof obj.calculateStyleTextAlign(PM, "abc") !== "number") throw new Error("type");
    });

    /* ---- 9. Style ops ---------------------------------------------- */
    test("removeStyleAll", () => obj.removeStyleAll());
    test("refreshStyle", () => obj.refreshStyle(PM, lv.STYLE_PROP_ANY));
    test("hasStyleProp -> bool", () => {
        let r = obj.hasStyleProp(PM, lv.STYLE_BG_COLOR);
        if (typeof r !== "boolean") throw new Error("type");
    });
    test("calculateExtDrawSize -> number", () => {
        let v = obj.calculateExtDrawSize(PM);
        if (typeof v !== "number") throw new Error("type");
    });
    test("refreshExtDrawSize", () => obj.refreshExtDrawSize());

    /* ---- 10. Fade -------------------------------------------------- */
    test("fadeIn", () => obj.fadeIn(200, 0));
    test("fadeOut", () => obj.fadeOut(200, 0));

    /* ---- 11. Flags ------------------------------------------------- */
    test("addFlag(CLICKABLE)", () => obj.addFlag(lv.OBJ_FLAG_CLICKABLE));
    test("hasFlag(CLICKABLE) -> true", () => {
        if (!obj.hasFlag(lv.OBJ_FLAG_CLICKABLE)) throw new Error("expected true");
    });
    test("hasFlagAny(CLICKABLE) -> true", () => {
        if (!obj.hasFlagAny(lv.OBJ_FLAG_CLICKABLE)) throw new Error("expected true");
    });
    test("addFlag(CHECKABLE)", () => obj.addFlag(lv.OBJ_FLAG_CHECKABLE));
    test("removeFlag(CHECKABLE)", () => obj.removeFlag(lv.OBJ_FLAG_CHECKABLE));
    test("hasFlag(CHECKABLE) -> false after remove", () => {
        if (obj.hasFlag(lv.OBJ_FLAG_CHECKABLE)) throw new Error("expected false");
    });
    test("updateFlag(HIDDEN, false)", () => obj.updateFlag(lv.OBJ_FLAG_HIDDEN, false));
    test("updateFlag(SCROLLABLE, true)", () => obj.updateFlag(lv.OBJ_FLAG_SCROLLABLE, true));

    /* ---- 12. State ------------------------------------------------- */
    test("addState(PRESSED)", () => obj.addState(lv.STATE_PRESSED));
    test("hasState(PRESSED) -> true", () => {
        if (!obj.hasState(lv.STATE_PRESSED)) throw new Error("expected true");
    });
    test("removeState(PRESSED)", () => obj.removeState(lv.STATE_PRESSED));
    test("hasState(PRESSED) -> false after remove", () => {
        if (obj.hasState(lv.STATE_PRESSED)) throw new Error("expected false");
    });
    test("getState -> number", () => {
        if (typeof obj.getState() !== "number") throw new Error("type");
    });
    test("setState(DEFAULT, true)", () => obj.setState(lv.STATE_DEFAULT, true));
    test("addState(DISABLED) then removeState", () => {
        obj.addState(lv.STATE_DISABLED);
        obj.removeState(lv.STATE_DISABLED);
    });

    /* ---- 13. Events ----------------------------------------------- */
    let cbFired = false;
    let cb1 = function(e) { cbFired = true; };
    let dsc1;
    test("addEventCb -> handle", () => {
        dsc1 = obj.addEventCb(cb1, lv.EVENT_CLICKED, null);
        if (!dsc1) throw new Error("null dsc");
    });
    test("getEventCount -> 1", () => {
        let n = obj.getEventCount();
        if (typeof n !== "number") throw new Error("type");
        if (n < 1) throw new Error("count=" + n);
    });
    test("getEventDsc(0) -> handle", () => {
        let d = obj.getEventDsc(0);
        if (!d) throw new Error("null");
    });
    test("sendEvent(CLICKED) fires callback", () => {
        cbFired = false;
        obj.sendEvent(lv.EVENT_CLICKED, null);
        if (!cbFired) throw new Error("callback not fired");
    });
    test("removeEventCb(cb) -> bool", () => {
        let r = obj.removeEventCb(cb1);
        if (typeof r !== "boolean") throw new Error("type");
    });

    // addEventCb + removeEventDsc
    let cb2 = function(e) {};
    let dsc2;
    test("addEventCb (for dsc remove)", () => {
        dsc2 = obj.addEventCb(cb2, lv.EVENT_ALL, null);
        if (!dsc2) throw new Error("null");
    });
    test("removeEventDsc -> bool", () => {
        let r = obj.removeEventDsc(dsc2);
        if (typeof r !== "boolean") throw new Error("type");
    });

    // addEventCb + removeEvent
    let cb3 = function(e) {};
    test("addEventCb (for removeEvent)", () => {
        obj.addEventCb(cb3, lv.EVENT_ALL, null);
    });
    test("removeEvent(0) -> bool", () => {
        let r = obj.removeEvent(0);
        if (typeof r !== "boolean") throw new Error("type");
    });

    // addEventCb + removeEventCbWithUserData
    let cb4 = function(e) {};
    let ud = {tag: "test"};
    test("addEventCb with user_data", () => {
        obj.addEventCb(cb4, lv.EVENT_ALL, ud);
    });
    test("removeEventCbWithUserData -> number", () => {
        let n = obj.removeEventCbWithUserData(cb4, ud);
        if (typeof n !== "number") throw new Error("type");
    });

    /* ---- 14. User data -------------------------------------------- */
    test("setUserData + getUserData", () => {
        obj.setUserData({key: "value", num: 42});
        let d = obj.getUserData();
        if (typeof d !== "object") throw new Error("type " + typeof d);
    });

    /* ---- 15. Scroll ----------------------------------------------- */
    // Make parent large enough for scroll to have effect
    test("setScrollbarMode(AUTO)", () => parent.setScrollbarMode(lv.SCROLLBAR_MODE_AUTO));
    test("setScrollDir(ALL)", () => parent.setScrollDir(lv.DIR_ALL));
    test("setScrollSnapX(NONE)", () => parent.setScrollSnapX(lv.SCROLL_SNAP_NONE));
    test("setScrollSnapY(NONE)", () => parent.setScrollSnapY(lv.SCROLL_SNAP_NONE));
    test("getScrollbarMode -> number", () => {
        if (typeof parent.getScrollbarMode() !== "number") throw new Error("type");
    });
    test("getScrollDir -> number", () => {
        if (typeof parent.getScrollDir() !== "number") throw new Error("type");
    });
    test("getScrollSnapX -> number", () => {
        if (typeof parent.getScrollSnapX() !== "number") throw new Error("type");
    });
    test("getScrollSnapY -> number", () => {
        if (typeof parent.getScrollSnapY() !== "number") throw new Error("type");
    });
    test("getScrollX -> number", () => {
        if (typeof parent.getScrollX() !== "number") throw new Error("type");
    });
    test("getScrollY -> number", () => {
        if (typeof parent.getScrollY() !== "number") throw new Error("type");
    });
    test("getScrollTop -> number", () => {
        if (typeof parent.getScrollTop() !== "number") throw new Error("type");
    });
    test("getScrollBottom -> number", () => {
        if (typeof parent.getScrollBottom() !== "number") throw new Error("type");
    });
    test("getScrollLeft -> number", () => {
        if (typeof parent.getScrollLeft() !== "number") throw new Error("type");
    });
    test("getScrollRight -> number", () => {
        if (typeof parent.getScrollRight() !== "number") throw new Error("type");
    });
    test("scrollBy", () => parent.scrollBy(5, 0, lv.ANIM_OFF));
    test("scrollByBounded", () => parent.scrollByBounded(0, 5, lv.ANIM_OFF));
    test("scrollTo", () => parent.scrollTo(0, 0, lv.ANIM_OFF));
    test("scrollToX", () => parent.scrollToX(0, lv.ANIM_OFF));
    test("scrollToY", () => parent.scrollToY(0, lv.ANIM_OFF));
    test("scrollToView", () => obj.scrollToView(lv.ANIM_OFF));
    test("scrollToViewRecursive", () => obj.scrollToViewRecursive(lv.ANIM_OFF));
    test("isScrolling -> bool", () => {
        if (typeof parent.isScrolling() !== "boolean") throw new Error("type");
    });
    test("updateSnap", () => parent.updateSnap(lv.ANIM_OFF));
    test("scrollbarInvalidate", () => parent.scrollbarInvalidate());
    test("readjustScroll", () => parent.readjustScroll(lv.ANIM_OFF));
    test("getScrollEnd -> object", () => {
        let v = parent.getScrollEnd();
        if (typeof v !== "object") throw new Error("type");
    });
    test("getScrollbarArea -> call no throw", () => {
        parent.getScrollbarArea();
    });

    /* ---- 16. Coords & Visibility ---------------------------------- */
    test("getCoords -> object", () => {
        let a = obj.getCoords();
        if (typeof a !== "object") throw new Error("type");
    });
    test("getContentCoords -> object", () => {
        let a = obj.getContentCoords();
        if (typeof a !== "object") throw new Error("type");
    });
    test("setExtClickArea", () => obj.setExtClickArea(5));
    test("getClickArea -> object", () => {
        let a = obj.getClickArea();
        if (typeof a !== "object") throw new Error("type");
    });
    test("isVisible -> bool", () => {
        if (typeof obj.isVisible() !== "boolean") throw new Error("type");
    });
    test("invalidate", () => obj.invalidate());
    test("invalidateArea", () => {
        obj.invalidateArea({x1: 0, y1: 0, x2: 10, y2: 10});
    });
    test("areaIsVisible -> bool", () => {
        let r = obj.areaIsVisible({x1: 0, y1: 0, x2: 10, y2: 10});
        if (typeof r !== "boolean") throw new Error("type");
    });
    test("hitTest -> bool", () => {
        let r = obj.hitTest({x: 10, y: 10});
        if (typeof r !== "boolean") throw new Error("type");
    });

    /* ---- 17. Movement --------------------------------------------- */
    test("moveForeground", () => obj.moveForeground());
    test("moveBackground", () => obj.moveBackground());
    test("moveTo", () => obj.moveTo(0, 0));
    test("moveChildrenBy", () => obj.moveChildrenBy(0, 0, false));
    test("refreshSelfSize", () => obj.refreshSelfSize());
    test("refrPos", () => obj.refrPos());

    /* ---- 18. Class & validity ------------------------------------- */
    test("isValid -> true", () => {
        if (!obj.isValid()) throw new Error("expected true");
    });
    test("getClass -> handle", () => {
        let c = obj.getClass();
        if (!c) throw new Error("null");
    });
    test("isEditable -> bool", () => {
        if (typeof obj.isEditable() !== "boolean") throw new Error("type");
    });
    test("isGroupDef -> bool", () => {
        if (typeof obj.isGroupDef() !== "boolean") throw new Error("type");
    });
    test("allocateSpecAttr", () => obj.allocateSpecAttr());

    /* ---- 19. Misc -------------------------------------------------- */
    test("getGroup -> handle or null", () => {
        obj.getGroup(); // may return null handle when not in group
    });

    /* ---- 20. Properties (prototype getter/setter) ----------------- */
    test("prop x get", () => {
        let v = obj.x;
        if (typeof v !== "number") throw new Error("type");
    });
    test("prop x set", () => { obj.x = 5; });
    test("prop y get", () => {
        if (typeof obj.y !== "number") throw new Error("type");
    });
    test("prop y set", () => { obj.y = 5; });
    test("prop x2 get", () => {
        if (typeof obj.x2 !== "number") throw new Error("type");
    });
    test("prop y2 get", () => {
        if (typeof obj.y2 !== "number") throw new Error("type");
    });
    test("prop xAligned get", () => {
        if (typeof obj.xAligned !== "number") throw new Error("type");
    });
    test("prop yAligned get", () => {
        if (typeof obj.yAligned !== "number") throw new Error("type");
    });
    test("prop width get", () => {
        if (typeof obj.width !== "number") throw new Error("type");
    });
    test("prop width set", () => { obj.width = 200; });
    test("prop height get", () => {
        if (typeof obj.height !== "number") throw new Error("type");
    });
    test("prop height set", () => { obj.height = 200; });
    test("prop contentWidth get", () => {
        if (typeof obj.contentWidth !== "number") throw new Error("type");
    });
    test("prop contentWidth set", () => { obj.contentWidth = 220; });
    test("prop contentHeight get", () => {
        if (typeof obj.contentHeight !== "number") throw new Error("type");
    });
    test("prop contentHeight set", () => { obj.contentHeight = 220; });
    test("prop selfWidth get", () => {
        if (typeof obj.selfWidth !== "number") throw new Error("type");
    });
    test("prop selfHeight get", () => {
        if (typeof obj.selfHeight !== "number") throw new Error("type");
    });
    test("prop parent get", () => {
        let p = obj.parent;
        if (!p) throw new Error("null");
    });
    test("prop parent set (reparent)", () => {
        obj.parent = parent;
    });
    test("prop screen get", () => {
        let s = obj.screen;
        if (!s) throw new Error("null");
    });
    test("prop display get", () => {
        let d = obj.display;
        if (!d) throw new Error("null");
    });
    test("prop childCount get", () => {
        if (typeof obj.childCount !== "number") throw new Error("type");
    });
    test("prop index get", () => {
        if (typeof obj.index !== "number") throw new Error("type");
    });
    test("prop state get", () => {
        if (typeof obj.state !== "number") throw new Error("type");
    });
    test("prop eventCount get", () => {
        if (typeof obj.eventCount !== "number") throw new Error("type");
    });
    test("prop group get", () => { obj.group; });
    test("prop scrollX get", () => {
        if (typeof parent.scrollX !== "number") throw new Error("type");
    });
    test("prop scrollY get", () => {
        if (typeof parent.scrollY !== "number") throw new Error("type");
    });
    test("prop scrollTop get", () => {
        if (typeof parent.scrollTop !== "number") throw new Error("type");
    });
    test("prop scrollBottom get", () => {
        if (typeof parent.scrollBottom !== "number") throw new Error("type");
    });
    test("prop scrollLeft get", () => {
        if (typeof parent.scrollLeft !== "number") throw new Error("type");
    });
    test("prop scrollRight get", () => {
        if (typeof parent.scrollRight !== "number") throw new Error("type");
    });
    test("prop scrollbarMode get", () => {
        if (typeof parent.scrollbarMode !== "number") throw new Error("type");
    });
    test("prop scrollbarMode set", () => { parent.scrollbarMode = lv.SCROLLBAR_MODE_OFF; });
    test("prop scrollDir get", () => {
        if (typeof parent.scrollDir !== "number") throw new Error("type");
    });
    test("prop scrollDir set", () => { parent.scrollDir = lv.DIR_ALL; });
    test("prop scrollSnapX get", () => {
        if (typeof parent.scrollSnapX !== "number") throw new Error("type");
    });
    test("prop scrollSnapX set", () => { parent.scrollSnapX = lv.SCROLL_SNAP_NONE; });
    test("prop scrollSnapY get", () => {
        if (typeof parent.scrollSnapY !== "number") throw new Error("type");
    });
    test("prop scrollSnapY set", () => { parent.scrollSnapY = lv.SCROLL_SNAP_NONE; });
    test("prop userData get", () => {
        parent.setUserData("hello");
        let v = parent.userData;
        if (v !== "hello") throw new Error("mismatch: " + v);
    });
    test("prop userData set", () => { parent.userData = {a: 1}; });
    test("prop align set", () => { obj.align = lv.ALIGN_CENTER; });
    test("prop extClickArea set", () => { obj.extClickArea = 4; });
    test("prop layout set", () => { obj.layout = lv.LAYOUT_NONE; });
    test("prop flexFlow set", () => { flexBox.flexFlow = lv.FLEX_FLOW_ROW; });
    test("prop flexGrow set", () => {
        let fc = new lv.obj(flexBox);
        fc.flexGrow = 1;
    });

    /* ---- 21. Lifecycle -------------------------------------------- */
    // Add a child to obj, then clean() should remove it
    let childForClean;
    test("clean: create child", () => {
        childForClean = new lv.obj(obj);
        if (obj.getChildCount() < 1) throw new Error("no child created");
    });
    test("clean: removes all children", () => {
        obj.clean();
        if (obj.getChildCount() !== 0) throw new Error("children remain: " + obj.getChildCount());
    });

    // deleteDelayed
    test("deleteDelayed: create and schedule", () => {
        let tmp = new lv.obj(parent);
        tmp.deleteDelayed(500);
    });

    // deleteAsync
    test("deleteAsync: create and schedule", () => {
        let tmp = new lv.obj(parent);
        tmp.deleteAsync();
    });

    /* ---- 22. delete ----------------------------------------------- */
    test("delete sibling", () => { sib.delete(); });
    test("delete flexBox", () => { flexBox.delete(); });
    // Do NOT delete parent or obj here — leave on screen for visibility
    /* Summary                                                           */
    });
}
