#ifndef DOSERWIDGET_H
#define DOSERWIDGET_H

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QImage>
#include <QLabel>
#include <QMap>
#include <QPoint>
#include <QProgressBar>
#include <QPushButton>
#include <QSpinBox>
#include <QThread>
#include <QVector>
#include <QWidget>

#include "dosermodel.h"

class DoserWidget : public QWidget
{
	Q_OBJECT

public:
	enum GuiElementType
	{
		SOURCE, DEEP, QUICK
	};

	static const int DEEP_GROUP_COLUMN_INDEX = 2;
	static const int QUICK_GROUP_COLUMN_INDEX = 3;

	explicit DoserWidget(QWidget* parent = nullptr);
	~DoserWidget();

signals:
	void doOpenImage(const QString& path);
	void doSegment(DoserModel::SegmentationMode mode, DoserModel::SegmentationParameters parameters);
	void status(const QString& message);

private slots:
	// handlers of model events
	void imageChanged(const QImage& image);
	void segmentationStarted(DoserModel::SegmentationMode mode);
	void drawSegment(DoserModel::SegmentationMode mode, const DoserModel::Segment& segment);
	void segmentationFinished(DoserModel::SegmentationMode mode, const QVector<DoserModel::Segment>& finalSegments);
	void segmentationProgressChanged(int current, int max);
	void subProcessProgressChanged(DoserModel::SubProcessType type, int current, int max);

	// utility slots
	void changeGuiMode();
	void openImage();
	void segment();

private:
	// initializer procedures
	void setupModel();
	void setupGui();
	QVector<QGroupBox*> createGuiGroups();
	QGroupBox* createSettingsGui();

	// gui-altering procedures
	void displayGridColumn(int column, bool isVisible);
	void resetImages();
	void setControlsEnabled(bool enabled);

	// utility functions
	DoserModel::SegmentationMode currentMode() const;
	GuiElementType toGuiElementType(DoserModel::SegmentationMode mode) const;
	QString toString(DoserModel::SegmentationMode mode) const;
	QString toString(DoserModel::SubProcessType type) const;

	// model-related attributes
	DoserModel* model;
	QThread modelThread;

	// display-related elements
	QGridLayout* gridLayout;
	QMap<GuiElementType, QImage> images;
	QMap<GuiElementType, QLabel*> imageLabels;
	QProgressBar* mainProgressBar;
	QProgressBar* subProgressBar;

	// controls
	QComboBox* modeComboBox;
	QSpinBox* targetSegmentationRatioSpin;
	QSpinBox* minimalSegmentSizeSpin;
	QDoubleSpinBox* iterationPrecisionSpin;
	QDoubleSpinBox* samplingProbabilitySpin;
	QDoubleSpinBox* weightRatioSpin;
	QCheckBox* forceGrayscaleCheckBox;
	QPushButton* segmentButton;
	QPushButton* openButton;
	QMap<GuiElementType, QPushButton*> saveButtons;
};

#endif // DOSERWIDGET_H
