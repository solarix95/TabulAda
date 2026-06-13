#include "view/iconfactory.h"

#include <QApplication>
#include <QPainter>
#include <QPalette>
#include <QPixmap>

namespace {

QPixmap createPixmap(IconFactory::Icon icon, int size)
{
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.scale(size / 16.0, size / 16.0);

    const QColor foreground = QApplication::palette().color(QPalette::ButtonText);
    QPen pen(foreground, 1.5, Qt::SolidLine, Qt::RoundCap);
    painter.setPen(pen);

    if (icon == IconFactory::TextColor) {
        QFont font = painter.font();
        font.setBold(true);
        font.setPixelSize(11);
        painter.setFont(font);
        painter.drawText(QRectF(2, 0, 12, 12), Qt::AlignCenter, QStringLiteral("A"));
        painter.fillRect(QRectF(2, 13, 12, 2), QColor(210, 50, 45));
        return pixmap;
    }

    if (icon == IconFactory::BackgroundColor) {
        painter.setBrush(foreground);
        painter.drawRect(QRectF(4, 2, 7, 7));
        painter.setBrush(QColor(250, 210, 60));
        painter.setPen(Qt::NoPen);
        painter.drawRect(QRectF(2, 11, 12, 4));
        painter.setPen(pen);
        painter.drawLine(QPointF(11, 5), QPointF(14, 8));
        return pixmap;
    }

    if (icon == IconFactory::MergeCells || icon == IconFactory::SplitCells) {
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(QRectF(2, 3, 12, 10));
        if (icon == IconFactory::SplitCells) {
            painter.drawLine(QPointF(8, 3), QPointF(8, 13));
            painter.drawLine(QPointF(2, 8), QPointF(14, 8));
        } else {
            painter.drawLine(QPointF(3, 8), QPointF(6, 8));
            painter.drawLine(QPointF(5, 6), QPointF(7, 8));
            painter.drawLine(QPointF(5, 10), QPointF(7, 8));
            painter.drawLine(QPointF(13, 8), QPointF(10, 8));
            painter.drawLine(QPointF(11, 6), QPointF(9, 8));
            painter.drawLine(QPointF(11, 10), QPointF(9, 8));
        }
        return pixmap;
    }

    const qreal widths[] = { 12, 8, 10, 6 };
    for (int row = 0; row < 4; ++row) {
        qreal x = 2;
        if (icon == IconFactory::AlignCenter)
            x = (16 - widths[row]) / 2;
        else if (icon == IconFactory::AlignRight)
            x = 14 - widths[row];
        painter.drawLine(QPointF(x, 3 + row * 3),
                         QPointF(x + widths[row], 3 + row * 3));
    }

    return pixmap;
}

}

QIcon IconFactory::create(Icon icon)
{
    QIcon result;
    const int sizes[] = { 16, 24, 32 };
    for (int size : sizes)
        result.addPixmap(createPixmap(icon, size));
    return result;
}

IconFactory::IconFactory()
{
}
