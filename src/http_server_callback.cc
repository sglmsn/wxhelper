﻿#include "pch.h"
#include "http_server_callback.h"
#include "http_server.h"
#include "export.h"
#include "global_context.h"
#include "hooks.h"
#include "db.h"

#define STR2LL(str) (wxhelper::Utils::IsDigit(str) ? stoll(str) : 0)
namespace common = wxhelper::common;



INT64 GetINT64Param(nlohmann::json data, std::string key) {
  INT64 result;
  try {
    result = data[key].get<INT64>();
  } catch (nlohmann::json::exception) {
    result = STR2LL(data[key].get<std::string>());
  }
  return result;
}

std::string GetStringParam(nlohmann::json data, std::string key) {
  return data[key].get<std::string>();
}

std::wstring GetWStringParam(nlohmann::json data, std::string key) {
  return wxhelper::Utils::UTF8ToWstring(data[key].get<std::string>());
}

std::vector<std::wstring> GetArrayParam(nlohmann::json data,  std::string key) {
  std::vector<std::wstring> result;
  std::wstring param = GetWStringParam(data, key);
  result = wxhelper::Utils::split(param, L',');
  return result;
}


void StartHttpServer(wxhelper::HttpServer *server) {
  int port = server->GetPort();
  std::string lsten_addr = "http://0.0.0.0:" + std::to_string(port);
  if (mg_http_listen(const_cast<mg_mgr *>(server->GetMgr()), lsten_addr.c_str(),
                     EventHandler,
                     const_cast<mg_mgr *>(server->GetMgr())) == NULL) {
    SPDLOG_INFO("http server  listen fail.port:{}", port);
#ifdef _DEBUG
    MG_INFO(("http server  listen fail.port: %d", port));
#endif
    return;
  }
  for (;;) {
    mg_mgr_poll(const_cast<mg_mgr *>(server->GetMgr()), 1000);
  }
}

void EventHandler(struct mg_connection *c, int ev, void *ev_data,
                  void *fn_data) {
  if (ev == MG_EV_OPEN) {
  } else if (ev == MG_EV_HTTP_MSG) {
    struct mg_http_message *hm = (struct mg_http_message *)ev_data;
    if (mg_http_match_uri(hm, "/websocket")) {
      mg_ws_upgrade(c, hm, NULL);
    } else if (mg_http_match_uri(hm, "/api/*")) {
      HandleHttpRequest(c, hm);
    } else {
      nlohmann::json res = {{"code", 400},
                            {"msg", "invalid url, please check url"},
                            {"data", NULL}};
      std::string ret = res.dump();
      mg_http_reply(c, 200, "Content-Type: application/json\r\n", "%s\n",
                    ret.c_str());
    }
  } else if (ev == MG_EV_WS_MSG) {
    HandleWebsocketRequest(c, ev_data);
  }
  (void)fn_data;
}

void HandleHttpRequest(struct mg_connection *c, void *ev_data) {
  struct mg_http_message *hm = (struct mg_http_message *)ev_data;
  std::string ret = R"({"code":200,"msg":"success"})";
  try {
    ret = HttpDispatch(c, hm);
  } catch (nlohmann::json::exception &e) {
    nlohmann::json res = {{"code", "500"}, {"msg", e.what()}, {"data", NULL}};
    ret = res.dump();
  }
  if (ret != "") {
    mg_http_reply(c, 200, "Content-Type: application/json\r\n", "%s\n",
                  ret.c_str());
  }
}

void HandleWebsocketRequest(struct mg_connection *c, void *ev_data) {
  // Got websocket frame. Received data is wm->data. Echo it back!
  struct mg_ws_message *wm = (struct mg_ws_message *)ev_data;
  mg_ws_send(c, wm->data.ptr, wm->data.len, WEBSOCKET_OP_TEXT);
}

