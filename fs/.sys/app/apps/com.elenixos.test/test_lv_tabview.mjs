/**
 * lv.tabview coverage test
 */

import { test, log, assertEqual, assertOk, assertType, assertNotNull, runSuite } from './framework.mjs';

export function suite() {
    runSuite('tabview', () => {
    let scr = eos.view.active();

    let tv;
    test('constructor new lv.tabview(scr)', () => {
        tv = new lv.tabview(scr);
        assertNotNull(tv);
    });

    test('setSize and align', () => {
        tv.setSize(350, 350);
        tv.align(lv.ALIGN_CENTER, 0, 0);
        assertOk(true);
    });

    test('addTab returns object', () => {
        let tab = tv.addTab('Tab 1');
        assertNotNull(tab);
    });

    test('add multiple tabs', () => {
        tv.addTab('Tab 2');
        tv.addTab('Tab 3');
        assertOk(true);
    });

    test('renameTab', () => {
        tv.renameTab(1, 'Renamed');
        assertOk(true);
    });

    test('setActive', () => {
        tv.setActive(0, 0);
        assertOk(true);
        tv.setActive(2, 0);
        assertOk(true);
    });

    test('getTabCount returns number', () => {
        let count = tv.getTabCount();
        assertType(count, 'number');
        assertOk(count > 0);
    });

    test('getTabActive returns number', () => {
        let active = tv.getTabActive();
        assertType(active, 'number');
    });

    test('getContent returns object', () => {
        let content = tv.getContent();
        assertNotNull(content);
    });

    test('getTabBar returns object', () => {
        let bar = tv.getTabBar();
        assertNotNull(bar);
    });

    test('setTabBarPosition', () => {
        tv.setTabBarPosition(lv.DIR_BOTTOM);
        assertOk(true);
        tv.setTabBarPosition(lv.DIR_TOP);
        assertOk(true);
    });

    test('setTabBarSize', () => {
        tv.setTabBarSize(30);
        assertOk(true);
    });

    test('property tabActive', () => { tv.tabActive = 1; assertType(tv.tabActive, 'number'); });
    test('property tabCount', () => { assertType(tv.tabCount, 'number'); });
    test('property tabBarPosition', () => { tv.tabBarPosition = lv.DIR_LEFT; assertType(tv.tabBarPosition, 'number'); });
    test('property tabBarSize', () => { tv.tabBarSize = 40; assertType(tv.tabBarSize, 'number'); });
    });
}
