package com.example.wxhk.tcp.vertx;

import com.example.wxhk.WxhkApplication;
import com.example.wxhk.constant.WxMsgType;
import io.vertx.core.AbstractVerticle;
import io.vertx.core.DeploymentOptions;
import io.vertx.core.Future;
import io.vertx.core.Promise;
import io.vertx.core.http.HttpServer;
import io.vertx.core.http.HttpServerOptions;
import io.vertx.core.json.JsonObject;
import io.vertx.core.parsetools.JsonParser;
import org.dromara.hutool.log.Log;
import org.springframework.boot.CommandLineRunner;
import org.springframework.core.annotation.Order;
import org.springframework.stereotype.Component;

import java.util.Objects;

/**
 * 接受微信hook信息
 *
 * @author wt
 * @date 2023/05/26
 */
@Component
@Order()
public class VertxHttp extends AbstractVerticle implements CommandLineRunner {

    protected static final Log log = Log.get();
    HttpServer httpServer;

    @Override
    public void start(Promise<Void> startPromise) throws Exception {

        httpServer = vertx.createHttpServer(new HttpServerOptions().setPort(InitWeChat.getVertxPort()));
        JsonParser parser = JsonParser.newParser();
        parser.objectValueMode();
        parser.handler(event -> {
            switch (event.type()) {
                case START_OBJECT -> {
                }
                case END_OBJECT -> {
                }
                case START_ARRAY -> {
                }
                case END_ARRAY -> {
                }
                case VALUE -> {
                    JsonObject entries = event.objectValue();

                    if(Objects.equals(entries.getInteger("type"), WxMsgType.扫码触发.getType()) ||
                            Objects.equals(entries.getInteger("type"), WxMsgType.转账和收款.getType())){
                        VertxTcp.LINKED_BLOCKING_QUEUE_MON.add(entries);
                    }else{
                        VertxTcp.LINKED_BLOCKING_QUEUE.add(entries);
                    }
                }
            }
        });
        httpServer.requestHandler(event -> {
            event.handler(parser);
        });
        Future<HttpServer> listen = httpServer.listen();


    }

    @Override
    public void run(String... args) throws Exception {
        WxhkApplication.vertx.deployVerticle(this, new DeploymentOptions().setWorkerPoolSize(6));
    }
}