std::string HttpDispatch(struct mg_connection *c, struct mg_http_message *hm) {
  std::string ret;
  if (mg_vcasecmp(&hm->method, "GET") == 0) {
    nlohmann::json ret_data = {{"code", 200},
                               {"data", {}},
                               {"msg", "not support get method,use post."}};
    ret = ret_data.dump();
    return ret;
  }

  nlohmann::json j_param = nlohmann::json::parse(
      hm->body.ptr, hm->body.ptr + hm->body.len, nullptr, false);
  if (hm->body.len != 0 && j_param.is_discarded() == true) {
    nlohmann::json ret_data = {
        {"code", 200}, {"data", {}}, {"msg", "json string is invalid."}};
    ret = ret_data.dump();
    return ret;
  }
  if (wxhelper::GlobalContext::GetInstance().state !=
      wxhelper::GlobalContextState::INITIALIZED) {
    nlohmann::json ret_data = {
        {"code", 200}, {"data", {}}, {"msg", "global context is initializing"}};
    ret = ret_data.dump();
    return ret;
  }
  if (mg_http_match_uri(hm, "/api/checkLogin")) {
    INT64 success = wxhelper::GlobalContext::GetInstance().mgr->CheckLogin();
    nlohmann::json ret_data = {
        {"code", success}, {"data", {}}, {"msg", "success"}};
    ret = ret_data.dump();
    return ret;
  } else if (mg_http_match_uri(hm, "/api/userInfo")) {
    common::SelfInfoInner self_info;
    INT64 success =
        wxhelper::GlobalContext::GetInstance().mgr->GetSelfInfo(self_info);
    nlohmann::json ret_data = {
        {"code", success}, {"data", {}}, {"msg", "success"}};
    if (success) {
      nlohmann::json j_info = {
          {"name", self_info.name},
          {"city", self_info.city},
          {"province", self_info.province},
          {"country", self_info.country},
          {"account", self_info.account},
          {"wxid", self_info.wxid},
          {"mobile", self_info.mobile},
          {"headImage", self_info.head_img},
          {"signature", self_info.signature},
          {"dataSavePath", self_info.data_save_path},
          {"currentDataPath", self_info.current_data_path},
          {"dbKey", self_info.db_key},
      };
      ret_data["data"] = j_info;
    }
    ret = ret_data.dump();
    return ret;
  } else if (mg_http_match_uri(hm, "/api/sendTextMsg")) {
    std::wstring wxid = GetWStringParam(j_param, "wxid");
    std::wstring msg = GetWStringParam(j_param, "msg");
    INT64 success =
        wxhelper::GlobalContext::GetInstance().mgr->SendTextMsg(wxid, msg);
    nlohmann::json ret_data = {
        {"code", success}, {"data", {}}, {"msg", "success"}};
    ret = ret_data.dump();
    return ret;
  } else if (mg_http_match_uri(hm, "/api/hookSyncMsg")) {
    INT64 success =
        wxhelper::hooks::HookSyncMsg("127.0.0.1", 19099, "", 3000, false);
    nlohmann::json ret_data = {
        {"code", success}, {"data", {}}, {"msg", "success"}};
    ret = ret_data.dump();
    return ret;
  } else if (mg_http_match_uri(hm, "/api/sendImagesMsg")) {
    std::wstring wxid = GetWStringParam(j_param, "wxid");
    std::wstring path = GetWStringParam(j_param, "imagePath");
    INT64 success =
        wxhelper::GlobalContext::GetInstance().mgr->SendImageMsg(wxid, path);
    nlohmann::json ret_data = {
        {"code", success}, {"data", {}}, {"msg", "success"}};
    ret = ret_data.dump();
    return ret;
  } else if (mg_http_match_uri(hm, "/api/sendFileMsg")) {
    std::wstring wxid = GetWStringParam(j_param, "wxid");
    std::wstring path = GetWStringParam(j_param, "filePath");
    INT64 success =
        wxhelper::GlobalContext::GetInstance().mgr->SendFileMsg(wxid, path);
    nlohmann::json ret_data = {
        {"code", success}, {"data", {}}, {"msg", "success"}};
    ret = ret_data.dump();
    return ret;
  } else if (mg_http_match_uri(hm, "/api/getContactList")) {
    std::vector<common::ContactInner> vec;
    INT64 success =
        wxhelper::GlobalContext::GetInstance().mgr->GetContacts(vec);
    nlohmann::json ret_data = {
        {"code", success}, {"data", {}}, {"msg", "success"}};
    for (unsigned int i = 0; i < vec.size(); i++) {
      nlohmann::json item = {
          {"customAccount", vec[i].custom_account},
          {"encryptName", vec[i].encrypt_name},
          {"type", vec[i].type},
          {"verifyFlag", vec[i].verify_flag},
          {"wxid", vec[i].wxid},
          {"nickname", vec[i].nickname},
          {"pinyin", vec[i].pinyin},
          {"pinyinAll", vec[i].pinyin_all},
          {"reserved1", vec[i].reserved1},
          {"reserved2", vec[i].reserved2},
      };
      ret_data["data"].push_back(item);
    }
    ret = ret_data.dump();
    return ret;
  } else if (mg_http_match_uri(hm, "/api/unhookSyncMsg")) {
    INT64 success = wxhelper::hooks::UnHookSyncMsg();
    nlohmann::json ret_data = {
        {"code", success}, {"data", {}}, {"msg", "success"}};
    ret = ret_data.dump();
    return ret;
  } else if (mg_http_match_uri(hm, "/api/getDBInfo")) {
    std::vector<void *> v_ptr = wxhelper::DB::GetInstance().GetDbHandles();
    nlohmann::json ret_data = {{"data", nlohmann::json::array()}};
    for (unsigned int i = 0; i < v_ptr.size(); i++) {
      nlohmann::json db_info;
      db_info["tables"] = nlohmann::json::array();
      common::DatabaseInfo *db =
          reinterpret_cast<common::DatabaseInfo *>(v_ptr[i]);
      db_info["handle"] = db->handle;
      std::wstring dbname(db->db_name);
      db_info["databaseName"] = wxhelper::Utils::WstringToUTF8(dbname);
      for (auto table : db->tables) {
        nlohmann::json table_info = {{"name", table.name},
                                     {"tableName", table.table_name},
                                     {"sql", table.sql},
                                     {"rootpage", table.rootpage}};
        db_info["tables"].push_back(table_info);
      }
      ret_data["data"].push_back(db_info);
    }
    ret_data["code"] = 1;
    ret_data["msg"] = "success";
    ret = ret_data.dump();
    return ret;
  } else if (mg_http_match_uri(hm, "/api/execSql")) {
    UINT64 db_handle = GetINT64Param(j_param, "dbHandle");
    std::string sql = GetStringParam(j_param, "sql");
    std::vector<std::vector<std::string>> items;
    int success =
        wxhelper::DB::GetInstance().Select(db_handle, sql.c_str(), items);
    nlohmann::json ret_data = {{"data", nlohmann::json::array()},
                               {"code", success},
                               {"msg", "success"}};
    if (success == 0) {
      ret_data["msg"] = "no data";
      ret = ret_data.dump();
      return ret;
    }
    for (auto it : items) {
      nlohmann::json temp_arr = nlohmann::json::array();
      for (size_t i = 0; i < it.size(); i++) {
        temp_arr.push_back(it[i]);
      }
      ret_data["data"].push_back(temp_arr);
    }
    ret = ret_data.dump();
    return ret;
  } else if (mg_http_match_uri(hm, "/api/getChatRoomDetailInfo")) {
    std::wstring chat_room_id = GetWStringParam(j_param, "chatRoomId");
    common::ChatRoomInfoInner chat_room_detail;
    INT64 success =
        wxhelper::GlobalContext::GetInstance().mgr->GetChatRoomDetailInfo(
            chat_room_id, chat_room_detail);
    nlohmann::json ret_data = {
        {"code", success}, {"msg", "success"}, {"data", {}}};

    nlohmann::json detail = {
        {"chatRoomId", chat_room_detail.chat_room_id},
        {"notice", chat_room_detail.notice},
        {"admin", chat_room_detail.admin},
        {"xml", chat_room_detail.xml},
    };
    ret_data["data"] = detail;
    ret = ret_data.dump();
    return ret;
   } else if (mg_http_match_uri(hm, "/api/addMemberToChatRoom")) {
    std::wstring room_id = GetWStringParam(j_param, "chatRoomId");
    std::vector<std::wstring> wxids = GetArrayParam(j_param, "memberIds");
    std::vector<std::wstring> wxid_list;
    for (unsigned int i = 0; i < wxids.size(); i++) {
      wxid_list.push_back(wxids[i]);
    }
    INT64 success =
        wxhelper::GlobalContext::GetInstance().mgr->AddMemberToChatRoom(
            room_id, wxid_list);
    nlohmann::json ret_data = {
        {"code", success}, {"msg", "success"}, {"data", {}}};
    ret = ret_data.dump();
    return ret;
  } else if (mg_http_match_uri(hm, "/api/modifyNickname")) {
    std::wstring room_id = GetWStringParam(j_param, "chatRoomId");
    std::wstring wxid = GetWStringParam(j_param, "wxid");
    std::wstring nickName = GetWStringParam(j_param, "nickName");
    INT64 success =
        wxhelper::GlobalContext::GetInstance().mgr->ModChatRoomMemberNickName(
            room_id, wxid, nickName);
    nlohmann::json ret_data = {
        {"code", success}, {"msg", "success"}, {"data", {}}};
    ret = ret_data.dump();
    return ret;
  } else if (mg_http_match_uri(hm, "/api/delMemberFromChatRoom")) {
    std::wstring room_id = GetWStringParam(j_param, "chatRoomId");
    std::vector<std::wstring> wxids = GetArrayParam(j_param, "memberIds");
    std::vector<std::wstring> wxid_list;
    for (unsigned int i = 0; i < wxids.size(); i++) {
      wxid_list.push_back(wxids[i]);
    }
    INT64 success =
        wxhelper::GlobalContext::GetInstance().mgr->DelMemberFromChatRoom(
            room_id, wxid_list);
    nlohmann::json ret_data = {
        {"code", success}, {"msg", "success"}, {"data", {}}};
    ret = ret_data.dump();
    return ret;
  } else if (mg_http_match_uri(hm, "/api/getMemberFromChatRoom")) {
    std::wstring room_id = GetWStringParam(j_param, "chatRoomId");
    common::ChatRoomMemberInner member;
    INT64 success =
        wxhelper::GlobalContext::GetInstance().mgr->GetMemberFromChatRoom(
            room_id, member);
    nlohmann::json ret_data = {
        {"code", success}, {"msg", "success"}, {"data", {}}};
    if (success >= 0) {
      nlohmann::json member_info = {
          {"admin", member.admin},
          {"chatRoomId", member.chat_room_id},
          {"members", member.member},
          {"adminNickname", member.admin_nickname},
          {"memberNickname", member.member_nickname},
      };
      ret_data["data"] = member_info;
    }
    ret = ret_data.dump();
    return ret;
  } else {
    nlohmann::json ret_data = {
        {"code", 200}, {"data", {}}, {"msg", "not support url"}};
    ret = ret_data.dump();
    return ret;
  }
  nlohmann::json ret_data = {
      {"code", 200}, {"data", {}}, {"msg", "unreachable code."}};
  ret = ret_data.dump();
  return ret;
}