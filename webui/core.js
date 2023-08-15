//定义枚举类
const XMsgObjectType = {
    //textMsg类
    TEXT: 0,
    MARKDOWN: 1,
    //fileMsg类
    FILE: 2,
    IMAGE: 3,
    AUDIO: 4,
    VIDEO: 5,
    //badMsg类
    BAD: 6,
};
const normal_base64en = [
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
    'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
    'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
    'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
    'w', 'x', 'y', 'z', '0', '1', '2', '3',
    '4', '5', '6', '7', '8', '9', '+', '/',
];
const urlsafe_base64en = [
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
    'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
    'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
    'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
    'w', 'x', 'y', 'z', '0', '1', '2', '3',
    '4', '5', '6', '7', '8', '9', '-', '_',
];
// 调整了+-/_对应值，这样可以同时支持urlsafe和normal自动识别处理
const base64de = [
    /* nul, soh, stx, etx, eot, enq, ack, bel, */
    255, 255, 255, 255, 255, 255, 255, 255,
    /*  bs,  ht,  nl,  vt,  np,  cr,  so,  si, */
    255, 255, 255, 255, 255, 255, 255, 255,
    /* dle, dc1, dc2, dc3, dc4, nak, syn, etb, */
    255, 255, 255, 255, 255, 255, 255, 255,
    /* can,  em, sub, esc,  fs,  gs,  rs,  us, */
    255, 255, 255, 255, 255, 255, 255, 255,
    /*  sp, '!', '"', '#', '$', '%', '&', ''', */
    255, 255, 255, 255, 255, 255, 255, 255,
    /* '(', ')', '*', '+', ',', '-', '.', '/', */
    255, 255, 255, 62, 255, 62, 255, 63,
    /* '0', '1', '2', '3', '4', '5', '6', '7', */
    52, 53, 54, 55, 56, 57, 58, 59,
    /* '8', '9', ':', ';', '<', '=', '>', '?', */
    60, 61, 255, 255, 255, 255, 255, 255,
    /* '@', 'A', 'B', 'C', 'D', 'E', 'F', 'G', */
    255, 0, 1, 2, 3, 4, 5, 6,
    /* 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', */
    7, 8, 9, 10, 11, 12, 13, 14,
    /* 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', */
    15, 16, 17, 18, 19, 20, 21, 22,
    /* 'X', 'Y', 'Z', '[', '\', ']', '^', '_', */
    23, 24, 25, 255, 255, 255, 255, 63,
    /* '`', 'a', 'b', 'c', 'd', 'e', 'f', 'g', */
    255, 26, 27, 28, 29, 30, 31, 32,
    /* 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', */
    33, 34, 35, 36, 37, 38, 39, 40,
    /* 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', */
    41, 42, 43, 44, 45, 46, 47, 48,
    /* 'x', 'y', 'z', '{', '|', '}', '~', del, */
    49, 50, 51, 255, 255, 255, 255, 255
];

function base64_encode(str, urlsafe) {
    if (typeof str == 'string') {
        const encoder = new TextEncoder();
        str = encoder.encode(str);
    }
    if (urlsafe === undefined) urlsafe = true;
    var s = 0;
    var i, j, inlen = str.length;
    var c, l = 0;
    var out = [];
    var base64en = urlsafe ? urlsafe_base64en : normal_base64en;
    for (i = j = 0; i < inlen; i++) {
        c = str[i];
        switch (s) {
            case 0:
                s = 1;
                out[j++] = base64en[(c >> 2) & 0x3F];
                break;
            case 1:
                s = 2;
                out[j++] = base64en[((l & 0x3) << 4) | ((c >> 4) & 0xF)];
                break;
            case 2:
                s = 0;
                out[j++] = base64en[((l & 0xF) << 2) | ((c >> 6) & 0x3)];
                out[j++] = base64en[c & 0x3F];
                break;
        }
        l = c;
    }
    switch (s) {
        case 1:
            out[j++] = base64en[(l & 0x3) << 4];
            out[j++] = '=';
            out[j++] = '=';
            break;
        case 2:
            out[j++] = base64en[(l & 0xF) << 2];
            out[j++] = '=';
            break;
    }
    return out.join('');
}

