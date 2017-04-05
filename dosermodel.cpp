#include "dosermodel.h"
#include <QtMath>
#include <QPoint>
#include <QVector>

void DoserModel::openImage(const QString& path)
{
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
	segment.reserve(nodes.size());
	for (QPair<QPoint, double> node : nodes)
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

	double prod = 0;
	for (int i = 0; i < v1.size(); ++i)
	{
		prod += v1[i].second * v2[i];
	}

	return prod;
}

double DoserModel::weight(const QPoint& px1, const QPoint& px2) const
{
	// todo: HSV
	static auto toGrayscale = [](const QImage& image, const QPoint& px) -> double
	{
		return qGray(image.pixel(px)) / 255.0;
	};

	double colorDiff = toGrayscale(image, px1) - toGrayscale(image, px2);
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

void DoserModel::iterate(NodeVector& nodes)
{
	QVector<double> fitnesses(nodes.size(), 0);
	for (int i = 0; i < nodes.size(); ++i)
	{
		for (int j = 0; j < nodes.size(); ++j)
		{
			fitnesses[i] += nodes[j].second * weight(nodes[i].first, nodes[j].first);
		}
	}

	double overallFitness = product(nodes, fitnesses);
	for (int i = 0; i < nodes.size(); ++i)
	{
		nodes[i].second *= fitnesses[i] / overallFitness;
	}
}
