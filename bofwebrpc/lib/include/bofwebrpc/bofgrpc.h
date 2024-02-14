/*!
  Copyright (c) 2008, EVS. All rights reserved.

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND,  EITHER EXPRESSED OR IMPLIED,  INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  History:
    V 1.00  August 24 2013  BHA : Original version
*/
#pragma once
//https://stackoverflow.com/questions/64639004/grpc-c-async-helloworld-client-example-doesnt-do-anything-asynchronously
//-->https://github.com/grpc/grpc/blob/master/examples/cpp/helloworld/greeter_async_client2.cc
//https://flatbuffers.dev/flatbuffers_grpc_guide_use_cpp.html

#include "bofwebrpc/bofwebrpc.h"
#include <grpc++/grpc++.h>
#include <grpcpp/grpcpp.h>

#include <bofstd/bofthread.h>
#include <bofstd/bofsocketos.h>

BEGIN_WEBRPC_NAMESPACE()
#define GRPC_DBG_LOG printf

class GrpcInterceptor : public grpc::experimental::Interceptor
{
public:
  void Intercept(grpc::experimental::InterceptorBatchMethods *pMethod) override;
};

class GrpcInterceptorFactory : public grpc::experimental::ServerInterceptorFactoryInterface
{
public:
  grpc::experimental::Interceptor *CreateServerInterceptor(grpc::experimental::ServerRpcInfo *pInfo) override;
};

template <typename T>
struct GRPC_SERVER_PARAM
{
  BOF::BOF_SOCKET_ADDRESS Ip_X;
  BOF::BOF_THREAD_PARAM ServerThreadParam_X;
#if defined(ASYNC)
  bool Async_B;
#endif
  T *pService;
  GRPC_SERVER_PARAM()
  {
    Reset();
  }
  void Reset()
  {
   Ip_X.Reset();
   ServerThreadParam_X.Reset();
#if defined(ASYNC)
   Async_B = false;
#endif
   pService = nullptr;
  }
};

// Per session data will be different for each connection.
template <typename T>
class GrpcServer;

template <typename T>
struct GRPC_DATA_PER_SESSION
{
  GrpcServer<T> *pGrpcServer;
  GRPC_DATA_PER_SESSION()
  {
    Reset();
  }
  void Reset()
  {
    pGrpcServer = nullptr;
  }
};

#if defined(ASYNC)
// Class encompasing the state and logic needed to serve a request.
template <typename T>
class AsyncCallStateMachine
{
public:
  // Take in the "service" instance (in this case representing an asynchronous
  // server) and the completion queue "cq" used for asynchronous communication
  // with the gRPC runtime.
  AsyncCallStateMachine(T *service, grpc::ServerCompletionQueue *cq)
    : service_(service), cq_(cq), status_(CREATE) //TODO , responder_(&ctx_)
  {
    // Invoke the serving logic right away.
    Proceed();
  }

  void Proceed()
  {
    if (status_ == CREATE)
    {
      // Make this instance progress to the PROCESS state.
      status_ = PROCESS;

      // As part of the initial CREATE state, we *request* that the system
      // start processing SayHello requests. In this request, "this" acts are
      // the tag uniquely identifying the request (so that different CallData
      // instances can serve different requests concurrently), in this case
      // the memory address of this CallData instance.
      //TODO service_->RequestSayHello(&ctx_, &request_, &responder_, cq_, cq_,this);
      mpResponder = service_->f(&ctx_, cq_, this);
    }
    else if (status_ == PROCESS)
    {
      // Spawn a new CallData instance to serve new clients while we process
      // the one for this CallData. The instance will deallocate itself as
      // part of its FINISH state.
      new AsyncCallStateMachine(service_, cq_);

      // The actual processing.
      //std::string prefix("Hello ");
      //TODO reply_.set_message(prefix + request_.name());

      // And we are done! Let the gRPC runtime know we've finished, using the
      // memory address of this instance as the uniquely identifying tag for
      // the event.
      service_->g(&ctx_, mpResponder, this);
      //mpResponder->Finish(reply_, Status::OK, this);
      status_ = FINISH;
    }
    else
    {
      GPR_ASSERT(status_ == FINISH);
      // Once in the FINISH state, deallocate ourselves (CallData).
      delete this;
    }
  }


private:
  // The means of communication with the gRPC runtime for an asynchronous
  // server.
  T *service_;
  void *mpResponder = nullptr;
  // What we get from the client.
  //TODO HelloRequest request_;
  // What we send back to the client.
  //TODO HelloReply reply_;

