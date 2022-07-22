//聊天室主人
var userid
// 消息接收者
var toUserid
var username;
var ws;
const msg_type =
{
    msg_type_unknown: 0,
    msg_type_heartbeat: 1000, //心跳包

    msg_type_register: 1001, //注册
    msg_type_login: 1002,    //登陆

    msg_type_getofriendlist: 1003,        //获取好友列表
    msg_type_finduser: 1004,              //查找用户
    msg_type_operatefriend: 1005,         //添加、删除等好友操作
    msg_type_userstatuschange: 1006,      //用户信息改变通知
    msg_type_updateuserinfo: 1007,        //更新用户信息
    msg_type_modifypassword: 1008,        //修改登陆密码
    msg_type_creategroup: 1009,           //创建群组
    msg_type_getgroupmembers: 1010,       //获取群组成员列表
    msg_type_chat: 1100,           //单聊消息
    msg_type_multichat: 1101,             //群发消息
    msg_type_kickuser: 1102,              //被踢下线
    msg_type_remotedesktop: 1103,         //远程桌面
    msg_type_updateteaminfo: 1104,        //更新用户好友分组信息
    msg_type_modifyfriendmarkname: 1105,  //更新好友备注信息
    msg_type_movefriendtootherteam: 1106, //移动好友至
};

//登录后显示用户名和状态
$(function () {
    $.ajax({
        //是否异步,此项目此处必须是false
        async: false,
        //请求方式
        type: 'GET',
        //请求url
        url: "/getUsername",
        success: function (res) {
            username = res;
            $('#chatMeu').html('<p>用户：' + res + "<span style='float: right;color: greenyellow; height: 20px'>在线</span></p>")
        }
    });
    //创建websocket对象
    ws = new WebSocket("ws://120.24.61.9:9999/chat");

    //建立连接后触发
    ws.onopen = function () {
        //显示登录的用户名
        $('#chatMeu').html('<p>用户：' + username + "<span style='float: right;color: greenyellow; height: 20px'>在线</span></p>");

        //请求好友列表
        requsetFriendList();
    };

    //接收到服务端的推送后触发
    ws.onmessage = function (evt) {
        //获取数据
        var dataStr = evt.data;
        console.log(dataStr);
        var jsonData = JSON.parse(dataStr);
        var cmd = jsonData[0]["cmd"];
        switch (cmd) {
            case msg_type.msg_type_getofriendlist:
                requsetFriendList_respondDeal(jsonData);
                break;
            case msg_type.msg_type_chat:
                getChatMsg_respondDeal(jsonData);
                break;
            case msg_type.msg_type_finduser:
                findFriend_respondDeal(jsonData);
                break;
            case msg_type.msg_type_operatefriend:
                operateFriend_respondDeal(jsonData);
                break;
            default:
                break;
        }
    };
    //关闭连接触发
    ws.onclose = function () {
        alert("和服务器断开连接, onclose 调用");

        $('#chatMeu').html('<p>用户' + username + "<span style='float: right;color: #d50a0a; height: 20px'>离线</span></p>");
        location.href = '/';
    };

    //发送按钮点击
    $("#submit").click(function () {
        msgSend(toUserid);
    });

    //监听消息输入框的回车事件暂时不用
    $("#tex_content").keydown(function (e) {
        //必须是消息输入框的回车事件才行
        if (e.keyCode == 13) {
            $("#submit").click();
            e.preventDefault();
        }
    })


    // $(document).keydown(function (e) {
    //     // console.log("按下键");
    //     // console.log(e.target.isSameNode($("#tex_content")));
    //     // console.log(e.target.nodeType);
    //     // console.log($("#tex_content")[0].nodeType);

    //     //如果不是在输入消息框按下回车键则发送消息
    //     // console.log(e.target == $("#tex_content")[0]);
    //     if ((e.keyCode == 13) && (e.target != $("#tex_content")[0])) {
    //         // console.log("按下回车键");
    //         $("#submit").click();
    //     }
    // })



    //findFriend_text 获得焦点
    $("#findFriend_text").focus(function () {
        if (this.value === "请输入名称") {
            this.value = "";
            this.style.color = "black";
        }
        this.className = "findFriend_text onfocus";
    });

    //findFriend_text 失去焦点
    $("#findFriend_text").blur(function () {
        if (this.value === "") {
            this.value = "请输入名称"
            this.style.color = "rgb(152, 158, 158)";
        }
        this.className = "findFriend_text onblur"; //多类名
    });

    //查找好友点击事件
    $("#findFriend_btn").click(function () {
        var friendName = $("#findFriend_text").val();
        if (friendName == "" || friendName == "请输入名称")
            return alert("请输入好友名称");
        var sendJson = [{ "cmd": msg_type.msg_type_finduser, "seq": 0 }, { "type": 1, "username": friendName }];
        ws.send(JSON.stringify(sendJson));
    });
});


