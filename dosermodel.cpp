#include "dosermodel.h"
#include <QFuture>
#include <QPoint>
#include <QtConcurrent/QtConcurrent>
#include <QtMath>
#include <QVector>

void DoserModel::openImage(const QString& path)
{
	if (isSegmenting)
	{
		throw;
	}

	QImage newImage(path);
	if (!newImage.isNull())
	{
		image = newImage;
		isGrayscale = image.isGrayscale(); // expensive call
		emit imageChanged(image);
	}
}

void DoserModel::segment(SegmentationMode mode)
{
	if (isSegmenting || image.isNull() || mode != DEEP_MODE)
	{
		throw;
	}

	isSegmenting = true;
	int pixelCount = image.width() * image.height();
	double initialWeight = 1.0 / pixelCount;

	NodeVector nodes;
	nodes.reserve(pixelCount);
	for (int y = 0; y < image.height(); ++y)
	{
		for (int x = 0; x < image.width(); ++x)
		{
			nodes.append(qMakePair(QPoint(x, y), initialWeight));
		}
	}

	double dist;
	do
	{
		NodeVector prevNodes(nodes);
		iterate(nodes);
		dist = distance(nodes, prevNodes);
	} while (dist > ITERATION_PRECISION);

	QVector<QPoint> segment;
	for (const QPair<QPoint, double>& node : nodes)
	{
		if (node.second > initialWeight)
		{
			segment.append(node.first);
		}
	}

	emit deepSegmentChanged(segment);
	isSegmenting = false;
}

double DoserModel::product(const NodeVector& v1, const QVector<double>& v2) const
{
	if (v1.size() != v2.size())
	{
		throw;
	}

	double product = 0;
	for (int i = 0; i < v1.size(); ++i)
	{
		product += v1[i].second * v2[i];
	}

	return product;
}

double DoserModel::weight(const QPoint& px1, const QPoint& px2) const
{
	QRgb rgb1 = image.pixel(px1);
	QRgb rgb2 = image.pixel(px2);

	double squareSum;
	if (isGrayscale || FORCE_GRAYSCALE)
	{
		squareSum = qPow((qGray(rgb1) - qGray(rgb2)) / 255.0, 2);
	}
	else
	{
		QColor hsv1 = QColor(rgb1).toHsv();
		QColor hsv2 = QColor(rgb2).toHsv();

		double h1 = hsv1.hueF(), v1 = hsv1.valueF();
		double h2 = hsv2.hueF(), v2 = hsv2.valueF();
		double vs1 = v1 * hsv1.saturationF(), vs2 = v2 * hsv2.saturationF();

		squareSum = qPow(v1 - v2, 2);
		squareSum += qPow(vs1 * qSin(h1) - vs2 * qSin(h2), 2);
		squareSum += qPow(vs1 * qCos(h1) - vs2 * qCos(h2), 2);
	}

	return qExp(-squareSum / WEIGHT_RATIO_SQUARE);
}

double DoserModel::distance(const NodeVector& v1, const NodeVector& v2) const
{
	if (v1.size() != v2.size())
	{
		throw;
	}

	double sumOfSquares = 0;
	for (int i = 0; i < v1.size(); ++i)
	{
		sumOfSquares += qPow(v1[i].second - v2[i].second, 2);
	}

	return qSqrt(sumOfSquares);
}

void DoserModel::iterate(NodeVector& races)
{
	int raceCount = races.size();
	QVector<QFuture<QPair<int, double>>> futures(raceCount);

	for (int i = 0; i < raceCount; ++i)
	{
		auto calculateIthFitness = [=]()
		{
			double fitness = 0;
			for (const QPair<QPoint, double>& rival : races)
			{
				 fitness += rival.second * weight(races[i].first, rival.first);
			}

			return qMakePair(i, fitness);
		};

		futures[i] = QtConcurrent::run(calculateIthFitness);
	}

	QVector<double> fitnesses(raceCount, 0);
	for (int i = 0; i < raceCount; ++i)
	{
		const QPair<int, double>& fitness = futures[i].result();
		fitnesses[fitness.first] = fitness.second;

		emit iterationProgress(i + 1, raceCount);
	}

	double overallFitness = product(races, fitnesses);
	for (int i = 0; i < raceCount; ++i)
	{
		races[i].second *= fitnesses[i] / overallFitness;
	}
}