  // The means to get back to the client.
  //TODO ServerAsyncResponseWriter<HelloReply> responder_;
    // Let's implement a tiny state machine with the following states.
  enum CallStatus
  {
    CREATE, PROCESS, FINISH
  };
  CallStatus status_;  // The current serving state.
  // The producer-consumer queue where for asynchronous server notifications.
  grpc::ServerCompletionQueue *cq_ = nullptr;
  // Context for the rpc, allowing to tweak aspects of it such as the use
  // of compression, authentication, as well as to send metadata back to the
  // client.
  grpc::ServerContext ctx_;
};
#endif

template <typename T>
class GrpcServer : BOF::BofThread
{
public:
  GrpcServer(const GRPC_SERVER_PARAM<T> &_rGrpcServerParam_X)
  {
    mGrpcServerParam_X = _rGrpcServerParam_X;
  }
  ~GrpcServer()
  {
    Stop();
  }

  BOFERR Run()
  {
    BOFERR Rts_E;

    Rts_E = LaunchBofProcessingThread(mGrpcServerParam_X.ServerThreadParam_X.Name_S, false, 0, mGrpcServerParam_X.ServerThreadParam_X.ThreadSchedulerPolicy_E, mGrpcServerParam_X.ServerThreadParam_X.ThreadPriority_E, mGrpcServerParam_X.ServerThreadParam_X.ThreadCpuCoreAffinityMask_U64, 2000, 0);
    return Rts_E;
  }
  BOFERR Stop()
  {
    BOFERR Rts_E;

    if (mpuServer)
    {
      mpuServer->Shutdown();
    }
#if defined(ASYNC)
    if (mGrpcServerParam_X.Async_B)
    {
      cq_->Shutdown(); // Always shutdown the completion queue after the server.
    }
#endif
    Rts_E = DestroyBofProcessingThread(nullptr);
    return Rts_E;
  }
  /*
  BOFERR AddClient(GRPC_LWS *_pLws_X, GRPC_DATA_PER_SESSION *_pWebSocketDataPerSession_X);
  BOFERR RemoveClient(GRPC_LWS *_pLws_X, GRPC_DATA_PER_SESSION *_pWebSocketDataPerSession_X);
  uint32_t GetNumberOfClient();

  virtual BOFERR V_ReadData(uint32_t _TimeoutInMs_U32, uint32_t &_rNb_U32, uint8_t *_pBuffer_U8) = 0;
  virtual BOFERR V_HttpCallback(GRPC_LWS_CALLBACK_REASON _Reason_E, const BOF::BOF_BUFFER &_rPayload_X, BOF::BOF_BUFFER &_rReply_X) = 0;
  virtual BOFERR V_WsCallback(GRPC_LWS_CALLBACK_REASON _Reason_E, const BOF::BOF_BUFFER &_rPayload_X, BOF::BOF_BUFFER &_rReply_X) = 0;
  */
  BOFERR V_OnProcessing() override
  {
    BOFERR Rts_E = BOF_ERR_EINVAL;
    // Build server
    grpc::ServerBuilder builder;
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(mGrpcServerParam_X.Ip_X.ToString(false, true).c_str(), grpc::InsecureServerCredentials());
#if defined(ASYNC)
    if (mGrpcServerParam_X.Async_B)
    {
      // Register "_asyncservice" as the instance through which we'll communicate with
      // clients. In this case it corresponds to an *asynchronous* service.
      builder.RegisterService(mGrpcServerParam_X.pService->mpAsyncService);
      // Get hold of the completion queue used for the asynchronous communication
       // with the gRPC runtime.
      cq_ = builder.AddCompletionQueue();
    }
    else
#endif
    {
      // Register "service" as the instance through which we'll communicate with
      // clients. In this case it corresponds to an *synchronous* service
      builder.RegisterService(mGrpcServerParam_X.pService);
    }
#if defined(ADD_INTERCEPTOR)
    //Add interceptor
    std::vector<std::unique_ptr<grpc::experimental::ServerInterceptorFactoryInterface>> InterceptorCollection;
    std::unique_ptr<grpc::experimental::ServerInterceptorFactoryInterface> Interceptor = std::make_unique<GrpcInterceptorFactory>();
    InterceptorCollection.push_back(std::move(Interceptor));
    builder.experimental().SetInterceptorCreators(std::move(InterceptorCollection));
#endif
    // Finally assemble the server.
    mpuServer = builder.BuildAndStart();

    // Run server
    GRPC_DBG_LOG("Server listening on %s\n", mGrpcServerParam_X.Ip_X.ToString(true, true).c_str());
#if defined(ASYNC)
    if (mGrpcServerParam_X.Async_B)
    {
      HandleRpc();
    }
    else
#endif
    {
      // Wait for the server to shutdown. Note that some other thread must be
      // responsible for shutting down the server for this call to ever return.
      mpuServer->Wait();
    }
    Rts_E = BOF_ERR_EXIT_THREAD;

    GRPC_DBG_LOG("GrpcServer thread stopped...\n");
    return Rts_E;
  }
private:
#if defined(ASYNC)
  void HandleRpc()
  {
    // Spawn a new CallData instance to serve new clients.
    new AsyncCallStateMachine<T>(mGrpcServerParam_X.pService, cq_.get());   //Delete is made by CallData itself
    void *tag; // uniquely identifies a request.
    bool ok;
    while (true)
    {
      // Block waiting to read the next event from the completion queue. The
      // event is uniquely identified by its tag, which in this case is the
      // memory address of a CallData instance.
      // The return value of Next should always be checked. This return value
      // tells us whether there is any kind of event or cq_ is shutting down.
      if (cq_->Next(&tag, &ok) && ok)
      {
        static_cast<AsyncCallStateMachine<T> *>(tag)->Proceed();
      }
      else
      {
        std::cerr << "Something went wrong" << std::endl;
        abort();
      }
    }
  }
#endif

#if 0
  BOFERR GrpcServer::AddClient(WEB_SOCKET_LWS *_pLws_X, GRPC_DATA_PER_SESSION *_pWebSocketDataPerSession_X)
  {
    BOFERR Rts_E = BOF_ERR_EINVAL;
    if ((_pLws_X) && (_pWebSocketDataPerSession_X))
    {
      std::lock_guard<std::mutex> Lock(mMtxClientCollection);
      auto It = std::find(mClientCollection.begin(), mClientCollection.end(), _pWebSocketDataPerSession_X);
      if (It == mClientCollection.end())
      {
        mClientCollection.push_back(_pWebSocketDataPerSession_X);
        Rts_E = BOF_ERR_NO_ERROR;
      }
      else
      {
        Rts_E = BOF_ERR_ALREADY_OPENED;
      }
    }
    return Rts_E;
  }
  BOFERR GrpcServer::RemoveClient(WEB_SOCKET_LWS *_pLws_X, GRPC_DATA_PER_SESSION *_pWebSocketDataPerSession_X)
  {
    BOFERR Rts_E = BOF_ERR_EINVAL;
    if ((_pLws_X) && (_pWebSocketDataPerSession_X))
    {
      std::lock_guard<std::mutex> Lock(mMtxClientCollection);
      auto It = std::find(mClientCollection.begin(), mClientCollection.end(), _pWebSocketDataPerSession_X);
      if (It == mClientCollection.end())
      {
        Rts_E = BOF_ERR_ALREADY_CLOSED;
      }
      else
      {
        mClientCollection.erase(It);
        Rts_E = BOF_ERR_NO_ERROR;
      }
    }
    return Rts_E;
  }

