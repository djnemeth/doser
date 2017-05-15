#include "dosermodel.h"
#include <algorithm>
#include <cstdlib>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>
#include <QTime>
#include <QtMath>
#include <QVector>
#include <QDebug>

DoserModel::DoserModel()
{
	qRegisterMetaType<SegmentationMode>("DoserModel::SegmentationMode");
	qRegisterMetaType<Segment>("DoserModel::Segment");
	qRegisterMetaType<QVector<Segment>>("QVector<DoserModel::Segment>");
	qRegisterMetaType<SubProcessType>("DoserModel::SubProcessType");

	qsrand(QTime::currentTime().msec());
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

void DoserModel::doSegment(SegmentationMode mode)
{
	if (isSegmenting || image.isNull() || (mode != QUICK_MODE && mode != DEEP_MODE))
	{
		throw;
	}

	emit segmentationStarted(mode);
	isSegmenting = true;
	weightedSegments.clear();
	externalPixels.clear();
	pendingPixels.clear();

	QVector<Node> internalNodes;
	for (int y = 0; y < image.height(); ++y)
	{
		for (int x = 0; x < image.width(); ++x)
		{
			if (mode == DEEP_MODE || ((double) qrand() / RAND_MAX) < SAMPLING_PROBABILITY)
			{
				internalNodes.append(qMakePair(Pixel(x, y), 0));
			}
			else
			{
				externalPixels.append(Pixel(x, y));
			}
		}
	}

	if (internalNodes.isEmpty())
	{
		internalNodes.reserve(externalPixels.size());
		for (int i = 0; i < externalPixels.size(); ++i)
		{
			internalNodes.append(qMakePair(externalPixels[i], 0));
		}

		externalPixels.clear();
	}

	double initialWeight = 1.0 / internalNodes.size();
	for (int i = 0; i < internalNodes.size(); ++i)
	{
		internalNodes[i].second = initialWeight;
	}

	int segmentedPixelCount = 0;
	int pixelCount = image.width() * image.height();
	int targetPixelCount = TARGET_SEGMENTATION_RATIO * pixelCount;
	while (segmentedPixelCount < targetPixelCount)
	{
		if (internalNodes.isEmpty())
		{
			break;
		}

		double dist;
		do
		{
			QVector<Node> prevInternalNodes(internalNodes);
			iterate(internalNodes);
			dist = distance(internalNodes, prevInternalNodes);
		} while (dist > ITERATION_PRECISION);

		WeightedSegment weightedSegment;
		QVector<Node> newInternalNodes;

		for (const Node& internalNode : internalNodes)
		{
			if (internalNode.second > initialWeight)
			{
				weightedSegment.append(internalNode);
			}
			else
			{
				newInternalNodes.append(internalNode);
			}
		}

		if (weightedSegment.empty()) // iff nodes is atomic
		{
			weightedSegment = internalNodes;
			internalNodes.clear();
		}
		else
		{
			internalNodes = newInternalNodes;
		}

		double sumOfWeights = 0.0;
		for (const QPair<Pixel, double>& weightedPixel : weightedSegment)
		{
			sumOfWeights += weightedPixel.second;
		}

		for (int i = 0; i < weightedSegment.size(); ++i)
		{
			weightedSegment[i].second /= sumOfWeights;
		}

		extrapolate(weightedSegment);

		initialWeight = 1.0 / internalNodes.size();
		for (int i = 0; i < internalNodes.size(); ++i)
		{
			internalNodes[i].second = initialWeight;
		}

		if (weightedSegment.size() < MINIMAL_SEGMENT_SIZE)
		{
			for (const QPair<Pixel, double>& weightedPixel : weightedSegment)
			{ // intentionally not Node
				pendingPixels.append(weightedPixel.first);
			}
		}
		else
		{
			weightedSegments.append(weightedSegment);
			emit segmentChanged(mode, toSegment(weightedSegment));
		}

		segmentedPixelCount += weightedSegment.size();
		emit segmentationProgress(segmentedPixelCount, pixelCount);
	}

	pendingPixels.append(externalPixels);
	for (int i = 0; i < internalNodes.size(); ++i)
	{
		pendingPixels.append(internalNodes[i].first);
	}

	mergePendingPixels();

	QVector<Segment> segments(weightedSegments.size());
	for (int i = 0; i < segments.size(); ++i)
	{
		segments[i] = toSegment(weightedSegments[i]);
	}

	emit segmentationFinished(mode, segments);
	isSegmenting = false;
}

void DoserModel::extrapolate(DoserModel::WeightedSegment &weightedSegment)
{
	int externalCount = externalPixels.size();
	if (externalCount == 0)
	{
		return;
	}

	QVector<QFuture<QPair<int, bool>>> futures(externalCount);
	for (int i = 0; i < externalCount; ++i)
	{
		const auto& extraPolateIthPixel = [=]()
		{
			return qMakePair(i, inducedWeight(weightedSegment, externalPixels[i]) >= 0);
		};

		futures[i] = QtConcurrent::run(extraPolateIthPixel);
	}

	QVector<Pixel> newExternalPixels;
	for (int i = 0; i < externalCount; ++i)
	{
		const QPair<int, bool>& extrapolated = futures[i].result();

		if (extrapolated.second)
		{
			weightedSegment.append(qMakePair(externalPixels[extrapolated.first], 0));
		}
		else
		{
			newExternalPixels.append(externalPixels[extrapolated.first]);
		}

		emit subProcessProgress(EXTRAPOLATION, i + 1, externalCount + 1);
	}

	externalPixels = newExternalPixels;
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

		emit subProcessProgress(ITERATION, i + 1, raceCount + 1);
	}

	double overallFitness = product(races, fitnesses);
	for (int i = 0; i < raceCount; ++i)
	{
		races[i].second *= fitnesses[i] / overallFitness;
	}
}

void DoserModel::mergePendingPixels()
{
	int pendingCount = pendingPixels.size();
	QVector<QFuture<QPair<int, QVector<WeightedSegment>::iterator>>> futures(pendingCount);

	for (int i = 0; i < pendingCount; ++i)
	{
		const auto& ithSimilarity = [=](const WeightedSegment& s1, const WeightedSegment& s2)
		{
			return inducedWeight(s1, pendingPixels[i]) < inducedWeight(s2, pendingPixels[i]);
		};

		const auto& mergeIthPending = [=]()
		{
			return qMakePair(i, std::max_element(weightedSegments.begin(), weightedSegments.end(), ithSimilarity));
		};

		futures[i] = QtConcurrent::run(mergeIthPending);
	}

	for (int i = 0; i < pendingCount; ++i)
	{
		const QPair<int, QVector<WeightedSegment>::iterator>& mergeInfo = futures[i].result();
		mergeInfo.second->append(qMakePair(pendingPixels[mergeInfo.first], 0));

		emit subProcessProgress(MERGING, i + 1, pendingPixels.size() + 1);
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

void DoserModel::segment(SegmentationMode mode)
{
	if (mode == BOTH_MODE)
	{
		doSegment(DEEP_MODE);
		doSegment(QUICK_MODE);
	}
	else
	{
		doSegment(mode);
	}
}
