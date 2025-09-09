#ifndef UWEBSPIKE_H
#define UWEBSPIKE_H

#if defined(ESP32)
#define SDDS_WEBSPIKE_SYNC 1
#endif

#include "uPlainCommHandler.h"
#include "uWebServer.h"
#include "uMultask.h"
#include "uMemoryUtils.h"
#include <vector>

#if SDDS_WEBSPIKE_SYNC
/**
 * @brief WebSocket events must be processed synchronously.
 * On RTOS-based platforms we therefore enqueue server events
 * for later handling in the main task context.
 * 
 * Elements in the queue are TwsEvent
 */
class TwsEvent {
		union{
			AwsFrameInfo awsInfo;
		} Farg;
		std::vector<uint8_t> Fdata;
	public:
		AwsEventType type;
		uint32_t clientId;

		TwsEvent() = default;

		TwsEvent(AsyncWebSocketClient *_client, AwsEventType _type, void* _arg, uint8_t *_data, size_t len){
			type = _type;
			if (type == WS_EVT_DATA){
				Farg.awsInfo = *(AwsFrameInfo*)_arg;
			}
			clientId = _client->id();
			Fdata.assign(_data, _data + len);
			Fdata.push_back(0);
		}

		uint8_t* data(){ return Fdata.data(); }
		size_t len() { return Fdata.size() - 1; }

		void* arg(){
			if (type == WS_EVT_DATA) return &Farg.awsInfo;
			return nullptr;
		}
};

/**
 * @brief Ringbuffer to enqueue server events
 * for later handling in the main task context.
 */
template <class T, int SIZE>
class TsyncedQueue{
		sdds::memUtils::TringBuffer<T,SIZE> Fq;
	public:
		TisrEvent SyncEvent;

		bool hasData(){ return !Fq.isEmpty(); }

		T read(){
			return Fq.read();
		}

		bool write(T* d){
			bool res = Fq.write(*d);
			SyncEvent.signal();
			return res;
		}
};

typedef TsyncedQueue<TwsEvent,10> TwebSpikeQ;

#endif

class TwebsocketPlainComm : public TplainCommHandler{
	public:
		using TplainCommHandler::TplainCommHandler;
};

class TwebSocketClientContext : public TstringStream{
  private:
    AsyncWebSocketClient* Fclient;
    TwebsocketPlainComm FcommHandler;
  public:
    TwebSocketClientContext(TmenuHandle* _ds, TstringStreamBuffer& _buffer, 
      AsyncWebSocketClient* _client) 
      : TstringStream(&_buffer)
      , Fclient(_client)
      , FcommHandler(_ds,this)
    {
    } 

    bool isClient(AsyncWebSocketClient* _client) { return _client == Fclient; }
    bool hasSameIp(AsyncWebSocketClient* _client) { 
      if (!Fclient){
        //Serial.println("TwebSocketClientContext.hasSameIp: Fclient=nil");
        return false;
      }
      return _client->remoteIP() == Fclient->remoteIP(); 
    }
    bool inUse() { return Fclient != nullptr; }
    
    void doOnDisconnect(){
      FcommHandler.shutdown();      //tell plainCommHanlder to shut the fuck up
      Fclient = nullptr;
    }

	bool connected(){
		if (!Fclient) return false;
		return Fclient->status() == WS_CONNECTED;
	}

    void sendMessage(const char* _msg){
		if (Fclient) Fclient->text(_msg);
    }

    void doOnConnect(AsyncWebSocketClient* _client){
      //Serial.println("TwebSocketClientContext.doConnect");
      Fclient = _client;
			FcommHandler.signal();
    }

    void handleMessage(const char* _msg, size_t _len){
		TsubStringRef msg(_msg,_len);
		FcommHandler.handleMessage(msg);
    }

    void flush() override { 
      sendMessage(data());
      TstringStream::clear();
    }
};

typedef TwebSocketClientContext* TpWebSocketClientContext;

class TwebSocket : public AsyncWebSocket{
  private:
    static const int MAX_CLIENTS = 4;

	//shared buffer for all clients, as we know we use it one at a time
    TmenuHandle* Fds;
    TstringStreamBuffer Fbuffer;
    TwebSocketClientContext* FclientContexts[MAX_CLIENTS];
#if SDDS_WEBSPIKE_SYNC
	TwebSpikeQ FmessageQ;
#endif

  public:
   void init(TwebServer& _server);
    // 
    TwebSocket(TmenuHandle& _ds, TwebServer& _server) 
      : AsyncWebSocket("/ws")
      , Fds(_ds)
    {
      for (auto i=0; i<MAX_CLIENTS; i++){ FclientContexts[i] = nullptr; }
      //init(_server);
      //Fbuffer = makeBuffer(1024);
#if SDDS_WEBSPIKE_SYNC
		on(FmessageQ.SyncEvent){ 
			onEventSynced();
		};
#endif
    }

  private:
    TpWebSocketClientContext findClientCtx(AsyncWebSocketClient* _client){
      for (auto i=0; i<MAX_CLIENTS; i++){ 
        TwebSocketClientContext* ctx = FclientContexts[i];
        if (!ctx) continue; 
        if (ctx->isClient(_client)){
          return ctx;
        }
      }
      return nullptr;
    }

