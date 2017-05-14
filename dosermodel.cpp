#include "dosermodel.h"
#include <algorithm>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>
#include <QtMath>
#include <QVector>

DoserModel::DoserModel()
{
	qRegisterMetaType<Segment>("DoserModel::Segment");
	qRegisterMetaType<QVector<Segment>>("QVector<DoserModel::Segment>");
}

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
	weightedSegments.clear();
	pendingPixels.clear();

	int pixelCount = image.width() * image.height();
	double initialWeight = 1.0 / pixelCount;

	QVector<Node> nodes;
	nodes.reserve(pixelCount);
	for (int y = 0; y < image.height(); ++y)
	{
		for (int x = 0; x < image.width(); ++x)
		{
			nodes.append(qMakePair(Pixel(x, y), initialWeight));
		}
	}

	int segmentedPixelCount = 0;
	int targetPixelCount = TARGET_SEGMENTATION_RATIO * pixelCount;
	while (segmentedPixelCount < targetPixelCount)
	{
		double dist;
		do
		{
			QVector<Node> prevNodes(nodes);
			iterate(nodes);
			dist = distance(nodes, prevNodes);
		} while (dist > ITERATION_PRECISION);

		WeightedSegment weightedSegment;
		QVector<Node> newNodes;

		for (const Node& node : nodes)
		{
			if (node.second > initialWeight)
			{
				weightedSegment.append(node);
			}
			else
			{
				newNodes.append(node);
			}
		}

		if (weightedSegment.empty()) // iff nodes is atomic
		{
			weightedSegment = nodes;
			nodes.clear();
		}
		else
		{
			nodes = newNodes;
			initialWeight = 1.0 / nodes.size();
			for (int i = 0; i < nodes.size(); ++i)
			{
				nodes[i].second = initialWeight;
			}
		}

		if (weightedSegment.size() < MINIMAL_SEGMENT_SIZE)
		{
			for (const QPair<Pixel, double>& weightedPixel : weightedSegment)
			{
				// intentionally not Node
				pendingPixels[mode].append(weightedPixel.first);
			}
		}
		else
		{
			double sumOfWeights = 0.0;
			for (const QPair<Pixel, double>& weightedPixel : weightedSegment)
			{
				sumOfWeights += weightedPixel.second;
			}

			for (int i = 0; i < weightedSegment.size(); ++i)
			{
				weightedSegment[i].second /= sumOfWeights;
			}

			weightedSegments[mode].append(weightedSegment);
			emit deepSegmentChanged(toSegment(weightedSegment));
		}

		segmentedPixelCount += weightedSegment.size();
		emit segmentationProgress(segmentedPixelCount, pixelCount);
	}

	if (!nodes.empty())
	{
		for (int i = 0; i < nodes.size(); ++i)
		{
			pendingPixels[mode].append(nodes[i].first);
		}
	}

	mergePendingPixels(mode);
	isSegmenting = false;

	QVector<Segment> segments(weightedSegments[mode].size());
	for (int i = 0; i < segments.size(); ++i)
	{
		segments[i] = toSegment(weightedSegments[mode][i]);
	}

	emit segmentationFinished(segments);
}

double DoserModel::product(const QVector<Node>& v1, const QVector<double>& v2) const
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

double DoserModel::weight(const Pixel &px1, const Pixel &px2) const
{
	const QRgb& rgb1 = image.pixel(px1);
	const QRgb& rgb2 = image.pixel(px2);

	double squareSum;
	if (isGrayscale || FORCE_GRAYSCALE)
	{
		squareSum = qPow((qGray(rgb1) - qGray(rgb2)) / 255.0, 2);
	}
	else
	{
		const QColor& hsv1 = QColor(rgb1).toHsv();
		const QColor& hsv2 = QColor(rgb2).toHsv();

		double h1 = hsv1.hueF(), v1 = hsv1.valueF();
		double h2 = hsv2.hueF(), v2 = hsv2.valueF();
		double vs1 = v1 * hsv1.saturationF(), vs2 = v2 * hsv2.saturationF();

		squareSum = qPow(v1 - v2, 2);
		squareSum += qPow(vs1 * qSin(h1) - vs2 * qSin(h2), 2);
		squareSum += qPow(vs1 * qCos(h1) - vs2 * qCos(h2), 2);
	}

	return qExp(-squareSum / WEIGHT_RATIO_SQUARE);
}

double DoserModel::distance(const QVector<Node>& v1, const QVector<Node>& v2) const
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

void DoserModel::iterate(QVector<Node>& races)
{
	int raceCount = races.size();
	QVector<QFuture<QPair<int, double>>> futures(raceCount);

	for (int i = 0; i < raceCount; ++i)
	{
		const auto& calculateIthFitness = [=]()
		{
			double fitness = 0;
			for (const Node& rival : races)
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

void DoserModel::mergePendingPixels(SegmentationMode mode)
{
	for (const Pixel& pixel : pendingPixels[mode])
	{
		const auto& similarity = [&](const WeightedSegment& s1, const WeightedSegment& s2)
		{
			return inducedWeight(s1, pixel) < inducedWeight(s2, pixel);
		};

		auto mostSimilarSegment = std::max_element(weightedSegments[mode].begin(),
			weightedSegments[mode].end(), similarity);
		mostSimilarSegment->append(qMakePair(pixel, 0.0));
	}
}

double DoserModel::inducedWeight(const WeightedSegment& weightedSegment, const Pixel& externalPixel) const
{
	double inducedWeight = 0;
	const Pixel& basisPixel = weightedSegment.first().first;

	for (const QPair<Pixel, double>& weightedPixel : weightedSegment)
	{
		inducedWeight += weightedPixel.second * (weight(weightedPixel.first, externalPixel)
			- weight(weightedPixel.first, basisPixel));
	}

	return inducedWeight;
}

DoserModel::Segment DoserModel::toSegment(const WeightedSegment &weightedSegment) const
{
	Segment segment(weightedSegment.size());
	for (int i = 0; i < weightedSegment.size(); ++i)
	{
		segment[i] = weightedSegment[i].first;
	}

	return segment;
}
