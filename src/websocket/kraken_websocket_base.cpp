#include "kraken_websocket_base.h"
#include <iostream>
#include <cstring>

KrakenWebSocketBase::KrakenWebSocketBase(const std::string& ws_endpoint)
    : ws_endpoint_(ws_endpoint), context_(nullptr), wsi_(nullptr), running_(false), connected_(false) {}

KrakenWebSocketBase::~KrakenWebSocketBase() {
  Stop();
}

int KrakenWebSocketBase::WebSocketCallback(struct lws* wsi, enum lws_callback_reasons reason,
                                           void* user, void* in, size_t len) {
  KrakenWebSocketBase* ws = static_cast<KrakenWebSocketBase*>(lws_context_user(lws_get_context(wsi)));
  
  switch (reason) {
    case LWS_CALLBACK_CLIENT_ESTABLISHED:
      std::cout << "✓ WebSocket connected" << std::endl;
      ws->connected_ = true;
      ws->SendPendingSubscription();
      break;
      
    case LWS_CALLBACK_CLIENT_RECEIVE: {
      std::string message((char*)in, len);
      try {
        json j = json::parse(message);
        ws->HandleMessage(j);
      } catch (const std::exception& e) {
        std::cerr << "Error parsing message: " << e.what() << std::endl;
      }
      break;
    }
    
    case LWS_CALLBACK_CLIENT_CLOSED:
      std::cout << "WebSocket closed" << std::endl;
      ws->running_ = false;
      ws->connected_ = false;
      break;
      
    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
      std::cerr << "WebSocket connection error";
      if (in) {
        std::cerr << ": " << (char*)in;
      }
      std::cerr << std::endl;
      ws->running_ = false;
      ws->connected_ = false;
      break;
      
    default:
      break;
  }
  
  return 0;
}

void KrakenWebSocketBase::Connect() {
  static struct lws_protocols protocols[] = {
    {
      "kraken-protocol",
      WebSocketCallback,
      0,
      4096,
    },
    { NULL, NULL, 0, 0 }
  };
  
  struct lws_context_creation_info info;
  memset(&info, 0, sizeof(info));
  
  info.port = CONTEXT_PORT_NO_LISTEN;
  info.protocols = protocols;
  info.gid = -1;
  info.uid = -1;
  info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
  info.user = this;
  
  context_ = lws_create_context(&info);
  if (!context_) {
    throw std::runtime_error("Failed to create WebSocket context");
  }
  
  struct lws_client_connect_info ccinfo;
  memset(&ccinfo, 0, sizeof(ccinfo));
  
  ccinfo.context = context_;
  ccinfo.address = "ws.kraken.com";
  ccinfo.port = 443;
  ccinfo.path = "/v2";
  ccinfo.host = ccinfo.address;
  ccinfo.origin = ccinfo.address;
  ccinfo.ssl_connection = LCCSCF_USE_SSL;
  ccinfo.protocol = protocols[0].name;
  
  wsi_ = lws_client_connect_via_info(&ccinfo);
  if (!wsi_) {
    lws_context_destroy(context_);
    throw std::runtime_error("Failed to connect to WebSocket");
  }
  
  running_ = true;
}

void KrakenWebSocketBase::Subscribe(const json& subscription) {
  pending_subscription_ = subscription.dump();
  
  if (connected_) {
    SendPendingSubscription();
  }
}

void KrakenWebSocketBase::SendMessage(const std::string& message) {
  if (!wsi_) return;
  
  unsigned char buf[LWS_PRE + 2048];
  memcpy(&buf[LWS_PRE], message.c_str(), message.length());
  lws_write(wsi_, &buf[LWS_PRE], message.length(), LWS_WRITE_TEXT);
}

void KrakenWebSocketBase::SendPendingSubscription() {
  if (pending_subscription_.empty() || !wsi_) {
    return;
  }
  
  std::cout << "✓ Subscribed to channel" << std::endl;
  SendMessage(pending_subscription_);
  pending_subscription_.clear();
}

void KrakenWebSocketBase::Run() {
  while (running_) {
    lws_service(context_, 50);
  }
}

void KrakenWebSocketBase::Stop() {
  running_ = false;
  if (context_) {
    lws_context_destroy(context_);
    context_ = nullptr;
  }
}