	void closeIdleClients(){
		for (auto i=0; i<MAX_CLIENTS; i++){ 
			TwebSocketClientContext* ctx = FclientContexts[i];
			if (!ctx) continue; 
			if (ctx->connected()) continue;
			ctx->doOnDisconnect();
		}
	}

    TpWebSocketClientContext findClientCtxByIp(AsyncWebSocketClient* _client){      
      for (auto i=0; i<MAX_CLIENTS; i++){ 
        TwebSocketClientContext* ctx = FclientContexts[i];
        if (!ctx) continue; 
        if (ctx->hasSameIp(_client)){
          return ctx;
        }
      }
      return nullptr;
    }

    TpWebSocketClientContext rejectClient(AsyncWebSocketClient* _client, const char* _error){
      //Serial.println(_error);
      _client->text(_error);
      _client->close();
      return nullptr;
    }

    TpWebSocketClientContext allocateClientContext(AsyncWebSocketClient* _client){
      for (auto i=0; i<MAX_CLIENTS; i++){ 
        TwebSocketClientContext* ctx = FclientContexts[i];
        
        //slot in context array available?
        if (!ctx) {
          ctx = new TwebSocketClientContext(Fds,Fbuffer,_client);
          if (ctx == nullptr) return rejectClient(_client,"E 0 100 no memory");
          FclientContexts[i]=ctx;
          return ctx;
        };

        //if curr ctx is not used then reuse it
        if (!ctx->inUse()) return ctx;
      }

      return rejectClient(_client,"E 0 100 maximum number of clients exeeded");
    }

	void doOnClientConnect(AsyncWebSocketClient* _client){		
		//Serial.printf("TwebSocket.onEvent client #%u connected from %s\n", _client->id(), _client->remoteIP().toString().c_str());

		closeIdleClients();

		if (findClientCtx(_client)){
			//check the ip!?
			//Serial.println("client alredy exists....");
			return;
		}

		//allow to connect the same ip multiple times???
		if (findClientCtxByIp(_client)){
			//Serial.println("client with this ip alredy connected....");
			return;
		}

		auto ctx = allocateClientContext(_client);
		if (ctx) ctx->doOnConnect(_client);
	}

    void doOnClientDisconnect(AsyncWebSocketClient* _client){
		//Serial.printf("TwebSocket.onEvent client #%u disconnected\n", _client->id());
		TpWebSocketClientContext ctx = findClientCtx(_client);
		if (ctx) ctx->doOnDisconnect();
		closeIdleClients();
    }

    void handleWebSocketMessage(AsyncWebSocketClient *client, void *arg, uint8_t *data, size_t len) {
      //Serial.println("->handleWebSocketMessage");
      AwsFrameInfo *info = (AwsFrameInfo*)arg;
      if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
		char* msg = (char*)data;
		if (*msg == '0')
			return;
        auto ctx = findClientCtx(client);
        if (ctx) ctx->handleMessage(msg,len);
      }
    }

	void onEvent(AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
        switch (type) {
          case WS_EVT_CONNECT:
            doOnClientConnect(client);
            break;
          case WS_EVT_DISCONNECT:
            doOnClientDisconnect(client);
            break;
          case WS_EVT_DATA:
            handleWebSocketMessage(client, arg, data, len);
            break;
          case WS_EVT_PING:
			//Serial.println("PING");
			break;
          case WS_EVT_PONG:
			//Serial.println("PONG");
			break;
          case WS_EVT_ERROR:
			//Serial.println("ERROR");
			break;
      }
	}

#if SDDS_WEBSPIKE_SYNC
	void onEventSynced() {
		//Serial.print(" in syncEvent taskName= ");
		//Serial.println(pcTaskGetName( nullptr ));
		while(FmessageQ.hasData()){
			auto d = FmessageQ.read();
			auto c = client(d.clientId);
			if (c){
				onEvent(c,d.type,d.arg(),d.data(),d.len());
			}
		}
	}
#endif

  public:
    void _onEvent(AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
        if (!client) return;

#if SDDS_WEBSPIKE_SYNC
		//Serial.print(" in WebSocket.onEvent taskName= ");
    	//Serial.println(pcTaskGetName( nullptr ));
		TwsEvent ws(client,type,arg,data,len);
		FmessageQ.write(&ws);
#else
		onEvent(client,type,arg,data,len);
#endif
	}
};

class TwebSpike : public Tthread{
  private:
    TwebServer FwebServer;
    TwebSocket FwebSocket;

  public:
    TwebSpike(TmenuHandle& _ds) 
      : FwebServer(80)
      , FwebSocket(_ds,FwebServer)
    {

    }

    void begin();

    void execute(Tevent* _ev) override{
      /* 
      without the next line we got some strange errors and reboots in some cases:
          assert failed: tcpip_api_call IDF/components/lwip/lwip/src/api/tcpip.c:497 (Invalid mbox)
      */
	#if defined(ESP32)
		esp_netif_init();
	#elif defined(ESP8266)
		netif_init();
	#endif
      begin();
    }

};


#endif