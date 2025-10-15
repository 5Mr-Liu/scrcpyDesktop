#ifndef CONTROLSENDER_H
#define CONTROLSENDER_H

#include <QObject>

class ControlSender : public QObject
{
    Q_OBJECT
public:
    explicit ControlSender(QObject *parent = nullptr);

signals:
};

#endif // CONTROLSENDER_H
