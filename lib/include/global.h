#pragma clang diagnostic push
#pragma ide diagnostic ignored "bugprone-macro-parentheses"
//
// Created by Yoshiki on 2023/6/2.
//

#ifndef LINUX_SERVER_LIB_INCLUDE_GLOBAL_H_
#define LINUX_SERVER_LIB_INCLUDE_GLOBAL_H_

#ifdef __cplusplus
#include <cerrno>
#else
#include <errno.h>
#endif
#define CHECK_RET(expr, fnName)                                                 \
            if (expr)                                                           \
            {                                                                   \
                fprintf(stderr,"%s:%s:%d:  "#fnName" err %s\n",                 \
                         __FILE__, __FUNCTION__, __LINE__, strerror(errno));    \
                exit(EXIT_FAILURE);                                             \
            }

#ifdef __cplusplus

#include <memory>
template <class T>
inline T *GetPtrHelper (T *d) { return d; }
template <class T>
inline auto GetPtrHelper (const T &d) -> decltype(d.operator->()) { return d.operator->(); }

#define DECLARE_PTR_Q(Class) Class *q_ptr
#define DECLARE_PTR_D(Class) std::unique_ptr<Class##Private> d_ptr
#define DECLARE_TEMPLATE_PTR_Q(Class, T) Class<T> *q_ptr
#define DECLARE_TEMPLATE_PTR_D(Class, T) std::unique_ptr<Class##Private<T>> d_ptr

#define DECLARE_PRIVATE_D(Class) \
    inline Class##Private *d_fun() { return reinterpret_cast<Class##Private *>(GetPtrHelper(d_ptr)); } \
    inline const Class##Private *d_fun() const { return reinterpret_cast<const Class##Private *>(GetPtrHelper(d_ptr)); } \
    friend class Class##Private

#define DECLARE_TEMPLATE_PRIVATE_D(Class, T) \
    inline Class##Private<T> *d_fun() { return reinterpret_cast<Class##Private<T> *>(GetPtrHelper(this->d_ptr)); } \
    inline const Class##Private<T> *d_fun() const { return reinterpret_cast<const Class##Private<T> *>(GetPtrHelper(this->d_ptr)); } \
    friend class Class##Private<T>

#define D_PTR(Class) Class##Private *const d = d_fun()
#define D_TEMPLATE_PTR(Class, T) Class##Private<T> *const d = d_fun()

#define DECLARE_PUBLIC_Q(Class) \
    inline Class *q_fun() { return reinterpret_cast<Class *>(GetPtrHelper(q_ptr)); } \
    inline const Class *q_fun() const { return reinterpret_cast<Class *>(GetPtrHelper(q_ptr)); } \
    friend class Class
#define DECLARE_TEMPLATE_PUBLIC_Q(Class, T) \
    inline Class<T> *q_fun() { return reinterpret_cast<Class<T> *>(GetPtrHelper(this->q_ptr)); } \
    inline const Class<T> *q_fun() const { return reinterpret_cast<Class<T> *>(GetPtrHelper(this->q_ptr)); } \
    friend class Class<T>

#define Q_PTR(Class) Class *const q = q_fun()
#define Q_TEMPLATE_PTR(Class) Class<T> *const q = q_fun()

#define CLASS_IS_VALID(Class, exp) inline bool isValid () const noexcept override { \
                                    const D_PTR(Class);                        \
                                    return ((typeid(*d) == typeid(Class##Private)) && d->isValid && exp); \
                              }

#define CLASS_TEMPLATE_IS_VALID(Class, exp, T) inline bool isValid () const noexcept override { \
                                    const D_TEMPLATE_PTR(Class, T);                        \
                                    return ((typeid(*d) == typeid(Class##Private<T>)) && d->isValid && exp); \
                              }

#define CLASS_DEFAULT_COPY_CONSTRUCTOR(Class, Parent) \
    Class(const Class &rhs) : Parent(nullptr) { this->operator=(rhs); } \
    Class &operator=(const Class &rhs) { if (this == &rhs) return *this;    \
                                        this->d_ptr = std::unique_ptr <Class##Private>(new Class##Private(*rhs.d_fun())); \
                                        return *this; }                 \

#define CLASS_DEFAULT_MOVE_COPY_CONSTRUCTOR(Class, Parent) \
    Class(Class &&rhs) noexcept : Parent(nullptr) { this->operator=(std::move(rhs)); } \
    Class &operator=(Class &&rhs) noexcept { if (this == &rhs) return *this; \
                                            this->d_ptr = std::move(rhs.d_ptr); \
                                            return *this; }

#define CLASS_DEFAULT_ALL_COPY_CONSTRUCTOR(Class, Parent) \
    CLASS_DEFAULT_COPY_CONSTRUCTOR(Class, Parent)         \
    CLASS_DEFAULT_MOVE_COPY_CONSTRUCTOR(Class, Parent)

#define CLASS_COPY_CONSTRUCTOR_DISABLED(Class) \
    Class(const Class &) = delete;             \
    Class &operator=(const Class &) = delete;

class Base;
class BasePrivate
{
public:
    BasePrivate ()
        : q_ptr(nullptr) {};
    virtual ~BasePrivate () noexcept = default;

protected:
    explicit BasePrivate (Base *parent)
        : q_ptr(parent) {}

    DECLARE_PUBLIC_Q(Base);
    DECLARE_PTR_Q(Base);

    bool isValid = true;
};
class Base
{
public:
    Base ()
        : d_ptr(nullptr) {};

    virtual ~Base () noexcept = default;

    virtual inline bool isValid () const noexcept = 0;

protected:
    explicit Base (BasePrivate *d)
        : d_ptr(d) {}
    DECLARE_PRIVATE_D(Base);
    DECLARE_PTR_D(Base);
};

template <class T>
class BaseTemplate;
template <class T>
class BaseTemplatePrivate
{
public:
    BaseTemplatePrivate ()
        : q_ptr(nullptr) {};
    virtual ~BaseTemplatePrivate () noexcept = default;

protected:
    explicit BaseTemplatePrivate (BaseTemplate <T> *parent)
        : q_ptr(parent) {}

    DECLARE_TEMPLATE_PUBLIC_Q(BaseTemplate, T);
    DECLARE_TEMPLATE_PTR_Q(BaseTemplate, T);

    bool isValid = true;
};
template <class T>
class BaseTemplate
{
public:
    BaseTemplate ()
        : d_ptr(nullptr) {};

    virtual ~BaseTemplate () noexcept = default;

    virtual inline bool isValid () const noexcept = 0;

protected:
    explicit BaseTemplate (BaseTemplatePrivate <T> *d)
        : d_ptr(d) {}
    DECLARE_TEMPLATE_PRIVATE_D(BaseTemplate, T);
    DECLARE_TEMPLATE_PTR_D(BaseTemplate, T);
};

#endif
#endif //LINUX_SERVER_LIB_INCLUDE_GLOBAL_H_

#pragma clang diagnostic pop