function base64_decode(str, asStr) {
    if (typeof str == 'string') {
        const encoder = new TextEncoder();
        str = encoder.encode(str);
    }
    if (asStr === undefined) asStr = true;
    var i, j, inlen = str.length;
    var c;
    var out = [];
    var eqchar = '='.charCodeAt(0);
    for (i = j = 0; i < inlen; i++) {
        c = str[i];
        if (c == eqchar) break;
        c = base64de[c];
        if (c == 255) return null;
        switch (i & 0x3) {
            case 0:
                out[j] = (c << 2) & 0xFF;
                break;
            case 1:
                out[j++] |= (c >> 4) & 0x3;
                out[j] = (c & 0xF) << 4;
                break;
            case 2:
                out[j++] |= (c >> 2) & 0xF;
                out[j] = (c & 0x3) << 6;
                break;
            case 3:
                out[j++] |= c;
                break;
        }
    }
    out.pop();//去掉最后一个0
    if (asStr) {
        const decoder = new TextDecoder();
        return decoder.decode(Uint8Array.from(out));
    }
    return out;
}
const small_hexen = ['0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'];
const big_hexen = ['0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'];
function hex_encode(str, bigChar) {
    if (typeof str == 'string') {
        const encoder = new TextEncoder();
        str = encoder.encode(str);
    }
    if (bigChar === undefined) bigChar = false;
    var out = [];
    var i, j, inlen = str.length;
    var c;
    var hexen = bigChar ? big_hexen : small_hexen;
    for (i = j = 0; i < inlen; i++) {
        c = str[i];
        out[j++] = hexen[c >> 4];
        out[j++] = hexen[c & 0xF];
    }
    return out.join('');
}

function hex_decode(str, asStr) {
    if (typeof str == 'string') {
        const encoder = new TextEncoder();
        str = encoder.encode(str);
    }
    if (asStr === undefined) asStr = true;
    var i, j, inlen = str.length;
    var c;
    var out = [];
    for (i = j = 0; i < inlen; i++) {
        c = str[i];
        if (c >= 48 && c <= 57) {//0-9
            c -= 48;
        } else if (c >= 65 && c <= 70) {//A-F
            c -= 55;
        } else if (c >= 97 && c <= 102) {//a-f
            c -= 87;
        } else {//非法字符
            continue;
        }
        if ((i & 0x1) == 0) {
            out[j] = c << 4;
        } else {
            out[j++] |= c;
        }
    }
    if (asStr) {
        const decoder = new TextDecoder();
        return decoder.decode(Uint8Array.from(out));
    }
    return out;
}
//获取当前URL中token=的值（解析参数）
var bk_token = null
function getToken() {
    if (bk_token != null)
        return bk_token;
    var url = window.location.href;
    //#和?的后方才是
    var token = url.split('#')[1];
    if (token == undefined) {
        token = url.split('?')[1];
    }
    if (token == undefined) {
        token = url;
    }
    var arr = token.split('&');
    for (var i = 0; i < arr.length; i++) {
        var item = arr[i];
        if (item.indexOf('token=') == 0) {
            var s = item.substring(6);
            if (s != '') {
                bk_token = s;
                return s;
            }
            break;
        }
    }
    //判断是不是index.html
    if (window.location.href.indexOf('index.html') == -1)
        window.location.href = './index.html';
    return '';
}
function defaultCallback(...args) { };

var canTrust = false;
{
    var xhr = new XMLHttpRequest();
    xhr.open("POST", "./api.cgi?", true);
    xhr.onreadystatechange = function () {
        if (xhr.readyState == 4 && xhr.status == 200) {
            if (xhr.responseText == "true")
                canTrust = true;
        }
    }
    xhr.send("api=canTrust");
}

function DG_canTrust(ownerID) {
    if (ownerID == undefined || ownerID == null) return canTrust;
    if (canTrust)
        return true;
    else {
        if (localStorage.getItem("#DG_db#token2ownerID#" + getToken()) == ownerID)
            return true;
        else
            return false;
    }
}
function DG_login(acc, pwd, callback) {
    if (callback == undefined || callback == null)
        callback = defaultCallback;
    if (acc == undefined || pwd == undefined || acc == "" || pwd == "") {
        callback(false, "账号或密码为空");
        return;
    }
    //acc、pwd不得大于1024字节
    if (acc.length > 1024 || pwd.length > 1024) {
        callback(false, "账号或密码过长(超过1024字节)");
        return;
    }
    var xhr = new XMLHttpRequest();
    xhr.open("POST", "./api.cgi?", true);
    xhr.onreadystatechange = function () {
        if (xhr.readyState == 4) {
            if (xhr.status == 200) {
                var res = JSON.parse(xhr.responseText);
                if (res.status != 0) {
                    callback(true, res.token, res.ownerID);
                } else {
                    callback(false, "账号或密码错误");
                }
            } else {
                callback(false, "网络错误");
            }
        }
    }
    xhr.send("api=login&acc=" + encodeURIComponent(acc) + "&pwd=" + encodeURIComponent(pwd));
}

