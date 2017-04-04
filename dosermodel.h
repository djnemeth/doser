#ifndef DOSERMODEL_H
#define DOSERMODEL_H

#include <QImage>
#include <QObject>
#include <QPair>
#include <QPoint>
#include <QString>

enum SegmentationMode
{
	QUICK_MODE, DEEP_MODE, BOTH_MODE
};

class DoserModel : public QObject
{
	Q_OBJECT

signals:
	void imageChanged(const QImage&);
	void deepSegmentChanged(QVector<QPoint>);

public slots:
	void openImage(const QString& path);
	void segment(SegmentationMode mode);

private:
	typedef QVector<QPair<QPoint, double>> NodeVector;
	static constexpr double ITERATION_PRECISION = 0.01;
	static constexpr double WEIGHT_RATIO = 0.1;

	double weight(QPoint px1, QPoint px2) const;
	double distance(NodeVector v1, NodeVector v2) const;
	double product(NodeVector v1, QVector<double> v2) const;
	void iterate(NodeVector& nodes);

	QImage image;
	bool isSegmenting = false;
};

#endif // DOSERMODEL_H
