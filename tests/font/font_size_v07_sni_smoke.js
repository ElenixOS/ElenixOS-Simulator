(() => {
    const check = (condition, message) => {
        if (!condition) throw new Error(message);
    };
    const rejects = (call, message) => {
        let rejected = false;
        try { call(); } catch (_) { rejected = true; }
        check(rejected, message);
    };

    check(typeof lv.obj.prototype.setFontSize === 'function', 'obj.setFontSize is missing');
    check(typeof lv.obj.prototype.clearFontSize === 'function', 'obj.clearFontSize is missing');
    check(typeof lv.label.prototype.setFontSize === 'function', 'label.setFontSize is missing');
    check(typeof lv.obj.prototype.setLVGLFontSize === 'undefined', 'legacy API is still registered');

    const activity = eos.activity.create();
    try {
        const view = eos.activity.getView(activity);
        const container = new lv.obj(view);
        const small = new lv.label(container);
        const medium = new lv.label(container);
        const large = new lv.label(container);
        container.setFontSize(24);
        small.setText('中文 English 18');
        medium.setText('中文 English 24');
        large.setText('中文 English 36');
        small.setFontSize(18);
        medium.setFontSize(24);
        large.setFontSize(36);
        small.clearFontSize();
        small.clearFontSize();
        small.setFontSize(18);

        rejects(() => small.setFontSize(18.5), 'fractional size accepted');
        rejects(() => small.setFontSize(0), 'zero size accepted');
        rejects(() => small.setFontSize(65), 'oversized font accepted');
        rejects(() => small.setFontSize(NaN), 'NaN size accepted');
        rejects(() => small.setFontSize(Infinity), 'infinite size accepted');
        rejects(() => small.setFontSize('18'), 'string size accepted');
        rejects(() => small.setFontSize(18, 24), 'extra argument accepted');
        rejects(() => small.clearFontSize(18), 'clear argument accepted');
        return 'PASS SNI setFontSize/clearFontSize on lv.obj and lv.label';
    } finally {
        eos.activity.destroy(activity);
    }
})()
