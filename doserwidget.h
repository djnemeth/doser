#ifndef DOSERWIDGET_H
#define DOSERWIDGET_H

#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QImage>
#include <QLabel>
#include <QMap>
#include <QPoint>
#include <QProgressBar>
#include <QPushButton>
#include <QThread>
#include <QVector>
#include <QWidget>

#include "dosermodel.h"

enum ImageLabelType
{
	SOURCE_LABEL, DEEP_LABEL, QUICK_LABEL
};

class DoserWidget : public QWidget
{
	Q_OBJECT

public:
	static const int DEEP_GROUP_COLUMN_INDEX = 2;
	static const int QUICK_GROUP_COLUMN_INDEX = 3;

	explicit DoserWidget(QWidget* parent = nullptr);
	~DoserWidget();

signals:
	void doSegment(SegmentationMode);
	void doOpenImage(const QString& path);
	void status(const QString& message);

private slots:
	void changeGuiMode();
	void drawSegment(const DoserModel::Segment &segment);
	void imageChanged(const QImage& sourceImage);
	void iterationProgressChanged(int current, int max);
	void openImage();
	void segment();
	void segmentationFinished(const QVector<DoserModel::Segment>& segments);
	void segmentationProgressChanged(int current, int max);

private:
	SegmentationMode currentMode() const;
	void setButtonsEnabled(bool enabled);
	void setupModel();
	void setupGui();
	QGroupBox* createSettingsGui();
	QVector<QGroupBox*> createGuiGroups();
	void displayGridColumn(int column, bool isVisible);

	DoserModel* model;
	QThread modelThread;
	QImage sourceImage;
	QImage deepImage;

	QGridLayout* gridLayout;
	QComboBox* modeComboBox;
	QMap<ImageLabelType, QLabel*> imageLabels;

	QPushButton* segmentButton;
	QPushButton* openButton;
	QPushButton* saveQuickButton;
	QPushButton* saveDeepButton;

	QProgressBar* mainProgressBar;
	QProgressBar* subProgressBar;
};

#endif // DOSERWIDGET_H
