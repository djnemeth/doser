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
	// todo: HSV
	const auto& toGrayscale = [&](const QPoint& px)
	{
		return qGray(image.pixel(px)) / 255.0;
	};

	double colorDiff = toGrayscale(px1) - toGrayscale(px2);
	double distance = qPow(colorDiff, 2);

	return qExp(-distance / WEIGHT_RATIO);
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
