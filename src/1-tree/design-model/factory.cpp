//
// Created by Administrator on 2023/4/25.
//

#include <iostream>
#include <string>

class IExport
{
public:
    virtual ~IExport () = default;
    virtual bool iExport (const std::string &) = 0;
};

class IExportFactory
{
public:
    virtual ~IExportFactory ()
    {
        delete export_;
        export_ = nullptr;
    }
    bool iExport (const std::string &data)
    {
        if (!export_)
            export_ = setExport();

        return export_->iExport(data);
    }
protected:
    virtual IExport *setExport () = 0;
private:
    IExport *export_ = nullptr;
};

class ExportJSON : public IExport
{
public:
    bool iExport (const std::string &data) override
    {
        std::cout << data << std::endl;
        return true;
    }
};

class ExportJSONFactory : public IExportFactory
{
protected:
    IExport *setExport () override
    {
        auto ptr = new ExportJSON;
        return ptr;
    }
};

int main ()
{
    auto ptr = new ExportJSONFactory;
    ptr->iExport("狗蛋");

    delete ptr;
    return 0;
}