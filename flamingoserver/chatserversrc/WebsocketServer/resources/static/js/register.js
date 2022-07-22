/*
 * @Author: 1850985123 1850985123@qq.com
 * @Date: 2022-07-21 15:28:46
 * @LastEditors: 1850985123 1850985123@qq.com
 * @LastEditTime: 2022-07-21 16:26:01
 * @FilePath: \前端学习\resources\static\js\register.js
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */

$(function () {
    //passwordImage 图片点击事件
    $("#passwordImage1").click(function () {
        var passwordInput1 = $("#passwordInput1")[0];
        // console.log("进入密码点击1");
        // console.log(passwordInput1);
        if (passwordInput1.type === "text") {
            passwordInput1.type = "password";
            this.src = "../image/close.png";
        } else {
            passwordInput1.type = "text";
            this.src = "../image/open.png";
        }
    });
    //passwordImage 图片点击事件
    $("#passwordImage2").click(function () {
        var passwordInput2 = $("#passwordInput2")[0];
        // console.log("进入密码点击2");
        // console.log(passwordInput2);
        if (passwordInput2.type === "text") {
            passwordInput2.type = "password";
            this.src = "../image/close.png";
        } else {
            passwordInput2.type = "text";
            this.src = "../image/open.png";
        }
    });

    // submit_btn 提交按钮点击事件
    $("#submit_btn").click(function () {
        console.log("进入注册点击事件");
        var passwordInput1 = $("#passwordInput1");
        var passwordInput2 = $("#passwordInput2");

        var userName = $("#userName");
        var nikeName = $("#nikeName");
        console.log(nikeName.val());
        console.log(userName.val());
        console.log(passwordInput1.val());
        console.log(passwordInput2.val());

        if (nikeName.val() == "" || userName.val() == "")
            return alert("昵称或用户名为空");
        if (passwordInput1.val() == "" || passwordInput2.val() == "")
            return alert("密码为空");
        if (passwordInput1.val() != passwordInput2.val())
            return alert("两次输入的密码不一致");

        /* 组装注册协议内容 */
        var data = [];
        data[0] = {
            "cmd": 1001,
            "seq": 0
        };
        data[1] = {};
        data[1]['username'] = userName.val();
        data[1]['nickname'] = nikeName.val();
        data[1]['password'] = passwordInput1.val();

        $.ajaxSetup({
            contentType: "application/json; charset=utf-8"
        })
        $.post("register", JSON.stringify(data), function (res) {
            console.log(res);
            var jsonRoot = JSON.parse(res);
            if (jsonRoot[1]["msg"] === "ok") {
                location.href = 'login.html';
            } else {
                if (jsonRoot[1]["msg"] === "registered already") {
                    alert("用户已经注册");
                }
                else
                    alert("注册失败：" + jsonRoot[1]["msg"]);
            }
        });

    });

});