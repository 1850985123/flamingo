
function login() {
    var data = [];
    data[0] = {
        "cmd": 1002,
        "seq": 0
    };
    data[1] = {};
    // data[1]['username'] = "1850985123";
    // data[1]['password'] = "dgy199613";

    data[1]['username'] = $("#uname").val();
    data[1]['password'] = $("#pwd").val();

    data[1]['clienttype'] = 1;
    data[1]['status'] = 1;


    $.ajaxSetup({ contentType: "application/json; charset=utf-8" })
    // alert("post 回调函数");
    // $.get("login", function (res) {
    //     // console.log("进入post 你好");
    //     alert("进入get ");
    //     // alert(JSON.stringify(res));
    //     // if (res.flag){
    //     //     location.href = 'chat.html';
    //     // }else {
    //     //     alert(JSON.stringify(res));
    //     // }
    // });//'json'

    $.post("login", JSON.stringify(data), function (res) {
        // console.log("进入post 你好");
        // alert("进入post 回调函数");
        // alert(JSON.stringify(res));
        console.log(res);
        var jsonRoot = JSON.parse(res);
        if (jsonRoot[1]["msg"] === "ok") {
            // alert("登录成功！");
            location.href = 'toChatroom';
        } else {
            alert("登录失败：" + jsonRoot[1]["msg"]);
        }
    });//'json'
}