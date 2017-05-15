#ifndef DOSERMODEL_H
#define DOSERMODEL_H

#include <QImage>
#include <QObject>
#include <QMap>
#include <QPair>
#include <QPoint>
#include <QString>

class DoserModel : public QObject
{
	Q_OBJECT

public:
	enum SegmentationMode
	{
		QUICK_MODE, DEEP_MODE, BOTH_MODE
	};

	struct SegmentationParameters
	{
		double targetSegmentationRatio = 0.9;
		double minimalSegmentSize = 50;
		double iterationPrecision = 0.01;
		double samplingProbability = 0.1;
		double weightRatioSquare = 0.01;
		bool forceGrayscale = false;
	};

	enum SubProcessType
	{
		ITERATION, EXTRAPOLATION, MERGING
	};

	typedef QPoint Pixel;
	typedef QPair<Pixel, double> Node;
	typedef QVector<Pixel> Segment;
	typedef QVector<QPair<Pixel, double>> WeightedSegment; // intentionally not QVector<Node>

	DoserModel();

signals:
	void imageChanged(QImage image);
	void segmentationStarted(DoserModel::SegmentationMode mode);
	void segmentChanged(DoserModel::SegmentationMode mode, DoserModel::Segment segment);
	void segmentationFinished(DoserModel::SegmentationMode mode, QVector<DoserModel::Segment> finalSegments);
	void segmentationProgress(int current, int max);
	void subProcessProgress(DoserModel::SubProcessType type, int current, int max);

public slots:
	void openImage(const QString& path);
	void segment(DoserModel::SegmentationMode mode, DoserModel::SegmentationParameters parameters);

private:
	// segmentation procedures
	void doSegment(SegmentationMode mode);
	void initialize(SegmentationMode mode);
	void solve(SegmentationMode mode);
	void finalize(SegmentationMode mode);
	void iterate();
	void extrapolate(WeightedSegment& weightedSegment);
	void merge();

	// utility functions
	double distance(const QVector<Node>& v1, const QVector<Node>& v2) const;
	double inducedWeight(const WeightedSegment& weightedSegment, const Pixel& externalPixel) const;
	double product(const QVector<Node>& v1, const QVector<double>& v2) const;
	Segment toSegment(const WeightedSegment& weightedSegment) const;
	double weight(const Pixel& px1, const Pixel& px2) const;

	// image-related representation
	QImage image;
	bool isGrayscale;

	// segmentation-related representation
	bool isSegmenting = false;
	SegmentationParameters parameters;
	QVector<Node> internalNodes;
	QVector<Pixel> externalPixels;
	QVector<Pixel> pendingPixels;
	QVector<WeightedSegment> weightedSegments;
};

#endif // DOSERMODEL_H