//点击好友列表后，执行的动作
function chatWith(userid) {
    // console.log("点击好友列表 userid = " + userid);
    toUserid = userid;
    var userInfo = sessionStorage.getItem(toUserid + "_userInfo");
    // console.log(" userInfo = " + userInfo);
    //得到好友的名字
    var jsonroot = JSON.parse(userInfo);
    var name = jsonroot["username"];

    //再次点击好友聊天，需要删除原来的”和xxx"聊天的提示
    $('#p1').remove();
    $('#chatMeu').append('<p id="p1" style="text-align: center">正在和<b style="color: #db41ca ">' + name + '</b>聊天</p>');
    $('#chatMain').css("display", "inline");
    //清空聊天区
    $("#chatCnt").html("");

    //设置未读消息显示
    var id_div = userid + "_unreadDispaly";
    $("#" + id_div).text("");
    $("#" + id_div)[0].className = "";

    //当点击聊天人列表时，需要获与之对应的聊天记录，聊天记录存放在sessionStorage中
    var chatData = sessionStorage.getItem(toUserid + "_chatMsg");
    if (chatData != null) {
        //渲染聊天数据到聊天区
        $("#chatCnt").html(chatData);
        showMsg();
    }
}
function showMsg() {
    //这一条代码实现了，每次显示最新的消息
    $("#chatCnt")[0].scrollTop = $("#chatCnt")[0].scrollHeight;
}

function msgSend(toUserid) {
    // console.log("触发按钮发送点击事件");
    if (toUserid == null) {
        alert("请选择聊天好友");
        return;
    }

    //获取发送输入框中的内容
    var data = $("#tex_content").val();
    //点击发送后，清空输入内容框架
    $("#tex_content").val("");

    // var sendJson = { "toName": toName, "message": data };
    // var userInfo = sessionStorage.getItem(toUserid + "_userInfo");
    // var jsonRoot = JSON.parse(userInfo);
    // var targetid = jsonRoot["userid"];
    //toUserid - 0 强制类型转换成 Number; 或者Number(toUserid) 或 toUserid。ToNumber();
    var sendJson = [{ "cmd": msg_type.msg_type_chat, "seq": 0, "targetid": toUserid - 0 }, { "data": data }];


    //聊天框显示发送内容
    var cnt = "<div  class=\"btalk\"><span id=\"bsay\">" + data + "</span></div>";
    $("#chatCnt").append(cnt);

    // console.log($("#chatCnt"));
    showMsg();

    // console.log($("#chatCnt")[0].scrollTop);
    // console.log($("#chatCnt")[0].scrollHeight);


    // showview.scrollTop=showview.scrollHeigh

    var chatData = sessionStorage.getItem(toUserid + "_chatMsg");
    if (chatData != null) {
        cnt = chatData + cnt;
    }
    sessionStorage.setItem(toUserid + "_chatMsg", cnt);

    console.log("发送消息的内容 = " + JSON.stringify(sendJson));
    //发送数据给服务端
    ws.send(JSON.stringify(sendJson));

}

function addFriend(friendUseid) {
    // console.log(typeof ws);
    // console.log(ws);

    var sendJson = [{ "cmd": msg_type.msg_type_operatefriend, "seq": 0 }, { "userid": friendUseid, "type": 1 }];

    console.log(JSON.stringify(sendJson));
    ws.send(JSON.stringify(sendJson));

}

function agreeAddFriend(friendUseid, friendName) {
    // console.log(typeof ws);
    // console.log(ws);

    var sendJson = [{ "cmd": msg_type.msg_type_operatefriend, "seq": 0 }, { "userid": friendUseid, "username": friendName, "type": 3, "accept": 1 }];

    // console.log(JSON.stringify(sendJson));
    ws.send(JSON.stringify(sendJson));

}

