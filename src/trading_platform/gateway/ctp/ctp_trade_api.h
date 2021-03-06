// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_GATEWAY_CTP_CTP_TRADE_API_H_
#define FT_SRC_GATEWAY_CTP_CTP_TRADE_API_H_

#include <ThostFtdcTraderApi.h>

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <string>

#include "core/config.h"
#include "core/constants.h"
#include "gateway/ctp/ctp_common.h"
#include "interface/gateway.h"
#include "interface/trading_engine_interface.h"

namespace ft {

class CtpGateway;

class CtpTradeApi : public CThostFtdcTraderSpi {
 public:
  explicit CtpTradeApi(TradingEngineInterface *engine);

  ~CtpTradeApi();

  bool login(const Config &config);

  void logout();

  bool send_order(const OrderReq &order);

  bool cancel_order(uint64_t order_id);

  bool query_contract(const std::string &ticker, const std::string &exchange);

  bool query_contracts();

  bool query_position(const std::string &ticker);

  bool query_positions();

  bool query_account();

  bool query_trades();

  bool query_margin_rate(const std::string &ticker);

  // 当客户端与交易后台建立起通信连接时（还未登录前），该方法被调用。
  void OnFrontConnected() override;

  // 当客户端与交易后台通信连接断开时，该方法被调用。
  // 当发生这个情况后，API会自动重新连接，客户端可不做处理。
  // @param reason 错误原因
  //         0x1001 网络读失败
  //         0x1002 网络写失败
  //         0x2001 接收心跳超时
  //         0x2002 发送心跳失败
  //         0x2003 收到错误报文
  void OnFrontDisconnected(int reason) override;

  // 心跳超时警告。当长时间未收到报文时，该方法被调用。
  // @param time_lapse 距离上次接收报文的时间
  void OnHeartBeatWarning(int time_lapse) override;

  // 客户端认证响应
  void OnRspAuthenticate(CThostFtdcRspAuthenticateField *auth,
                         CThostFtdcRspInfoField *rsp_info, int req_id,
                         bool is_last) override;

  // 登录请求响应
  void OnRspUserLogin(CThostFtdcRspUserLoginField *logon,
                      CThostFtdcRspInfoField *rsp_info, int req_id,
                      bool is_last) override;

  void OnRspQrySettlementInfo(CThostFtdcSettlementInfoField *settlement,
                              CThostFtdcRspInfoField *rsp_info, int req_id,
                              bool is_last) override;

  void OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *confirm,
                                  CThostFtdcRspInfoField *rsp_info, int req_id,
                                  bool is_last) override;

  void OnRspUserLogout(CThostFtdcUserLogoutField *user_logout,
                       CThostFtdcRspInfoField *rsp_info, int req_id,
                       bool is_last) override;

  // 拒绝报单
  void OnRspOrderInsert(CThostFtdcInputOrderField *ctp_order,
                        CThostFtdcRspInfoField *rsp_info, int req_id,
                        bool is_last) override;

  void OnRspOrderAction(CThostFtdcInputOrderActionField *action,
                        CThostFtdcRspInfoField *rsp_info, int req_id,
                        bool is_last) override;

  void OnRtnOrder(CThostFtdcOrderField *ctp_order) override;

  // 成交通知
  void OnRtnTrade(CThostFtdcTradeField *trade) override;

  void OnRspQryInstrument(CThostFtdcInstrumentField *instrument,
                          CThostFtdcRspInfoField *rsp_info, int req_id,
                          bool is_last) override;

  void OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *position,
                                CThostFtdcRspInfoField *rsp_info, int req_id,
                                bool is_last) override;

  void OnRspQryTradingAccount(CThostFtdcTradingAccountField *trading_account,
                              CThostFtdcRspInfoField *rsp_info, int req_id,
                              bool is_last) override;

  void OnRspQryOrder(CThostFtdcOrderField *order,
                     CThostFtdcRspInfoField *rsp_info, int req_id,
                     bool is_last) override;

  void OnRspQryTrade(CThostFtdcTradeField *trade,
                     CThostFtdcRspInfoField *rsp_info, int req_id,
                     bool is_last) override;

  void OnRspQryInstrumentMarginRate(
      CThostFtdcInstrumentMarginRateField *margin_rate,
      CThostFtdcRspInfoField *rsp_info, int req_id, bool is_last) override;

 private:
  int next_req_id() { return next_req_id_++; }

  uint64_t get_engine_order_id(int order_ref) const {
    return order_ref - order_ref_base_;
  }

  uint64_t get_order_id(uint32_t ticker_index, uint32_t order_sys_id) const {
    return ((static_cast<uint64_t>(ticker_index) << 32) &
            0xffffffff00000000ULL) |
           (static_cast<uint64_t>(order_sys_id) & 0xffffffffULL);
  }

  void done() { is_done_ = true; }

  void error() { is_error_ = true; }

  bool wait_sync() {
    while (!is_done_)
      if (is_error_) return false;

    is_done_ = false;
    return true;
  }

 private:
  TradingEngineInterface *engine_;
  std::unique_ptr<CThostFtdcTraderApi, CtpApiDeleter> trade_api_;

  std::string front_addr_;
  std::string broker_id_;
  std::string investor_id_;
  int front_id_;
  int session_id_;
  int order_ref_base_;

  std::atomic<int> next_req_id_ = 0;

  volatile bool is_error_ = false;
  volatile bool is_connected_ = false;
  volatile bool is_done_ = false;
  volatile bool is_logon_ = false;

  std::map<uint32_t, Position> pos_cache_;
};

}  // namespace ft

#endif  // FT_SRC_GATEWAY_CTP_CTP_TRADE_API_H_