function DG_logout(callback) {
    if (callback == undefined || callback == null)
        callback = defaultCallback;
    var token = getToken();
    if (token == '' || token == null || token == undefined) {
        callback(false, "未登录");
        return;
    }
    var xhr = new XMLHttpRequest();
    xhr.open("POST", "./api.cgi?", true);
    xhr.onreadystatechange = function () {
        if (xhr.readyState == 4) {
            if (xhr.status == 200) {
                var res = JSON.parse(xhr.responseText);
                if (res.status != 0) {
                    localStorage.removeItem("#DG_db#token2ownerID#" + token);
                    callback(true);
                } else {
                    callback(false, "未知错误");
                }
            } else {
                callback(false, "网络错误");
            }
        }
    }
    xhr.send("api=logout&token=" + token);
}

function DG_register(acc, pwd, callback) {
    if (callback == undefined || callback == null)
        callback = defaultCallback;
    if (acc == undefined || pwd == undefined || acc == "" || pwd == "") {
        callback(false, "账号或密码为空");
        return;
    }
    if (acc == "未知") {
        callback(false, "账号不得为字符串'未知'");
        return;
    }
    if (acc == "*") {
        callback(false, "账号不得为字符串'*'");
        return;
    }
    //acc、pwd不得大于1024字节
    if (acc.length > 1024 || pwd.length > 1024) {
        callback(false, "账号或密码过长(超过1024字节)");
        return;
    }
    var xhr = new XMLHttpRequest();
    xhr.open("POST", "./api.cgi?", true);
    xhr.onreadystatechange = function () {
        if (xhr.readyState == 4) {
            if (xhr.status == 200) {
                var res = JSON.parse(xhr.responseText);
                if (res.status != 0) {
                    callback(true, res.token);
                } else {
                    callback(false, "账号已存在");
                }
            } else {
                callback(false, "网络错误");
            }
        }
    }
    xhr.send("api=register&acc=" + encodeURIComponent(acc) + "&pwd=" + encodeURIComponent(pwd));
}

function DG_getStudentID(callback) {
    if (callback == undefined || callback == null)
        callback = defaultCallback;
    var xhr = new XMLHttpRequest();
    xhr.open("GET", "./api.cgi?api=getStudentID", true);
    xhr.onreadystatechange = function () {
        if (xhr.readyState == 4) {
            if (xhr.status == 200) {
                callback(true, xhr.responseText);
            } else {
                callback(false, "网络错误");
            }
        }
    }
    xhr.send();
}

function DG_infosafeusers(callback) {
    if (callback == undefined || callback == null)
        callback = defaultCallback;
    var xhr = new XMLHttpRequest();
    xhr.open("POST", "./api.cgi?", true);
    xhr.onreadystatechange = function () {
        if (xhr.readyState == 4) {
            if (xhr.status == 200) {
                var res = JSON.parse(xhr.responseText);
                callback(true, res);
            } else {
                callback(false, "网络错误");
            }
        }
    }
    xhr.send("api=infosafeusers&token=" + getToken());
}

/*
    说明：
        请求：/api.cgi?api=data&token=XXX&time_filter=XXX&type_filter=XXX&owner_filter=XXX&sort=XXX&search=XXX
        返回：XMsgObjectsResult的json输出结果
        意义：用于显示数据
        额外说明：
            sort参数： asc 或 desc （默认asc升序）
            time_filter参数： HEX(start)-HEX(end) 或 *-HEX(end) 或 HEX(start)-* 或 *-*（默认*-*）
            type_filter参数： XXX 或 *（默认*）另外，允许XXX_XXX这样的多选类型(text/markdown/file/image/audio/video)
            owner_filter参数： HEX 或 *（默认*）
            search参数： msgName搜索关键字(标题)，可能也是完整匹配msgId，同时收集判断（不存在那就是不搜索全部展示）
            排序规则：时间
 */
