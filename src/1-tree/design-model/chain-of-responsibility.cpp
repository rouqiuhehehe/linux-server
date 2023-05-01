//
// Created by Administrator on 2023/4/25.
//
#include <string>
#include <iostream>

class Context
{
public:
    std::string name;
    int day;
};

class IHandler
{
public:
    virtual ~IHandler () { next = nullptr; };
    void setNextHandler (IHandler *n) { next = n; };

    static bool handlerLeaveReq (const Context &context);
protected:
    virtual bool canHandle (const Context &context) = 0;
    virtual bool handleRes (const Context &context) = 0;
    IHandler *getNext () const { return next; };
private:
    bool handler (const Context &context)
    {
        if (canHandle(context))
            return handleRes(context);
        else if (getNext())
            return getNext()->handler(context);
        else
            //err
            return false;
    }

    IHandler *next = nullptr;
};
class HandleByMainProgram : public IHandler
{
protected:
    bool canHandle (const Context &context) override
    {
        if (context.day <= 3)
            return true;
        return false;
    }

    bool handleRes (const Context &context) override
    {
        std::cout << "confirm by main program" << std::endl;
        return true;
    }
};

class HandleByProjMgr : public IHandler
{
protected:
    bool canHandle (const Context &context) override
    {
        if (context.day <= 7)
            return true;
        return false;
    }

    bool handleRes (const Context &context) override
    {
        std::cout << "confirm by project manager" << std::endl;
        return true;
    }
};

class HandleByBoss : public IHandler
{
protected:
    bool canHandle (const Context &context) override
    {
        if (context.day <= 30)
            return true;
        return false;
    }

    bool handleRes (const Context &context) override
    {
        std::cout << "confirm by boss" << std::endl;
        return true;
    }
};

bool IHandler::handlerLeaveReq (const Context &context)
{
    HandleByMainProgram h0;
    HandleByProjMgr h1;
    HandleByBoss h2;

    h0.setNextHandler(&h1);
    h1.setNextHandler(&h2);

    return h0.handler(context);
}

int main ()
{
    Context context { "大苏打撒旦", 30 };

    IHandler::handlerLeaveReq(context);

    return 0;
}