#ifndef ICONFACTORY_H
#define ICONFACTORY_H

#include <QIcon>

class IconFactory
{
public:
    enum Icon {
        TextColor,
        BackgroundColor,
        AlignLeft,
        AlignCenter,
        AlignRight,
        MergeCells,
        SplitCells
    };

    static QIcon create(Icon icon);

private:
    IconFactory();
};

#endif // ICONFACTORY_H