function DG_data(time_filter, type_filter, owner_filter, sort, search, callback) {
    if (callback == undefined || callback == null)
        callback = defaultCallback;
    if (time_filter == undefined || time_filter == null)
        time_filter = "*-*";
    if (type_filter == undefined || type_filter == null)
        type_filter = "*";
    if (owner_filter == undefined || owner_filter == null)
        owner_filter = "*";
    if (search == undefined || search == null)
        search = "";
    if (sort == undefined || sort == null)
        sort = "asc";
    else {
        sort = sort.toLowerCase();
        if (sort != "asc" && sort != "desc")
            sort = "asc";
    }
    var xhr = new XMLHttpRequest();
    xhr.open("POST", "./api.cgi?", true);
    xhr.onreadystatechange = function () {
        if (xhr.readyState == 4) {
            if (xhr.status == 200) {
                var res = JSON.parse(xhr.responseText);
                DG_infosafeusers(function (success, Raw_userinfos) {
                    if (success) {
                        var userinfos = {};
                        for (var i = 0; i < Raw_userinfos.length; i++) {
                            var userinfo = Raw_userinfos[i];
                            userinfos[userinfo.ownerID] = userinfo;
                        }
                        //补齐数据
                        for (var i = 0; i < res.length; i++) {
                            var item = res[i];
                            var owner = item.ownerID;
                            item.owner = "未知";
                            if (owner != undefined && userinfos[owner] != undefined) {
                                item.owner = userinfos[owner].account || "未知";
                            }
                            var type = item.type;
                            item.type = "未知";
                            switch (type) {
                                case XMsgObjectType.TEXT:
                                    item.type = "文本";
                                    item.size = item.textMsg.length;
                                    break;
                                case XMsgObjectType.MARKDOWN:
                                    item.type = "Markdown";
                                    item.size = item.textMsg.length;
                                    break;
                                case XMsgObjectType.FILE:
                                    item.type = "文件";
                                    item.size = item.fileMsg.size;
                                    break;
                                case XMsgObjectType.IMAGE:
                                    item.type = "图片";
                                    item.size = item.fileMsg.size;
                                    break;
                                case XMsgObjectType.VIDEO:
                                    item.type = "视频";
                                    item.size = item.fileMsg.size;
                                    break;
                                case XMsgObjectType.AUDIO:
                                    item.type = "音频";
                                    item.size = item.fileMsg.size;
                                    break;
                                case XMsgObjectType.BAD:
                                default:
                                    item.type = "未知";
                                    item.size = 0;
                                    break;
                            }
                            // //截取前27个字符
                            // if (item.msgName.length > 30)
                            //     item.msgName = item.msgName.substr(0, 27) + "...";
                            item.timeStr = item.time[0] + "-" + (item.time[1] < 10 ? "0" : "") + item.time[1] + "-" + (item.time[2] < 10 ? "0" : "") + item.time[2] + " " + (item.time[3] < 10 ? "0" : "") + item.time[3] + ":" + (item.time[4] < 10 ? "0" : "") + item.time[4] + ":" + (item.time[5] < 10 ? "0" : "") + item.time[5];
                        }
                        callback(true, res);
                        // //res内容翻倍（测试用）
                        // var res2 = [];
                        // for (var i = 0; i < res.length; i++) {
                        //     res2.push(res[i]);
                        //     res2.push(res[i]);
                        //     res2.push(res[i]);
                        //     res2.push(res[i]);
                        //     res2.push(res[i]);
                        //     res2.push(res[i]);
                        // }
                        // callback(true, res2);
                    } else {
                        callback(false, "用户基本数据无法获取");
                    }
                });
            } else {
                callback(false, "网络错误");
            }
        }
    }
    if (search == "") {
        xhr.send("api=data&time_filter=" + encodeURIComponent(time_filter) +
            "&type_filter=" + (type_filter) +
            "&owner_filter=" + encodeURIComponent(owner_filter) +
            "&sort=" + (sort) +
            "&token=" + getToken());
    } else {
        xhr.send("api=data&time_filter=" + encodeURIComponent(time_filter) +
            "&type_filter=" + (type_filter) +
            "&owner_filter=" + encodeURIComponent(owner_filter) +
            "&sort=" + (sort) +
            "&search=" + encodeURIComponent(search) +
            "&token=" + getToken());
    }
}


