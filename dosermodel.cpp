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
	for (int x = 0; x < image.width(); ++x)
	{
		for (int y = 0; y < image.height(); ++y)
		{
			nodes.push_back(qMakePair(QPoint(x, y), initialWeight));
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
	for (QPair<QPoint, double> node : nodes)
	{
		if (node.second > initialWeight)
		{
			segment.push_back(node.first);
		}
	}

	emit deepSegmentChanged(segment);
	isSegmenting = false;
}

double DoserModel::product(NodeVector v1, QVector<double> v2) const
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

double DoserModel::weight(QPoint px1, QPoint px2) const
{
	// todo: HSV
	auto toGrayscale = [](const QImage& image, QPoint px) -> double
	{
		return qGray(image.pixel(px)) / 255.0;
	};

	double colorDiff = toGrayscale(image, px1) - toGrayscale(image, px2);
	double distance = qPow(colorDiff, 2);

	return qExp(-distance / WEIGHT_RATIO);
}

double DoserModel::distance(NodeVector v1, NodeVector v2) const
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

	return qPow(sumOfSquares, 0.5);
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
