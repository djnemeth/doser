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

signals:
	void imageChanged(const QImage&);

public slots:
	void openImage(const QString& path);

private:
	QImage image;
};

#endif // DOSERMODEL_H