function DG_send(data, type, callback, progressCallback) {
    //type参数： text/markdown/file/image/audio/video
    if (data === undefined) return;
    if (type != "text" && type != "markdown" && type != "file" && type != "image" && type != "audio" && type != "video") return;
    if (callback == undefined || callback == null) callback = defaultCallback;
    if (progressCallback == undefined || progressCallback == null) progressCallback = defaultCallback;
    var xhr = new XMLHttpRequest();
    xhr.open("POST", "./api.cgi?", true);
    xhr.onreadystatechange = function () {
        if (xhr.readyState == 4) {
            if (xhr.status == 200) {
                var res = JSON.parse(xhr.responseText);
                callback(true, res);
            } else {
                callback(false, "网络错误");
            }
        }
    }
    function __onprogress(e) {
        if (e.lengthComputable) {
            var per = e.loaded / e.total;
            per = Math.floor(per * 10000) / 100;
            progressCallback(per, e.loaded, e.total, e);
        }
    }
    xhr.upload.onprogress = __onprogress;
    xhr.onprogress = __onprogress;
    switch (type) {
        case "text":
        case "markdown": {
            var encodebuff = base64_encode(data.content, true);
            xhr.setRequestHeader("api", "send");
            xhr.setRequestHeader("token", getToken());
            xhr.setRequestHeader("type", type);
            xhr.setRequestHeader("msgName", encodeURIComponent(data.msgName));
            xhr.setRequestHeader("data", encodebuff);
            xhr.send();
            // xhr.send("api=send&token=" + getToken() + "&type=" + type + "&msgName=" + encodeURIComponent(data.msgName)
            //     + "&data=" + encodebuff);
            break;
        }
        case "file":
        case "image":
        case "audio":
        case "video": {
            var fr = new FileReader();
            fr.onload = function () {
                var encodebuff = base64_encode(new Uint8Array(fr.result), true);
                xhr.setRequestHeader("api", "send");
                xhr.setRequestHeader("token", getToken());
                xhr.setRequestHeader("type", type);
                xhr.setRequestHeader("msgName", encodeURIComponent(data.msgName));
                xhr.setRequestHeader("fileName", encodeURIComponent(data.name));
                xhr.setRequestHeader("data", encodebuff);
                xhr.send();
                // xhr.send("api=send&token=" + getToken() + "&type=" + type + "&msgName=" + encodeURIComponent(data.msgName) + "&fileName=" + encodeURIComponent(data.name)
                //     + "&data=" + encodebuff);
            }
            fr.readAsArrayBuffer(data.file);
            break;
        }
    }
}
/*
    文件类容易有体积过大出问题，只能切片发送（文本类能超出然后有问题那也算神了，那么多内容干嘛不发文件呢？）
    大文件只能分割发送（如果想支持的话，目前现在小体积项目就能用了，大文件可以以后再说或者不支持）
    切片大致思路：
    先申请下来一个文件消息和对应文件句柄（通知我一共多少片，每片多大，总大小等信息）
    然后发起切片写入请求，告诉对哪个切片写入，写入多少，写入的内容是什么
    归还文件句柄，申请最终构建信息列表项目，加进列表中，完成上传
    具体过程：
    filesplitsend->得到合并令牌（临时目录中创建文件，合并现在这里写）
    filesplitwrite->携带令牌写入切片
    filesplitmerge->携带令牌通知完成合并，生成消息记录（临时文件移动到正式目录，合并完成）
    如果失败，临时文件由系统自动清理就不管了
*/
function DG_filesplitsend(data, type, callback, progressCallback) {
    //type参数： text/markdown/file/image/audio/video
    if (data === undefined) return;
    if (type != "text" && type != "markdown" && type != "file" && type != "image" && type != "audio" && type != "video") return;
    if (callback == undefined || callback == null) callback = defaultCallback;
    if (progressCallback == undefined || progressCallback == null) progressCallback = defaultCallback;
    var Fst_xhr = new XMLHttpRequest();
    Fst_xhr.open("POST", "./api.cgi?", true);
    switch (type) {
        case "file":
        case "image":
        case "audio":
        case "video": {
            var fr = new FileReader();
            fr.onload = function () {
                const uint8arr = new Uint8Array(fr.result);
                const splitSize = 1024 * 1024 * 5;//5M切片
                const splitCount = Math.ceil(uint8arr.length / splitSize);
                var splitList = [];
                var finishsize = 0;
                for (var i = 0; i < splitCount; i++) {
                    const start = i * splitSize;
                    var end = (i + 1) * splitSize;
                    if (end > uint8arr.length) end = uint8arr.length;
                    const splitData = uint8arr.slice(start, end);
                    splitList.push(splitData);
                }
                Fst_xhr.onreadystatechange = function () {
                    if (Fst_xhr.readyState == 4) {
                        if (Fst_xhr.status == 200) {
                            var res = JSON.parse(Fst_xhr.responseText);
                            const mergeId = res.mergeId;
                            var OK = true;
                            var count = 0;
                            var per = finishsize / uint8arr.length;
                            function sendSplit() {
                                if (splitList.length > 0 && OK) {
                                    count++;
                                    const splitData = splitList.shift();
                                    const dataLen = splitData.length;
                                    var splitXhr = new XMLHttpRequest();
                                    splitXhr.open("POST", "./api.cgi?", true);
                                    splitXhr.onreadystatechange = function () {
                                        if (splitXhr.readyState == 4) {
                                            if (splitXhr.status != 200) {
                                                OK = false;
                                            } else {
                                                finishsize += dataLen;
                                                per = finishsize / uint8arr.length;
                                                per = Math.floor(per * 10000) / 100;
                                                progressCallback(count, splitCount, per, finishsize, uint8arr.length);
                                                sendSplit();
                                            }
                                        }
                                    }
                                    function __onprogress(e) {
                                        if (e.lengthComputable) {
                                            var t_finishsize = finishsize + e.loaded;
                                            per = t_finishsize / uint8arr.length;
                                            per = Math.floor(t_per * 10000) / 100;
                                            progressCallback(count, splitCount, per, t_finishsize, uint8arr.length);
                                        }
                                    }
                                    //见鬼，进度条不动弹
                                    splitXhr.upload.onprogress = __onprogress;
                                    splitXhr.onprogress = __onprogress;
                                    var encodebuff = base64_encode(splitData, true);
                                    splitXhr.setRequestHeader("api", "filesplitwrite");
                                    splitXhr.setRequestHeader("mergeId", mergeId);
                                    splitXhr.setRequestHeader("dataLen", "" + dataLen);
                                    splitXhr.setRequestHeader("data", encodebuff);//仅有的稳定提交方式
                                    splitXhr.send();
                                    // splitXhr.send("api=filesplitwrite&mergeId=" + mergeId + "&dataLen=" + dataLen + "&data=" + encodebuff);
                                } else {
                                    if (!OK) {
                                        callback(false, "网络错误（切片发送时）");
                                        return;
                                    }
                                    var mergeXhr = new XMLHttpRequest();
                                    mergeXhr.open("POST", "./api.cgi?", true);
                                    mergeXhr.onreadystatechange = function () {
                                        if (mergeXhr.readyState == 4) {
                                            if (mergeXhr.status == 200) {
                                                var res = JSON.parse(mergeXhr.responseText);
                                                callback(true, res);
                                            } else {
                                                callback(false, "网络错误（合并时）");
                                            }
                                        }
                                    }
                                    mergeXhr.send("api=filesplitmerge&mergeId=" + mergeId);
                                }
                            }
                            sendSplit();

                        } else {
                            callback(false, "网络错误");
                        }
                    }
                }
                Fst_xhr.send("api=filesplitsend&token=" + getToken() + "&type=" + type + "&msgName=" + encodeURIComponent(data.msgName) + "&fileName=" + encodeURIComponent(data.name)
                    + "&splitCount=" + splitCount + "&splitSize=" + splitSize + "&fileSize=" + uint8arr.length);
            }
            fr.readAsArrayBuffer(data.file);
            break;
        }
        default: {
            callback(false, "不支持的类型");
            break;
        }
    }
}

