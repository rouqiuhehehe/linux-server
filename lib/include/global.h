#pragma clang diagnostic push
#pragma ide diagnostic ignored "bugprone-macro-parentheses"
//
// Created by Yoshiki on 2023/6/2.
//

#ifndef LINUX_SERVER_LIB_INCLUDE_GLOBAL_H_
#define LINUX_SERVER_LIB_INCLUDE_GLOBAL_H_

#include "util.h"
#ifdef __cplusplus

#include <memory>
template <class T>
inline T *GetPtrHelper (T *d) { return d; }
template <class T>
inline auto GetPtrHelper (const T &d) -> decltype(d.operator->()) { return d.operator->(); }

#define DECLARE_PTR_Q(Class) Class *q_ptr
#define DECLARE_PTR_D(Class) std::unique_ptr<Class##Private> d_ptr

#define DECLARE_PRIVATE_D(Class) \
    inline Class##Private *d_fun() { return reinterpret_cast<Class##Private *>(GetPtrHelper(d_ptr)); } \
    inline const Class##Private *d_fun() const { return reinterpret_cast<const Class##Private *>(GetPtrHelper(d_ptr)); } \
    friend class Class##Private  \

#define D_PTR(Class) Class##Private *const d = d_fun()

#define DECLARE_PUBLIC_Q(Class) \
    inline Class *q_fun() { return reinterpret_cast<Class *>(GetPtrHelper(q_ptr)); } \
    inline const Class *q_fun() const { return reinterpret_cast<Class *>(GetPtrHelper(q_ptr)); } \
    friend class Class  \

#define Q_PTR(Class) Class *const q = q_fun()

class Base;
class BasePrivate
{
public:
    explicit BasePrivate (Base *parent)
        : q_ptr(parent) {}
    virtual ~BasePrivate () noexcept = default;

protected:
    DECLARE_PUBLIC_Q(Base);
    DECLARE_PTR_Q(Base);
};
class Base
{
public:
    Base ()
        : d_ptr(new BasePrivate(this)) {}
    virtual ~Base () noexcept = default;

protected:
    explicit Base (BasePrivate *d)
        : d_ptr(d) {}
    DECLARE_PRIVATE_D(Base);
    DECLARE_PTR_D(Base);
};

#endif
#endif //LINUX_SERVER_LIB_INCLUDE_GLOBAL_H_

#pragma clang diagnostic pop