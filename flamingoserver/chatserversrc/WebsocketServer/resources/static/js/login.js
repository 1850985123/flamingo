/*
 * @Author: 1850985123 1850985123@qq.com
 * @Date: 2022-07-21 14:02:19
 * @LastEditors: 1850985123 1850985123@qq.com
 * @LastEditTime: 2022-07-21 16:26:24
 * @FilePath: \前端学习\resources\static\js\login.js
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */

$(function () {

    $("#register_btn").click(function () {
        register();
    });
    $("#login_btn").click(function () {
        login();
    });

    //usernameInput 获得焦点
    $("#usernameInput").focus(function () {
        if (this.value === "用户名") {
            this.value = "";
            this.style.color = "black";
        }
        // this.className = "onfocus";
        this.className = "usernameInput onfocus";
    });

    //usernameInput 失去焦点
    $("#usernameInput").blur(function () {
        if (this.value === "") {
            this.value = "用户名"
            this.style.color = "rgb(152, 158, 158)";
        }
        // this.className = "onblur"; //直接修改元素的类名来修改样式
        this.className = "usernameInput onblur"; //多类名
    });

    //passwordInput 失去焦点
    $("#passwordInput").blur(function () {
        this.className = "passwordInput onblur"; //多类名
        if (this.value.length < 6 || this.value.length > 16) {
            // alert("密码长度在 6- 16 位之间，现在长度 = " + this.value.length);
        }
    });

    //passwordInput 获得焦点
    $("#passwordInput").focus(function () {
        console.log("passwordInput 获得焦点")
        this.className = "passwordInput onfocus";
    });

    //passwordImage 图片点击事件
    $("#passwordImage").click(function () {
        if (passwordInput.type === "text") {
            passwordInput.type = "password";
            this.src = "../image/close.png";
        } else {
            passwordInput.type = "text";
            this.src = "../image/open.png";
        }
    });
});


function login() {
    var data = [];
    data[0] = {
        "cmd": 1002,
        "seq": 0
    };
    data[1] = {};
    data[1]['username'] = $("#usernameInput").val();
    data[1]['password'] = $("#passwordInput").val();
    data[1]['clienttype'] = 1;
    data[1]['status'] = 1;

    $.ajaxSetup({
        contentType: "application/json; charset=utf-8"
    })
    $.post("login", JSON.stringify(data), function (res) {
        console.log(res);
        var jsonRoot = JSON.parse(res);
        if (jsonRoot[1]["msg"] === "ok") {
            location.href = 'chat.html';
        } else {
            alert("登录失败：" + jsonRoot[1]["msg"]);
        }
    });
}

function register() {

    location.href = 'register.html';
    // $.ajaxSetup({
    //         contentType: "application/json; charset=utf-8"
    //     })
    //     // JSON.stringify(data),
    // $.get("register", function(res) {
    //     console.log(res);
    //     var jsonRoot = JSON.parse(res);
    //     if (jsonRoot[1]["msg"] === "ok") {
    //         location.href = 'toChatroom';
    //     } else {
    //         alert("登录失败：" + jsonRoot[1]["msg"]);
    //     }
    // });
}