function DG_shareDownloadName(msgId, callback) {
    if (callback == undefined || callback == null) callback = defaultCallback;
    var xhr = new XMLHttpRequest();
    xhr.open("GET", "./api.cgi?api=shareDownloadName&msgId=" + msgId, true);
    xhr.onreadystatechange = function () {
        if (xhr.readyState == 4) {
            if (xhr.status == 200) {
                var res = JSON.parse(xhr.responseText);
                callback(true, res);
            } else {
                callback(false, "网络错误");
            }
        }
    }
    xhr.send();
}

function DG_download(msgId) {
    if (msgId == undefined || msgId == null) return;
    var name = msgId;
    DG_shareDownloadName(msgId, function (status, res) {
        if (status) name = res.name;
        var url = "./api.cgi?api=download&msgId=" + msgId;
        var a = document.createElement("a");
        name = name.replace(/[\|\/\<\>\:\*\?\\\"]/g, "_");
        a.download = name;
        a.href = url;
        a.click();
        // var xhr = new XMLHttpRequest();
        // xhr.open("GET", "./api.cgi?api=download&msgId=" + msgId, true);
        // xhr.responseType = "blob";
        // xhr.onreadystatechange = function () {
        //     if (xhr.readyState == 4) {
        //         if (xhr.status == 200) {
        //             var blob = xhr.response;
        //             var a = document.createElement("a");
        //             a.href = window.URL.createObjectURL(blob);
        //             //name过滤掉不能在文件名上使用的字符
        //             name = name.replace(/[\|\/\<\>\:\*\?\\\"]/g, "_");
        //             a.download = name;
        //             a.click();
        //         } else {
        //             layer.msg("下载失败");
        //         }
        //     }
        // }
        // xhr.send();
    });
}
function DG_getbuffer(msgId, callback) {
    if (msgId == undefined || msgId == null) return;
    if (callback == undefined || callback == null) callback = defaultCallback;
    var xhr = new XMLHttpRequest();
    xhr.open("GET", "./api.cgi?api=download&msgId=" + msgId, true);
    xhr.responseType = "blob";
    xhr.onreadystatechange = function () {
        if (xhr.readyState == 4) {
            if (xhr.status == 200) {
                var blob = xhr.response;
                callback(true, blob);
            } else {
                callback(false, "下载失败");
            }
        }
    }
    xhr.send();
}
function DG_withdraw(msgId, callback) {
    if (msgId == undefined || msgId == null) return;
    if (callback == undefined || callback == null) callback = defaultCallback;
    var xhr = new XMLHttpRequest();
    xhr.open("POST", "./api.cgi?", true);
    xhr.onreadystatechange = function () {
        if (xhr.readyState == 4) {
            if (xhr.status == 200) {
                var res = JSON.parse(xhr.responseText);
                if (res.status == 1) {
                    callback(true, "撤除成功");
                } else {
                    callback(false, "撤除失败");
                }
            } else {
                callback(false, "网络错误");
            }
        }
    }
    xhr.send("api=withdraw&msgId=" + msgId + "&token=" + getToken());
}

function DG_delete(msgId, callback) {
    if (msgId == undefined || msgId == null) return;
    if (callback == undefined || callback == null) callback = defaultCallback;
    var xhr = new XMLHttpRequest();
    xhr.open("POST", "./api.cgi?", true);
    xhr.onreadystatechange = function () {
        if (xhr.readyState == 4) {
            if (xhr.status == 200) {
                var res = JSON.parse(xhr.responseText);
                if (res.status == 1) {
                    callback(true, "删除成功");
                } else {
                    callback(false, "删除失败");
                }
            } else {
                callback(false, "网络错误");
            }
        }
    }
    xhr.send("api=delete&msgId=" + msgId + "&token=" + getToken());
}

/*
    说明：
        请求：/api.cgi?api=getMsgById&msgId=XXX
        返回：XXXX
        意义：用于获取单消息信息
 */
function DG_getMsgById(msgId, callback) {
    if (msgId == undefined || msgId == null) return;
    if (callback == undefined || callback == null) callback = defaultCallback;
    var xhr = new XMLHttpRequest();
    xhr.open("GET", "./api.cgi?api=getMsgById&msgId=" + msgId, true);
    xhr.onreadystatechange = function () {
        if (xhr.readyState == 4) {
            if (xhr.status == 200) {
                var res = JSON.parse(xhr.responseText);
                callback(true, res[0]);
            } else {
                callback(false, "网络错误");
            }
        }
    }
    xhr.send();
}