  uint32_t GrpcServer::GetNumberOfClient()
  {
    return mClientCollection.size();
  }
#endif

private:
  GRPC_SERVER_PARAM<T> mGrpcServerParam_X;
  std::mutex mMtxClientCollection;
  std::vector<GRPC_DATA_PER_SESSION<T> *> mClientCollection;

  grpc::ServerContext mServerContext;
  std::unique_ptr<grpc::ServerCompletionQueue> mpuServerCompletionQueue;
  std::unique_ptr<grpc::Server> mpuServer;
};

struct GRPC_CLIENT_PARAM
{
  BOF::BOF_SOCKET_ADDRESS Ip_X;
#if defined(ASYNC)
  bool Async_B;
#endif
  GRPC_CLIENT_PARAM()
  {
    Reset();
  }
  void Reset()
  {
    Ip_X.Reset();
#if defined(ASYNC)
    Async_B = false;
#endif
  }
};

class GrpcClient 
{
public:
  GrpcClient(const GRPC_CLIENT_PARAM &_rGrpcClientParam_X)
  {
    mGrpcClientParam_X = _rGrpcClientParam_X;
    mpsGrpcChannel = grpc::CreateChannel(mGrpcClientParam_X.Ip_X.ToString(false, true).c_str(), grpc::InsecureChannelCredentials());
  }

private:
  GRPC_CLIENT_PARAM mGrpcClientParam_X;
protected:
  std::shared_ptr<grpc::Channel> mpsGrpcChannel;
};


