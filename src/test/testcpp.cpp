//
// Created by Yoshiki on 2023/6/2.
//

#include "util/global.h"
#include "util/util.h"
#include <iostream>

class Widget;
class WidgetPrivate : public BasePrivate
{
public:
    WidgetPrivate (int w, Widget *parent);
    ~WidgetPrivate () noexcept override = default;
protected:
    DECLARE_PUBLIC_Q(Widget);
private:
    int width;
};
class Widget : public Base
{
    DECLARE_PRIVATE_D(Widget);
public:
    explicit Widget (int w)
        : Base(new WidgetPrivate(w, this)) {};
    ~Widget () noexcept override = default;

    CLASS_IS_VALID(Widget, true)

    int width () const
    {
        const D_PTR(Widget);
        return d->width;
    }
protected:
    explicit Widget (WidgetPrivate *d)
        : Base(d) {}
};
WidgetPrivate::WidgetPrivate (int w, Widget *parent)
    : width(w), BasePrivate(parent) {}

class Label;
class LabelPrivate : public WidgetPrivate
{
    DECLARE_PUBLIC_Q(Label);
public:
    LabelPrivate (std::string title, int w, Label *parent);
    ~LabelPrivate () noexcept override = default;

    int a () const noexcept;
private:
    std::string title;
};

class Label final : public Widget
{
    DECLARE_PRIVATE_D(Label);
public:
    Label (const std::string &title, int w)
        : Widget(new LabelPrivate(title, w, this)) {}
    ~Label () noexcept final = default;

    CLASS_IS_VALID(Label, true)

    const std::string &title () const
    {
        const D_PTR(Label);

        d->a();
        return d->title;
    }
};

LabelPrivate::LabelPrivate (std::string title, int w, Label *parent)
    : title(std::move(title)), WidgetPrivate(w, parent) {}
int LabelPrivate::a () const noexcept
{
    const Q_PTR(Label);
    std::cout << "test q : " << q->width() << std::endl;

    return 0;
}

int main ()
{
    Label label("顶顶顶顶顶顶顶顶顶顶顶", 100);
    std::cout << "title: " << label.title() << "\twidth: " << label.width() << std::endl;

    std::cout << "address : " << Utils::addressOf(label) << std::endl;
    return 0;
}