#include "dosermodel.h"
#include <algorithm>
#include <cstdlib>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>
#include <QTime>
#include <QtMath>
#include <QVector>

// constructor

DoserModel::DoserModel()
{
	qRegisterMetaType<SegmentationMode>("DoserModel::SegmentationMode");
	qRegisterMetaType<Segment>("DoserModel::Segment");
	qRegisterMetaType<QVector<Segment>>("QVector<DoserModel::Segment>");
	qRegisterMetaType<SubProcessType>("DoserModel::SubProcessType");

	qsrand(QTime::currentTime().msec());
}

// public slots

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

// segmentation procedures

void DoserModel::doSegment(SegmentationMode mode)
{
	if (isSegmenting || image.isNull() || (mode != QUICK_MODE && mode != DEEP_MODE))
	{
		throw;
	}

	initialize(mode);
	solve(mode);
	finalize(mode);
}

void DoserModel::initialize(SegmentationMode mode)
{
	// initializing

	isSegmenting = true;
	emit segmentationStarted(mode);

	internalNodes.clear();
	externalPixels.clear();
	pendingPixels.clear();
	weightedSegments.clear();

	// sampling and filtering

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
		// fall back to deep mode

		internalNodes.reserve(externalPixels.size());
		for (int i = 0; i < externalPixels.size(); ++i)
		{
			internalNodes.append(qMakePair(externalPixels[i], 0));
		}

		externalPixels.clear();
	}
}

void DoserModel::solve(SegmentationMode mode)
{
	// initialize progress tracking

	int segmentedPixelCount = 0;
	int pixelCount = image.width() * image.height();
	int targetPixelCount = TARGET_SEGMENTATION_RATIO * pixelCount;

	// segmentation loop

	while (segmentedPixelCount < targetPixelCount)
	{
		if (internalNodes.isEmpty()) // ineffective extrapolation
		{
			break;
		}

		// set initial value

		double initialWeight = 1.0 / internalNodes.size();
		for (int i = 0; i < internalNodes.size(); ++i)
		{
			internalNodes[i].second = initialWeight;
		}

		// iteration loop

		double dist;
		do
		{
			QVector<Node> prevInternalNodes(internalNodes);
			iterate();
			dist = distance(internalNodes, prevInternalNodes);
		} while (dist > ITERATION_PRECISION);

		// extracting the segment

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

		// calculating the weighted characteristic vector

		double sumOfWeights = 0;
		for (const QPair<Pixel, double>& weightedPixel : weightedSegment)
		{
			sumOfWeights += weightedPixel.second;
		}

		for (int i = 0; i < weightedSegment.size(); ++i)
		{
			weightedSegment[i].second /= sumOfWeights;
		}

		// extrapolating

		extrapolate(weightedSegment);

		// registering the extended segment

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

		// progress tracking

		segmentedPixelCount += weightedSegment.size();
		emit segmentationProgress(segmentedPixelCount, pixelCount);
	}
}

void DoserModel::finalize(SegmentationMode mode)
{
	// collect leftover pixels

	pendingPixels.append(externalPixels);
	for (int i = 0; i < internalNodes.size(); ++i)
	{
		pendingPixels.append(internalNodes[i].first);
	}

	internalNodes.clear();

	// merge

	merge();

	// collect final segments and notify clients

	QVector<Segment> segments(weightedSegments.size());
	for (int i = 0; i < segments.size(); ++i)
	{
		segments[i] = toSegment(weightedSegments[i]);
	}

	emit segmentationFinished(mode, segments);
	isSegmenting = false;
}

