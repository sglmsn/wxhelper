package com.example.wxhk.controller;


import com.example.wxhk.constant.WxMsgType;
import com.example.wxhk.model.PrivateChatMsg;
import com.example.wxhk.tcp.vertx.ArrHandle;
import org.dromara.hutool.log.Log;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

import java.util.Objects;

@RestController
@RequestMapping("/wx")
public class WxMsgController {

    protected static final Log log = Log.get();

    @RequestMapping("msg")
    public Object msg( @RequestBody PrivateChatMsg msg){

        if(Objects.equals(msg.getType(), WxMsgType.扫码触发.getType()) ||
                Objects.equals(msg.getType(), WxMsgType.转账和收款.getType())){
            ArrHandle.LINKED_BLOCKING_QUEUE_MON.add(msg);
        }else{
            ArrHandle.LINKED_BLOCKING_QUEUE.add(msg);
        }
        return true;
    }


}
