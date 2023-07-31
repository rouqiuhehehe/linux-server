//
// Created by Administrator on 2023/4/24.
//
#include <vector>
#include <algorithm>
#include <iostream>
class Interface
{
public:
    explicit Interface (int a)
        : i(a) {};
    bool operator== (const Interface &r) const
    {
        return i == r.i;
    }

    virtual void show () = 0;

protected:
    int i;
};
class Database
{
public:
    typedef std::vector <Interface *> DataType;

    DataType &data () { return data_; };
    void notify ()
    {
        for (auto &v : data_)
        {
            v->show();
        }
    }
private:
    DataType data_;
};
class Observer
{
public:
    Observer ()
        : database_(new Database) {}
    ~Observer ()
    {
        delete database_;
    }
    void attach (Interface *interface)
    {
        database_->data().push_back(interface);
    }
    void detach (Interface *interface)
    {
        database_->data().erase(std::find(database_->data().begin(), database_->data().end(), interface));
    }

    const Database *database () const { return database_; };
    void notify ()
    {
        database_->notify();
    }

private:
    Database *database_;
};

class A : public Interface
{
public:
    explicit A (int a)
        : Interface(a) {};
    void show () override
    {
        std::cout << "A : " << i << std::endl;
    }
};
class AA : public Interface
{
public:
    explicit AA (int a)
        : Interface(a) {};
    void show () override
    {
        std::cout << "AA : " << i << std::endl;
    }
};
class AAA : public Interface
{
public:
    explicit AAA (int a)
        : Interface(a) {};
    void show () override
    {
        std::cout << "AAA : " << i << std::endl;
    }
};
int main ()
{
    auto *observer = new Observer;

    auto *a = new A(10);
    auto *aa = new AA(110);
    auto *aaa = new AAA(1110);

    observer->attach(a);
    observer->attach(aa);
    observer->attach(aaa);

    observer->notify();

    return 0;
}