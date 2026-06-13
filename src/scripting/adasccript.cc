#include "adasccript.h"

AdaSccript::AdaSccript()
    : QObject()
    , mAda(new NdaRuntime())
{
}

AdaSccript::~AdaSccript()
{
    Q_ASSERT(mAda);
    delete mAda;
}
