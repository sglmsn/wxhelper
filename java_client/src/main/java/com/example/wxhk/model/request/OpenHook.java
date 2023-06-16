package com.example.wxhk.model.request;

import com.example.wxhk.infe.SendMsg;
import lombok.Data;
import lombok.experimental.Accessors;

/**
 * 开启hook
 *
 * @author wt
 * @date 2023/06/01
 */
@Data
@Accessors(chain = true)
public class OpenHook implements SendMsg<OpenHook> {
    String port;
    String ip;
    /**
     * http请求url
     */
    String url;

    /**
     * 超时时间,单位毫秒
     */
    String timeout;
    /**
     * 1.启用http 0.不启用http
     */
    Integer enableHttp;
}
