#ifndef ADASCCRIPT_H
#define ADASCCRIPT_H

#include <QObject>
#include <libneoada/runtime.h>

class AdaSccript : public QObject
{
public:
    AdaSccript();
    virtual ~AdaSccript();

private:
    NdaRuntime *mAda;
};

#endif // ADASCCRIPT_H
