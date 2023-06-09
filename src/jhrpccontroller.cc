#include "jhrpccontroller.h"

jhrpcController::jhrpcController()
    :ctl_failsym(false),ctl_errmsg("")
{}

void jhrpcController::Reset()
{
    ctl_failsym = false;
    ctl_errmsg = "";
}

bool jhrpcController::Failed() const
{
    return ctl_failsym;
}
std::string jhrpcController::ErrorText() const
{
    return ctl_errmsg;
}

void jhrpcController::SetFailed(const std::string &reason)
{
    ctl_failsym = true;
    ctl_errmsg = reason;
}


// 用不上，也许以后可以看
void jhrpcController::StartCancel(){}
bool jhrpcController::IsCanceled() const {}
void jhrpcController::NotifyOnCancel(google::protobuf::Closure *callback){}