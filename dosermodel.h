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

	typedef QPoint Pixel;
	typedef QVector<Pixel> Segment;
	typedef QVector<QPair<Pixel, double>> WeightedSegment; // intentionally not QVector<Node>
	typedef QPair<Pixel, double> Node;

	DoserModel();

signals:
	void segmentChanged(DoserModel::SegmentationMode mode, DoserModel::Segment segment);
	void imageChanged(QImage image);
	void iterationProgress(int current, int max);
	void segmentationStarted(DoserModel::SegmentationMode mode);
	void segmentationFinished(DoserModel::SegmentationMode mode, QVector<DoserModel::Segment> finalSegments);
	void segmentationProgress(int current, int max);

public slots:
	void openImage(const QString& path);
	void segment(DoserModel::SegmentationMode mode);

private:
	static constexpr double ITERATION_PRECISION = 0.01;
	static constexpr double WEIGHT_RATIO_SQUARE = 0.01;
	static constexpr double TARGET_SEGMENTATION_RATIO = 0.9;
	static constexpr double MINIMAL_SEGMENT_SIZE = 15;
	static const bool FORCE_GRAYSCALE = false;

	double weight(const Pixel& px1, const Pixel& px2) const;
	double distance(const QVector<Node>& v1, const QVector<Node>& v2) const;
	double product(const QVector<Node>& v1, const QVector<double>& v2) const;
	void iterate(QVector<Node>& races);
	void mergePendingPixels(SegmentationMode mode);
	double inducedWeight(const WeightedSegment& weightedSegment, const Pixel& externalPixel) const;
	Segment toSegment(const WeightedSegment& weightedSegment) const;
	void doSegment(SegmentationMode mode);

	QImage image;
	bool isGrayscale;
	bool isSegmenting = false;
	QMap<SegmentationMode, QVector<WeightedSegment>> weightedSegments;
	QMap<SegmentationMode, QVector<Pixel>> pendingPixels;
};

#endif // DOSERMODEL_H