END_WEBRPC_NAMESPACE()

#if 0

The gRPC C++ library provides two ways to implement asynchronous callbacks :

The CompletionQueue API is the traditional way to implement asynchronous callbacks in gRPC.It involves creating a CompletionQueue object, registering callbacks with the CompletionQueue, and then polling the CompletionQueue for completion events.
The Callback API is a newer API that is designed to be easier to use than the CompletionQueue API.It involves providing a callback function to the gRPC library, and the library will call this function when the RPC is complete.
Here is an example of how to implement an asynchronous callback server in C++ using the CompletionQueue API :

C++
class GreeterServiceImpl final : public Greeter::Service
{
public:
  Status SayHello(grpc::ServerContext *context, const HelloRequest *request,
                  HelloReply *reply) override
  {
    // Create a CompletionQueue object.
    grpc::CompletionQueue cq;

    // Register a callback with the CompletionQueue.
    auto f = [this, cq, reply](grpc::Status status) {
      if (status.ok())
      {
        reply->set_message("Hello, " + request->name());
      }
      else
      {
        std::cerr << "RPC failed: " << status.error_message() << std::endl;
      }
    };
    grpc::CompletionHandler<HelloReply> handler(f, cq);

    // Make the RPC call.
    grpc::Status status = stub_->SayHello(context, request, &handler);

    // Block until the RPC is complete.
    cq.Run();

    return status;
  }

private:
  std::unique_ptr<Greeter::Stub> stub_;
};
Use code with caution.Learn more
Here is an example of how to implement an asynchronous callback client in C++ using the Callback API :

C++
void SayHello(const std::string & name)
{
  // Create a channel to the server.
  grpc::Channel *channel = grpc::CreateChannel("localhost:50051",
                                               grpc::InsecureChannelCredentials());

  // Create a stub for the Greeter service.
  Greeter::Stub stub(channel);

  // Create a callback object.
  struct Callback
  {
    void OnCompleted(const HelloReply &reply)
    {
      std::cout << "Greeter received: " << reply.message() << std::endl;
    }
  };
  Callback callback;

  // Make the RPC call.
  stub.AsyncSayHello(name, &callback);
}
#endif