function requsetFriendList() {
    //请求好友列表
    var sendJson = [{ "cmd": 1003, "seq": 0, "username": username }];

    ws.send(JSON.stringify(sendJson));
}
function requsetFriendList_respondDeal(jsonData) {
    var friendsList = "";
    for (var userinfo of jsonData[1]["userinfo"]) {
        var teamname = userinfo["teamname"];
        friendsList += "<li><span class = 'group' >" + teamname + "</span></li>";
        for (var member of userinfo["members"]) {
            name = member["username"];
            userid = member["userid"];
            friendsList += "<li><span class = 'friend' onclick='chatWith(\"" + userid + "\")'>" + name + "</span> <div id='" + userid + "_unreadDispaly'> </div></li>";
            //把所有好友的信息存储到session中

            sessionStorage.setItem(userid + "_userInfo", JSON.stringify(member));
            // var m_userInfo = sessionStorage.getItem(name + "_userInfo");
            // console.log(m_userInfo);
        }
    }
    //     //渲染页面
    $("#friendsList").html(friendsList);

}

function getChatMsg_respondDeal(jsonData) {
    var data = jsonData[1]["data"];
    var cnt = "<div class=\"atalk\"><span id=\"asay\">" + data + "</span></div>";

    var senderid = jsonData[0]["senderid"];
    console.log("接收到好友发送来的消息 = " + data);
    //正在和接收到消息的用户通信
    if (toUserid == senderid) {
        $("#chatCnt").append(cnt);

        showMsg();

        console.log("toUserid == senderid 相等");
    }
    else {
        //设置未读消息显示
        var id_div = senderid + "_unreadDispaly";
        var count = 0;
        if ($("#" + id_div).text() != "") {
            count = $("#" + id_div).text() - 0;
        }
        $("#" + id_div).text(++count);
        $("#" + id_div)[0].className = "unreadDisplay";
    }



    var chatData = sessionStorage.getItem(senderid + "_chatMsg");
    if (chatData != null) {
        cnt = chatData + cnt;
    }
    sessionStorage.setItem(senderid + "_chatMsg", cnt);
}

function findFriend_respondDeal(jsonData) {
    var showHtml;
    if (jsonData[1]["userinfo"] != "") {

        var friendName = jsonData[1]["userinfo"][0]["username"];
        var friendUserid = jsonData[1]["userinfo"][0]["userid"];
        showHtml = "<li><span class='add_span'>" + friendName + "</span > <button class='add_btn' onclick = 'addFriend(" + friendUserid + ")'> 添加</button > </li>";

        // console.log(friendName);
        // console.log(friendUserid);
        console.log(showHtml);
    }
    else//找不到好友
    {
        showHtml = "<li><span class='add_span'>用户不存在 </span ></li>"
    }
    $('#findFriend_result').html("");
    $('#findFriend_result').html(showHtml);

}

function operateFriend_respondDeal(jsonData) {
    if (jsonData[1]["type"] === 2) {// 收到加好友请求
        var friendName = jsonData[1]["username"];
        var friendUserid = jsonData[1]["userid"];

        var showHtml = "<li id = '" + friendUserid + "_" + friendName + "' clsss='systemMsg_li'><span class='add_span'>" + friendName + "添加好友</span><button class='add_btn' onclick = 'agreeAddFriend(" + friendUserid + "," + friendName + ")'>同意</button></li>"
        console.log(showHtml);
        $('#systemMsg').append(showHtml);
        // var showHtml = "<li><span class='add_span'>" + friendName + "</span > <button class='add_btn' onclick = 'addFriend(" + friendUserid + ")'> 添加</button > </li>";
    }
    if (jsonData[1]["type"] === 3) {// 加好友请求的应答

        if (jsonData[1]["accept"] === 1) {// 同意
            var friendName = jsonData[1]["username"];
            var friendUserid = jsonData[1]["userid"];
            var li_id = friendUserid + "_" + friendName;
            $("#" + li_id).remove();//还是直接删除算了
            // var add_btn = $("#" + li_id).children("button.add_btn")
            // add_btn.text("已同意");
            // add_btn.attr("disabled", true);
            requsetFriendList();
        }
    }
}