void DoserModel::iterate()
{
	QVector<Node>& races = internalNodes;
	int raceCount = races.size();
	QVector<QFuture<QPair<int, double>>> futureFitnessInfos(raceCount);

	for (int i = 0; i < raceCount; ++i)
	{
		const auto& calculateIthFitnessInfo = [=]()
		{
			double fitness = 0;
			for (const Node& rival : races)
			{
				 fitness += rival.second * weight(races[i].first, rival.first);
			}

			return qMakePair(i, fitness);
		};

		futureFitnessInfos[i] = QtConcurrent::run(calculateIthFitnessInfo);
	}

	QVector<double> fitnesses(raceCount, 0);
	for (int i = 0; i < raceCount; ++i)
	{
		const QPair<int, double>& fitnessInfo = futureFitnessInfos[i].result();
		fitnesses[fitnessInfo.first] = fitnessInfo.second;

		emit subProcessProgress(ITERATION, i + 1, raceCount + 1);
	}

	double overallFitness = product(races, fitnesses);
	for (int i = 0; i < raceCount; ++i)
	{
		races[i].second *= fitnesses[i] / overallFitness;
	}
}

void DoserModel::extrapolate(WeightedSegment& weightedSegment)
{
	int externalCount = externalPixels.size();
	if (externalCount == 0)
	{
		return;
	}

	QVector<QFuture<QPair<int, bool>>> futureExtrapolationInfos(externalCount);
	for (int i = 0; i < externalCount; ++i)
	{
		const auto& calculateIthExtrapolationInfo = [=]()
		{
			return qMakePair(i, inducedWeight(weightedSegment, externalPixels[i]) >= 0);
		};

		futureExtrapolationInfos[i] = QtConcurrent::run(calculateIthExtrapolationInfo);
	}

	QVector<Pixel> newExternalPixels;
	for (int i = 0; i < externalCount; ++i)
	{
		const QPair<int, bool>& extrapolationInfo = futureExtrapolationInfos[i].result();

		if (extrapolationInfo.second)
		{
			weightedSegment.append(qMakePair(externalPixels[extrapolationInfo.first], 0));
		}
		else
		{
			newExternalPixels.append(externalPixels[extrapolationInfo.first]);
		}

		emit subProcessProgress(EXTRAPOLATION, i + 1, externalCount + 1);
	}

	externalPixels = newExternalPixels;
}

void DoserModel::merge()
{
	int pendingCount = pendingPixels.size();
	QVector<QFuture<QPair<int, QVector<WeightedSegment>::iterator>>> futureMergeInfos(pendingCount);

	for (int i = 0; i < pendingCount; ++i)
	{
		const auto& ithSimilarity = [=](const WeightedSegment& s1, const WeightedSegment& s2)
		{
			return inducedWeight(s1, pendingPixels[i])
				< inducedWeight(s2, pendingPixels[i]);
		};

		const auto& calculateIthMergeInfo = [=]()
		{
			return qMakePair(i, std::max_element(weightedSegments.begin(),
				weightedSegments.end(), ithSimilarity));
		};

		futureMergeInfos[i] = QtConcurrent::run(calculateIthMergeInfo);
	}

	for (int i = 0; i < pendingCount; ++i)
	{
		const QPair<int, QVector<WeightedSegment>::iterator>& mergeInfo = futureMergeInfos[i].result();
		mergeInfo.second->append(qMakePair(pendingPixels[mergeInfo.first], 0));

		emit subProcessProgress(MERGING, i + 1, pendingPixels.size() + 1);
	}
}

// utility functions

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

double DoserModel::inducedWeight(const WeightedSegment& weightedSegment, const Pixel& externalPixel) const
{
	double inducedWeight = 0;
	const Pixel& referencePixel = weightedSegment.first().first;

	for (const QPair<Pixel, double>& weightedPixel : weightedSegment)
	{
		inducedWeight += weightedPixel.second * (weight(weightedPixel.first, externalPixel)
			- weight(weightedPixel.first, referencePixel));
	}

	return inducedWeight;
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

DoserModel::Segment DoserModel::toSegment(const WeightedSegment& weightedSegment) const
{
	Segment segment(weightedSegment.size());
	for (int i = 0; i < weightedSegment.size(); ++i)
	{
		segment[i] = weightedSegment[i].first;
	}

	return segment;
}

double DoserModel::weight(const Pixel& px1, const Pixel& px2) const
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
