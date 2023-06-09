#pragma once
#include <google/protobuf/service.h>
#include <string>

class jhrpcController : public google::protobuf::RpcController
{
public:
    jhrpcController();
    virtual void Reset();

    virtual bool Failed() const;

    virtual std::string ErrorText() const;

    virtual void SetFailed(const std::string &reason);


    //用不上，也许以后可以看
    virtual void StartCancel();

    virtual bool IsCanceled() const;

    virtual void NotifyOnCancel(google::protobuf::Closure *callback);

private:
    bool ctl_failsym;
    std::string ctl_errmsg;
};