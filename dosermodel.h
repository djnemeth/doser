#ifndef DOSERMODEL_H
#define DOSERMODEL_H

#include <QImage>
#include <QObject>
#include <QString>

class DoserModel : public QObject
{
	Q_OBJECT

public:
	DoserModel();
	void openImage(QString path);

signals:
	void imageChanged(const QImage&);

private:
	QImage image;
};

#endif // DOSERMODEL_H
