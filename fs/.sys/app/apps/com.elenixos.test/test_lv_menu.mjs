/**
 * lv.menu coverage test
 */

import { test, log, assertEqual, assertOk, assertType, assertNotNull, runSuite } from './framework.mjs';

export function suite() {
    runSuite('menu', () => {
    let scr = eos.view.active();

    let menu;
    test('constructor new lv.menu(scr)', () => {
        menu = new lv.menu(scr);
        assertNotNull(menu);
    });

    test('setSize and align', () => {
        menu.setSize(350, 400);
        menu.align(lv.ALIGN_CENTER, 0, 0);
        assertOk(true);
    });

    test('pageCreate returns object', () => {
        let page = lv.menu.pageCreate(menu, 'Main Page');
        assertNotNull(page);
    });

    test('contCreate returns object', () => {
        let cont = lv.menu.contCreate(menu);
        assertNotNull(cont);
    });

    test('sectionCreate returns object', () => {
        let sec = lv.menu.sectionCreate(menu);
        assertNotNull(sec);
    });

    test('separatorCreate returns object', () => {
        let sep = lv.menu.separatorCreate(menu);
        assertNotNull(sep);
    });

    test('setPage', () => {
        let page = lv.menu.pageCreate(menu, 'Page 1');
        menu.setPage(page);
        assertOk(true);
    });

    test('setPageTitle', () => {
        let page = lv.menu.pageCreate(menu, 'Old');
        lv.menu.setPageTitle(page, 'New Title');
        assertOk(true);
    });

    test('setSidebarPage', () => {
        let sidebar = lv.menu.pageCreate(menu, 'Sidebar');
        menu.setSidebarPage(sidebar);
        assertOk(true);
    });

    test('setModeHeader', () => {
        menu.setModeHeader(lv.MENU_HEADER_TOP_FIXED);
        assertOk(true);
        menu.setModeHeader(lv.MENU_HEADER_TOP_UNFIXED);
        assertOk(true);
        menu.setModeHeader(lv.MENU_HEADER_BOTTOM_FIXED);
        assertOk(true);
    });

    test('setModeRootBackButton', () => {
        menu.setModeRootBackButton(lv.MENU_ROOT_BACK_BUTTON_ENABLED);
        assertOk(true);
        menu.setModeRootBackButton(lv.MENU_ROOT_BACK_BUTTON_DISABLED);
        assertOk(true);
    });

    test('getCurMainPage returns object', () => {
        let p = menu.getCurMainPage();
        assertNotNull(p);
    });

    test('getCurSidebarPage returns object', () => {
        let p = menu.getCurSidebarPage();
        assertNotNull(p);
    });

    test('getMainHeader returns object', () => {
        let h = menu.getMainHeader();
        assertNotNull(h);
    });

    test('getMainHeaderBackButton returns object', () => {
        let btn = menu.getMainHeaderBackButton();
        assertNotNull(btn);
    });

    test('getSidebarHeader returns object', () => {
        let h = menu.getSidebarHeader();
        assertNotNull(h);
    });

    test('getSidebarHeaderBackButton returns object', () => {
        let btn = menu.getSidebarHeaderBackButton();
        assertNotNull(btn);
    });

    test('backButtonIsRoot returns boolean', () => {
        let main = lv.menu.pageCreate(menu, 'root');
        let v = menu.backButtonIsRoot(main);
        assertType(v, 'boolean');
    });

    test('clearHistory', () => {
        menu.clearHistory();
        assertOk(true);
    });

    test('property curMainPage', () => { assertNotNull(menu.curMainPage); });
    test('property curSidebarPage', () => { assertNotNull(menu.curSidebarPage); });
    test('property mainHeader', () => { assertNotNull(menu.mainHeader); });
    test('property sidebarHeader', () => { assertNotNull(menu.sidebarHeader); });
    });
}
