var sensemote = (function () {
    "use strict";
    // private
    var STREAM_TIMEOUT = 10000,  // ms before timing out /stream request
        RETRY_DELAY = 500,      // ms delay between /stream requests

        ids = {},
        data_cb = null,
        error_cb = null,
        pollxhr = null,
        pollxhrRepeat = false,
        timedout = false,
        streamtimer,
        pollTimer,

        isdefined = function (o) { return typeof o !== 'undefined'; },

        stop = function () {
            pollxhrRepeat = false;
            if (pollxhr !== null)
                pollxhr.abort();
        },

        reset = function () {
            stop();
            ids = {};
        },

        pollLoop = function () {
            var i, lines, pair, topic, obj;
            pollxhr = new XMLHttpRequest();
            pollxhr.onreadystatechange = function () {
                if (pollxhr.readyState === 4) {
                    if (pollxhr.status === 200) {
                        clearTimeout(streamtimer);
                        if (null !== data_cb) {
                            lines = pollxhr.responseText.split(/\r\n/);
                            for (i = 0; i < lines.length; i++) {
                                try {
                                    obj = JSON.parse(lines[i]);
                                    for (pair in obj)
                                        for (topic in obj[pair])
                                            if (null != data_cb)
                                                data_cb(topic, obj[pair][topic]);
                                } catch(e) {
                                    reset();
                                    if (null != error_cb)
                                        error_cb('bad json');
                                }
                            }
                        }
                        if (pollxhrRepeat)
                            pollTimer = setTimeout(function (){pollLoop();}, RETRY_DELAY);
                    }
                    else {
                        if (!timedout && pollxhrRepeat) {
                            reset();
                            if (null != error_cb)
                                error_cb(pollxhr.statusText, pollxhr.responseText);
                        }
                    }
                }
            };

            streamtimer = setTimeout(function () {
                timedout = true;
                pollxhr.abort();
                clearTimeout(pollTimer);
                timedout = false;
                if (pollxhrRepeat)
                    setTimeout(function (){pollLoop();}, RETRY_DELAY);
            }, STREAM_TIMEOUT);

            pollxhr.open('POST', '/stream', true);
            pollxhr.send(JSON.stringify(ids));
        },

        start = function () {
            pollxhrRepeat = true;
            pollLoop();
        };

    reset();

    // public
    return {
        stream : function () {
            stop();
            start();
        },

        subscribe : function (topic, cbs) {
            var req = new XMLHttpRequest();
            req.onreadystatechange = function () {
                if (req.readyState == 4) {
                    if (req.status == 200) {
                        var o = JSON.parse(req.responseText);
                        ids[o.id] = o.seckey;
                        if (isdefined(cbs) && isdefined(cbs.success))
                            cbs.success();
                    }
                    else {
                        if (isdefined(cbs) && isdefined(cbs.error))
                            cbs.error(req.statusText, req.responseText);
                        reset();
                        if (null != error_cb)
                            error_cb();
                    }
                }
            };
            req.open('POST', '/subscribe', true);
            req.send(JSON.stringify([topic]));
        },

        publish : function (topic, msg, cbs) {
            var o = {};
            if (typeof msg === 'string')
                o[topic] = msg;
            else
                o[topic] = JSON.stringify(msg);
            var req = new XMLHttpRequest();
            req.onreadystatechange = function () {
                if (req.readyState == 4) {
                    if (req.status == 200) {
                        if (isdefined(cbs) && isdefined(cbs.success))
                            cbs.success(topic, msg);
                    }
                    else {
                        if (isdefined(cbs) && isdefined(cbs.error))
                            cbs.error(textStatus, errorThrown);
                        reset();
                        if (null != error_cb)
                            error_cb();
                    }
                }
            };
            req.open('POST', '/publish', true);
            req.send(JSON.stringify(o));
        },

        register : function (cbs) {
            if (isdefined(cbs) && isdefined(cbs.data))
                data_cb = cbs.data;
            if (isdefined(cbs) && isdefined(cbs.error))
                error_cb = cbs.error;
        }
    }
})();  // end sensemote


//////////////////////////////////////

/*
// sensemote.publish('sensemote/test', 'wibble', {success:function () { window.console.log('ok');}})
// sensemote.publish('sensemote/test', {'bob':'bob'}, {success:function () { window.console.log('ok');}})

var timer;
function reconnect()
{
    clearTimeout(timer);
    timer = setTimeout(function (){startStreaming();}, 1000);
}

function startStreaming()
{
    sensemote.register({
        data: function (t, m) {window.console.log('rx '+t+' '+m);},
        error: function () {reconnect();},
    });
    sensemote.subscribe('sensemote/apidemo/#', {
        success: function () {
            sensemote.subscribe('sensemote/monkey/#', {success: sensemote.stream});
        }
    });
}

setInterval(function (){sensemote.publish('sensemote/apidemo/foo', 'bar');}, 10000);
startStreaming();
*/
