package com.example.wxhk.server.impl;

import com.example.wxhk.model.PrivateChatMsg;
import com.example.wxhk.model.dto.PayoutInformation;
import com.example.wxhk.model.request.ConfirmThePayment;
import com.example.wxhk.util.HttpSendUtil;
import org.dromara.hutool.core.text.StrUtil;
import org.dromara.hutool.core.util.XmlUtil;
import org.dromara.hutool.log.Log;
import org.springframework.stereotype.Service;
import org.w3c.dom.Document;
import org.w3c.dom.Element;

@Service
public class WxSmgServerImpl implements com.example.wxhk.server.WxSmgServer {

    protected  static  final Log log=Log.get();

    public static final String FILEHELPER = "filehelper";
    @Override
    public void 接到收款(PayoutInformation payoutInformation) {
        HttpSendUtil.确认收款(new ConfirmThePayment().setWxid(payoutInformation.wxid()).setTranscationId(payoutInformation.transcationid()).setTransferId(payoutInformation.transferid()));
    }
    @Override
    public void 收款之后(PayoutInformation pay) {
        HttpSendUtil.发送文本(pay.wxid(), StrUtil.format("收到款项:{},备注:{}", pay.decimal().stripTrailingZeros().toPlainString(), pay.remark()));
    }

    @Override
    public void 私聊(PrivateChatMsg chatMsg) {
        log.info("收到:{}的信息:{}",chatMsg.getFromUser(),chatMsg.getContent());
    }

    @Override
    public void 群聊(PrivateChatMsg chatMsg) {

    }

    @Override
    public void 手机发出信息(PrivateChatMsg chatMsg) {
        log.info("对:{}发出信息:{}",chatMsg.getFromUser(),chatMsg.getContent());
    }

    @Override
    public void 文件助手(PrivateChatMsg chatMsg) {

    }

    @Override
    public void 收到名片(PrivateChatMsg chatMsg) {
        if (FILEHELPER.equals(chatMsg.getFromUser())) {
            Document document = XmlUtil.parseXml(chatMsg.getContent());
            Element documentElement = document.getDocumentElement();
            String username = documentElement.getAttribute("username");
            if (StrUtil.isNotBlank(username)) {
                HttpSendUtil.发送文本(username);
            }
        }
    }

    @Override
    public void 收到好友请求(PrivateChatMsg chatMsg) {
        HttpSendUtil.通过好友请求(chatMsg);
    }

    @Override
    public void 扫码收款(PayoutInformation payoutInformation) {
        HttpSendUtil.发送文本(payoutInformation.wxid(), StrUtil.format("扫码收款:{},备注:{}", payoutInformation.decimal().stripTrailingZeros().toPlainString(), payoutInformation.remark()));
    }
}
